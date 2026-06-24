/**
 * @file addons/cpp/view.hpp
 * @brief Compile-time query view (Tier-A2: EnTT basic_view<...> port).
 *
 * Provides flecs::view<Components...> template that resolves component ids and
 * column layout at compile time. Inside the each() callback, fields are
 * resolved by index, eliminating the runtime term->id->column lookup that
 * the dynamic world::each<...> performs.
 *
 * Goals:
 *  - Compile-time id binding (ecs_id<T>() resolved in constructor; no runtime
 *    term_id array lookup per iteration).
 *  - Type-safe column access via flecs::field<T, Index>() template where
 *    sizeof(T) is a constant fold.
 *  - Trivial cache + sparse-set integration at iteration layer.
 *  - Header-only, no link-time cost.
 *
 * Note: This is the v0 cut. The dynamic world::each<...> path remains the
 * primary API; view<...> is additive.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <utility>
#include <tuple>
#include <type_traits>

namespace flecs
{

/**
 * @defgroup cpp_view Compile-time views
 * @ingroup cpp_core
 * Tier-A2 compile-time query views.
 *
 * @{
 */

/**
 * Compile-time type-safe column accessor (Tier-A2 view API).
 *
 * sizeof(T) is a compile-time constant, so the runtime path is a single
 * ecs_field_w_size call with a baked-in size. Renamed from `field` to
 * `view_field` to avoid collision with the existing `flecs::field<T>`
 * array-wrapper template (which has different semantics).
 *
 * @tparam T     Component type.
 * @tparam Index Column index (default 0).
 *
 * @ingroup cpp_view
 */
template <typename T, int Index = 0>
inline T* view_field(ecs_iter_t *it) {
    return static_cast<T*>(ecs_field_w_size(it, sizeof(T), Index));
}

template <typename T, int Index = 0>
inline const T* view_field(const ecs_iter_t *it) {
    return static_cast<const T*>(ecs_field_w_size(
        const_cast<ecs_iter_t*>(it), sizeof(T), Index));
}

/* Compile-time trait: does T carry EcsDontFragment / EcsSparse flag?
 * Default false. Specialize via FLECS_DECLARE_SPARSE(T) below. */
template <typename T>
struct is_sparse : std::false_type {};

template <typename T>
constexpr bool is_sparse_v = is_sparse<T>::value;

/* Token-pasting helpers (force macro re-scan after template substitution).
 *
 * ecs_id() expands to FLECS_ID<T>ID_ which requires T to be a single token.
 * When T is itself a template parameter, the preprocessor pastes the literal
 * token "T" giving FLECS_IDTID_. We work around this by routing through an
 * indirection (a class template static method whose body is parsed only at
 * instantiation), and using _::type<T>::id(world) — the C++ wrapper's
 * template-based id resolver — instead of the macro form.
 *
 * The actual id lookup happens inside view<...>::build_terms() below, which
 * is a non-static member function. Its body is only parsed when view<...>
 * is instantiated, by which point the full _::type<T>::id() definition from
 * component.hpp is visible.
 */

/** Mark a component type as sparse (EcsDontFragment). */
#define FLECS_DECLARE_SPARSE(T) \
    namespace flecs { template <> struct is_sparse<T> : std::true_type {}; }

/**
 * Compile-time view over a fixed set of components.
 *
 * Pattern follows EnTT's basic_view<...>: each component in the parameter
 * pack contributes one term to the underlying query, and each term's column
 * index is bound to its position in the pack.
 *
 * Example:
 * @code
 *   flecs::view<Position, Velocity> v(world);
 *   v.each([](flecs::entity e, Position& p, Velocity& v) {
 *       p.x += v.dx; p.y += v.dy;
 *   });
 * @endcode
 *
 * @tparam Components Component types, in column order.
 *
 * @ingroup cpp_view
 */
template <typename... Components>
class view {
    static_assert(sizeof...(Components) > 0,
        "flecs::view<> requires at least one component type");

public:
    /** Number of columns in the view. */
    static constexpr std::size_t column_count = sizeof...(Components);

    /** Construct a view over a world.
     *
     * @param w    World handle.
     * @param name Optional query name (debug only).
     */
    explicit view(world_t *w, const char *name = nullptr)
        : world_(w)
        , q_(nullptr)
    {
        /* Build terms with compile-time resolved component ids.
         * Uses _::type<T>::id(world) — the C++ wrapper's template id lookup —
         * resolved lazily via the deferred template instantiation of the view
         * constructor body. */
        std::size_t i = 0;
        ((terms_[i].id = _::type<Components>::id(w),
          terms_[i].inout = EcsInOutDefault,
          ++i), ...);

        ecs_query_desc_t desc{};
        desc.expr = name; /* Optional debug name. */
        /* Copy terms into desc.terms[0..N-1]; rest stay zero (sentinel). */
        for (std::size_t k = 0; k < terms_.size(); ++k) {
            desc.terms[k] = terms_[k];
        }
        q_ = ecs_query_init(w, &desc);
    }

    /** Destructor. Frees the query. */
    ~view() {
        if (q_) ecs_query_fini(q_);
    }

    view(const view&) = delete;
    view& operator=(const view&) = delete;

    /** Access underlying query (escape hatch). */
    ecs_query_t* query() const { return q_; }

    /** Iterate; for each entity, invoke func with typed column refs.
     *
     * Callback signature: void(flecs::entity, Components&...)
     */
    template <typename Func>
    void each(Func&& func) {
        constexpr auto seq = std::index_sequence_for<Components...>{};
        ecs_iter_t it = ecs_query_iter(world_, q_);
        while (ecs_query_next(&it)) {
            const int32_t count = it.count;
            /* Resolve column pointers once per chunk. */
            auto cols = make_columns(it, seq);
            for (int32_t row = 0; row < count; ++row) {
                invoke_each_row(func, it.entities[row], cols, row, seq);
            }
        }
    }

    /** Iterate raw chunks; callback gets column pointers directly.
     *
     * Callback signature: void(Components*...)
     */
    template <typename Func>
    void iter(Func&& func) {
        ecs_iter_t it = ecs_query_iter(world_, q_);
        while (ecs_query_next(&it)) {
            invoke_iter(func, it, std::index_sequence_for<Components...>{});
        }
    }

private:
    /* Build tuple<Components*...> via fold expression. */
    template <std::size_t... Is>
    std::tuple<Components*...> make_columns(ecs_iter_t& it,
                                            std::index_sequence<Is...>) const {
        return std::tuple<Components*...>{
            flecs::view_field<Components, static_cast<int>(Is)>(&it)...
        };
    }

    /* Invoke each-row callback with typed refs.
     * Pass entity_t (raw id) as first arg since flecs::entity has an explicit
     * constructor and many user callbacks take flecs::entity_t directly. */
    template <typename Func, std::size_t... Is>
    static void invoke_each_row(Func& func, ecs_entity_t e,
                                std::tuple<Components*...>& cols,
                                std::size_t row,
                                std::index_sequence<Is...>) {
        func(static_cast<flecs::entity_t>(e),
             std::get<Is>(cols)[row]...);
    }

    template <typename Func, std::size_t... Is>
    static void invoke_iter(Func& func, ecs_iter_t& it,
                            std::index_sequence<Is...>) {
        func(flecs::view_field<Components, static_cast<int>(Is)>(&it)...);
    }

    world_t *world_;
    ecs_query_t *q_;
    std::array<ecs_term_t, sizeof...(Components)> terms_{};
};

/**
 * Free-function factory (does not require world::view to be in scope).
 */
template <typename... Components>
inline flecs::view<Components...> make_view(world_t *w, const char *name = nullptr) {
    return flecs::view<Components...>(w, name);
}

} // namespace flecs

/** @} */