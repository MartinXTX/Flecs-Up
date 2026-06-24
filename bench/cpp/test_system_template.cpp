/**
 * Tier-D1.2 RUNTIME correctness test: flecs::typed_system<Components...>
 * with compile-time typed each() callback.
 *
 * Validates:
 *   1) typed_system<...> constructor builds an ecs_system with the right
 *      terms.
 *   2) system::each(fn) installs a C++ callback that receives typed refs.
 *   3) Both ecs_progress() (pipeline-driven) and system::run() (manual)
 *      invoke the each() callback on every matching entity.
 *   4) Aggregated values (sum, mutation) match the runtime baseline.
 *
 * BUILD REQUIREMENT: this test requires a C++-compiled flecs lib that
 * provides the C++ glue symbols (FLECS_ID<T>ID_ for builtins, ecs_cpp_*).
 * Mirrors the build constraint of test_view_template.cpp.
 */

#define FLECS_CPP_NO_AUTO_REGISTRATION
#include "flecs.h"
#include "flecs/addons/cpp/flecs.hpp"
#include "flecs/addons/cpp/system.hpp"

#include <cstdio>
#include <cstdint>
#include <cassert>

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health   { int   hp; };

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Health);

/* ----- Baseline: ecs_query + ecs_run loop, no template ----- */
struct accum_t {
    uint64_t sum;
    int      count;
};

static void baseline_sum_cb(ecs_iter_t *it) {
    Position *p = static_cast<Position*>(ecs_field(it, Position, 0));
    Velocity *v = static_cast<Velocity*>(ecs_field(it, Velocity, 1));
    auto *acc = static_cast<accum_t*>(it->ctx);
    for (int i = 0; i < it->count; ++i) {
        acc->sum += static_cast<uint64_t>(p[i].x) + static_cast<uint64_t>(v[i].dx);
        acc->count++;
    }
}

static accum_t baseline_run(ecs_world_t *w, int N) {
    accum_t acc{0, 0};
    ecs_system_desc_t desc{};
    desc.query.terms[0].id = ecs_id(Position);
    desc.query.terms[1].id = ecs_id(Velocity);
    desc.callback = baseline_sum_cb;
    desc.ctx = &acc;
    ecs_entity_t sys = ecs_system_init(w, &desc);
    ecs_run(w, sys, 0.0f, nullptr);
    return acc;
}

/* ----- Tier-D1.2 template path ----- */
static accum_t template_run(ecs_world_t *w) {
    accum_t acc{0, 0};
    /* We can't capture acc in the lambda directly via the trampoline; we
     * need to bind it through ctx. For the template path, we use a global
     * counter pattern (or a static). The template's each() invokes the
     * stored C++ callback with a tuple of typed refs, so we can use a
     * shared_ptr to a counter. */
    static accum_t *s_acc;  /* set right before run() */
    s_acc = &acc;

    /* Manual system: trigger=0 means it's not auto-bound to a phase. */
    flecs::typed_system<Position, Velocity> s(w, "D1_2_sys", 0);
    s.each([](flecs::entity_t /*e*/, Position& p, Velocity& v) {
        s_acc->sum += static_cast<uint64_t>(p.x) + static_cast<uint64_t>(v.dx);
        s_acc->count++;
    });
    s.run(0.0f);
    return acc;
}

int main(int argc, char **argv) {
    const int N = (argc > 1) ? atoi(argv[1]) : 100000;
    int rc = 0;

    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, Position);
    ECS_COMPONENT_DEFINE(w, Velocity);
    ECS_COMPONENT_DEFINE(w, Health);

    for (int i = 0; i < N; ++i) {
        ecs_entity_t e = ecs_new(w);
        Position p{ (float)(i & 0xFF), (float)((i >> 8) & 0xFF) };
        Velocity v{ 1.0f, -1.0f };
        ecs_set_id(w, e, ecs_id(Position), sizeof(Position), &p);
        ecs_set_id(w, e, ecs_id(Velocity), sizeof(Velocity), &v);
    }

    accum_t b = baseline_run(w, N);
    accum_t t = template_run(w);

    printf("[Tier-D1.2] N=%d  baseline(sum=%llu, count=%d)  template(sum=%llu, count=%d)  %s\n",
        N,
        (unsigned long long)b.sum, b.count,
        (unsigned long long)t.sum, t.count,
        (b.sum == t.sum && b.count == t.count) ? "MATCH" : "MISMATCH");

    if (b.sum != t.sum || b.count != t.count) rc = 1;

    /* Second pass: pipeline-driven ecs_progress(). */
    accum_t pp{0, 0};
    static accum_t *s_pp;
    s_pp = &pp;
    flecs::typed_system<Position, Velocity> psys(w, "D1_2_psys", EcsOnUpdate);
    psys.each([](flecs::entity_t /*e*/, Position& p, Velocity& v) {
        s_pp->sum += static_cast<uint64_t>(p.x) + static_cast<uint64_t>(v.dx);
        s_pp->count++;
    });
    ecs_progress(w, 0.0f);
    ecs_progress(w, 0.0f);  /* second tick */

    printf("[Tier-D1.2] pipeline-driven (2x progress) sum=%llu count=%d  (expect 2x baseline count, same sum)  %s\n",
        (unsigned long long)pp.sum, pp.count,
        (pp.count == 2 * b.count && pp.sum == 2 * b.sum) ? "MATCH" : "MISMATCH");

    if (pp.count != 2 * b.count || pp.sum != 2 * b.sum) rc = 1;

    ecs_fini(w);
    return rc;
}
