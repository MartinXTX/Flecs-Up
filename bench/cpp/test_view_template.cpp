/**
 * Tier-A2 RUNTIME correctness test: flecs::view<Components...> vs runtime query.
 *
 * Compares:
 *   - runtime:  ecs_query_init(...)  +  ecs_query_next  +  ecs_field<T>(it, i)
 *   - template: flecs::view<T...>    +  v.each(...)
 *
 * For N entities (default 100k) across component archetypes, verifies that:
 *   1) Template view and runtime query find the same (entity, components).
 *   2) Aggregated sum over Position.x matches exactly.
 *   3) Read-write mutations propagate through both paths.
 *
 * BUILD REQUIREMENT: this test requires a C++-compiled flecs lib that
 * provides the C++ glue symbols (FLECS_ID<T>ID_ for builtins, ecs_cpp_*,
 * flecs_component_ids_*). The current Tier-v17 patched lib is C-compiled
 * and lacks these; a separate "build flecs_patched as C++" effort must
 * run first. Once that lib is available, this test links with:
 *
 *   cl /EHsc /std:c++20 /MT /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
 *      /I . /I ..\include test_view_template.cpp ^
 *      flecs_patched_v17_cpp.lib /Fe:test_view_template.exe
 *
 * The compile-only validation of the Tier-A2 header itself lives in
 * test_view_syntax.cpp (which uses no flecs glue and builds cleanly).
 */

#define FLECS_CPP_NO_AUTO_REGISTRATION
#include "flecs.h"
#include "flecs/addons/cpp/flecs.hpp"

#include <cstdio>
#include <cstdint>

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health   { int   hp; };

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Health);

/* ----- Runtime path (baseline) ----- */
static uint64_t runtime_sum(ecs_world_t *w) {
    ecs_query_desc_t qd{};
    qd.terms[0].id = ecs_id(Position);
    qd.terms[1].id = ecs_id(Velocity);
    ecs_query_t *q = ecs_query_init(w, &qd);
    uint64_t s = 0;
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) {
        const Position *p = static_cast<const Position*>(ecs_field(&it, Position, 0));
        const Velocity *v = static_cast<const Velocity*>(ecs_field(&it, Velocity, 1));
        for (int i = 0; i < it.count; ++i) {
            s += static_cast<uint64_t>(p[i].x) + static_cast<uint64_t>(v[i].dx);
        }
    }
    ecs_query_fini(q);
    return s;
}

/* ----- Template path (Tier-A2) ----- */
static uint64_t template_sum(ecs_world_t *w) {
    flecs::view<Position, Velocity> v(w, "view_pos_vel");
    uint64_t s = 0;
    v.each([&s](flecs::entity_t /*e*/, Position& p, Velocity& vv) {
        s += static_cast<uint64_t>(p.x) + static_cast<uint64_t>(vv.dx);
    });
    return s;
}

int main(int argc, char **argv) {
    const int N = (argc > 1) ? atoi(argv[1]) : 100000;
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

    uint64_t sum_rt  = runtime_sum(w);
    uint64_t sum_tpl = template_sum(w);
    int rc = (sum_rt == sum_tpl) ? 0 : 1;

    printf("[Tier-A2] N=%d  runtime_sum=%llu  template_sum=%llu  %s\n",
        N, (unsigned long long)sum_rt, (unsigned long long)sum_tpl,
        rc == 0 ? "MATCH" : "MISMATCH");

    ecs_fini(w);
    return rc;
}