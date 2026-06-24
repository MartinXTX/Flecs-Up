/**
 * @file addons/cpp/system.hpp
 * @brief Compile-time typed system (Tier-D1.2: EnTT basic_system port).
 *
 * Provides flecs::typed_system<Components...> template that resolves component ids
 * and column layout at compile time, and binds a C++ each() callback to the
 * system via a per-instance C thunk. The thunk's hot path is a single
 * ecs_field_w_size(it, sizeof(T), Index) per chunk — same pattern as the
 * Tier-A2 view template, but driven by the world's pipeline / manual run.
 *
 * Goals:
 *  - Compile-time id binding (ecs_id resolved once in constructor).
 *  - Type-safe column access via flecs::view_field<T, Index>().
 *  - Reuse ecs_system_init (gets pipeline integration for free) and ecs_run
 *    (manual invoke).
 *  - Header-only; no link-time cost beyond the C thunk trampoline.
 *
 * Usage:
 * @code
 *   flecs::typed_system<Position, Velocity> move_sys(world, "Move");
 *   move_sys.each([](flecs::entity_t e, Position& p, Velocity& v) {
 *       p.x += v.dx; p.y += v.dy;
 *   });
 *   world.progress();  // invokes the each() callback on every match
 * @endcode
 *
 * Or for the manual / EnTT-style path:
 * @code
 *   move_sys.run(0.0f);  // single-step, no pipeline
 * @endcode
 *
 * Note: This is a v0 cut. It coexists with the existing
 * flecs::system_builder<...>::each(...) path; using both in the same TU is
 * fine, but mixing the two on the same system entity is unsupported.
 */

#pragma once

#include "view.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace flecs
{

/**
 * @defgroup cpp_system Compile-time systems
 * @ingroup cpp_core
 * Tier-D1.2 compile-time typed systems.
 *
 * @{
 */

namespace _ {

/* Per-system dispatch state attached as ecs_system_desc_t::ctx. The C thunk
 * dereferences this and dispatches into the stored C++ callback. We use a
 * heap-allocated struct so the system can be moved/freed without lifetime
 * concerns: the C side keeps only the void* ctx pointer, and ctx_free
 * releases the dispatch object on system fini. */
template <typename... Cs>
struct system_dispatch {
    /* Type-erased move-only function holder. */
    struct holder_base {
        virtual ~holder_base() = default;
        virtual void invoke(ecs_iter_t *it) = 0;
    };
    template <typename Fn>
    struct holder : holder_base {
        Fn fn;
        explicit holder(Fn f) : fn(std::move(f)) {}
        void invoke(ecs_iter_t *it) override {
            invoke_impl(it, std::index_sequence_for<Cs...>{});
        }
        template <std::size_t... Is>
        void invoke_impl(ecs_iter_t *it, std::index_sequence<Is...>) {
            const int32_t count = it->count;
            auto cols = std::tuple<Cs*...>{
                flecs::view_field<Cs, static_cast<int>(Is)>(it)...};
            for (int32_t row = 0; row < count; ++row) {
                /* Pass raw entity id (entity_t = ecs_entity_t) so the callback
                 * can take either flecs::entity_t or flecs::entity (latter
                 * has an explicit constructor; users opt in via their
                 * parameter type). Matches view.hpp::invoke_each_row. */
                fn(static_cast<flecs::entity_t>(it->entities[row]),
                   std::get<Is>(cols)[row]...);
            }
        }
    };

    world_t *world;
    holder_base *fn_holder;

    explicit system_dispatch(world_t *w, holder_base *h)
        : world(w), fn_holder(h) {}
};

/* C thunk: signature matches ecs_iter_action_t. Reads ctx, dispatches. */
template <typename... Cs>
inline void system_trampoline(ecs_iter_t *it) {
    auto *d = static_cast<_::system_dispatch<Cs...>*>(it->ctx);
    if (d && d->fn_holder) {
        d->fn_holder->invoke(it);
    }
}

/* ctx_free callback: releases the dispatch object when the system is
 * finalized (via ecs_delete or world fini). */
template <typename... Cs>
inline void system_ctx_free(void *ptr) {
    auto *d = static_cast<_::system_dispatch<Cs...>*>(ptr);
    if (d) {
        delete d->fn_holder;
        delete d;
    }
}

} // namespace _

/**
 * Compile-time typed system.
 *
 * Mirrors the flecs::view<...> pattern but persists across ecs_progress()
 * calls and integrates with the pipeline. The constructor resolves all
 * component ids at compile time and stores them in a static term array.
 * The system callback is a C trampoline that decodes ctx to find the
 * stored callback and dispatches.
 *
 * Note: this is named `typed_system<...>` to avoid clashing with
 * `flecs::system` (the existing single-class system handle) and
 * `flecs::system_builder<...>` (the builder returned by
 * `world::system<...>(...)`). Use `flecs::typed_system<Cs...>` directly
 * or `world::system_t<Cs...>(...)` for the factory.
 *
 * @tparam Components Component types, in column order.
 *
 * @ingroup cpp_system
 */
template <typename... Components>
class typed_system {
    static_assert(sizeof...(Components) > 0,
        "flecs::typed_system<> requires at least one component type");

public:
    /** Number of columns. */
    static constexpr std::size_t column_count = sizeof...(Components);

    /** Construct a system.
     *
     * @param w       World handle.
     * @param name    Optional system name (debug only).
     * @param trigger Phase / event id to subscribe to. Default EcsOnUpdate.
     *                Pass 0 to create a manual system (no auto-run).
     */
    explicit typed_system(world_t *w, const char *name = nullptr,
                          ecs_entity_t trigger = EcsOnUpdate)
        : world_(w)
        , entity_(0)
        , dispatch_(nullptr)
    {
        /* Compile-time term construction. Same fold pattern as view.hpp. */
        std::size_t i = 0;
        ((terms_[i].id = _::type<Components>::id(w),
          terms_[i].inout = EcsInOutDefault,
          ++i), ...);

        /* Resolve the system entity name first (C++ wrapper pattern). The
         * system desc itself has no name field; the name is set on the
         * entity that the system is bound to. */
        ecs_entity_desc_t entity_desc{};
        entity_desc.name = name;
        entity_desc.sep = "::";
        entity_desc.root_sep = "::";
        ecs_entity_t ent = ecs_entity_init(w, &entity_desc);

        /* Initialize the system with a noop callback. ecs_system_init creates
         * the system entity and query, so we always have a valid handle. The
         * each() method later swaps in the real callback via ecs_system_update.
         * If each() is never called, the system still exists and matches
         * entities; the noop callback short-circuits. */
        ecs_system_desc_t desc{};
        desc.entity = ent;
        /* Copy terms into desc.terms[0..N-1]; terms_[N] stays zero (sentinel). */
        for (std::size_t k = 0; k < terms_.size(); ++k) {
            desc.query.terms[k] = terms_[k];
        }
        if (trigger != 0) {
            desc.phase = trigger;
        }
        desc.callback = nullptr;  /* user must call each() to set callback */
        desc.run = nullptr;        /* default runner invokes callback per result */
        desc.immediate = false;
        desc.multi_threaded = false;
        entity_ = ecs_system_init(w, &desc);
    }

    /** Destructor. ecs_delete fires the ctx_free that releases dispatch. */
    ~typed_system() {
        if (entity_ != 0) {
            ecs_delete(world_, entity_);
        }
    }

    typed_system(const typed_system&) = delete;
    typed_system& operator=(const typed_system&) = delete;

    /** Bind a C++ callback.
     *
     * The callback gets a typed column reference for each component in the
     * parameter pack, in pack order. If the callback was previously set, it
     * is replaced and the previous dispatch is released.
     *
     * @tparam Fn Callable with signature
     *         void(flecs::entity, Components&...) (or any compatible form
     *         with flecs::entity_t as first arg).
     */
    template <typename Fn>
    void each(Fn&& fn) {
        /* Release previous dispatch holder if any. We don't go through
         * ecs_delete (which would destroy the system); we own the dispatch
         * and the C ctx_free will release it at system fini time. */
        if (dispatch_) {
            delete dispatch_->fn_holder;
            delete dispatch_;
            dispatch_ = nullptr;
        }
        /* Allocate fresh dispatch with the new C++ callback. */
        auto *holder = new typename _::system_dispatch<Components...>::
            template holder<std::decay_t<Fn>>(std::forward<Fn>(fn));
        dispatch_ = new _::system_dispatch<Components...>(world_, holder);

        /* Attach the dispatch object as the system ctx. ecs_system_update
         * re-runs the init logic with the new callback/ctx. The term array
         * is unchanged so the query stays valid. */
        ecs_system_desc_t update{};
        update.query.terms[0] = terms_[0];
        for (std::size_t k = 0; k < terms_.size(); ++k) {
            update.query.terms[k] = terms_[k];
        }
        update.callback = &_::system_trampoline<Components...>;
        update.ctx = dispatch_;
        update.ctx_free = &_::system_ctx_free<Components...>;
        ecs_system_update(world_, entity_, &update);
    }

    /** Run the system manually.
     *
     * @param delta_time Time delta in seconds (default 0).
     */
    ecs_entity_t run(ecs_ftime_t delta_time = 0.0f) {
        return ecs_run(world_, entity_, delta_time, nullptr);
    }

    /** Access underlying world. */
    world_t* world() const { return world_; }

    /** Access underlying system entity. */
    ecs_entity_t entity() const { return entity_; }

private:
    world_t *world_;
    ecs_entity_t entity_;
    _::system_dispatch<Components...> *dispatch_;
    /* sizeof...(C) + 1 slots: ecs_query_init treats terms[i].id == 0 as end.
     * We bake one extra zero slot at the end. */
    std::array<ecs_term_t, sizeof...(Components) + 1> terms_{};
};

/**
 * Free-function factory (does not require world::system to be in scope).
 */
template <typename... Components>
inline flecs::typed_system<Components...> make_typed_system(
    world_t *w, const char *name = nullptr,
    ecs_entity_t trigger = EcsOnUpdate) {
    return flecs::typed_system<Components...>(w, name, trigger);
}

} // namespace flecs

/** @} */
