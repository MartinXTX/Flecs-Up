/**
 * Tier-A2 BENCHMARK: flecs::view<Components...> vs runtime flecs::query.
 *
 * Measures end-to-end iteration throughput (entities/sec) for both paths
 * over a 1M-entity workload and reports the speedup factor.
 *
 * Status: depends on a C++-compiled flecs lib (see test_view_template.cpp
 * for the build requirement). Once the runtime correctness test passes,
 * uncomment the benchmark loop and run.
 *
 * Expected gain (from PERFORMANCE_AUDIT.md, target +30-100%):
 *   - Compile-time column binding eliminates per-chunk ecs_field_id() lookup
 *   - sizeof(T) is a constant fold (no runtime size_t passed to ecs_field_w_size)
 *   - Template lambda inlining by MSVC
 */

#define FLECS_CPP_NO_AUTO_REGISTRATION
#include "flecs.h"
#include "flecs/addons/cpp/flecs.hpp"

#include <cstdio>
#include <cstdint>
#include <chrono>

struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health   { int   hp; };

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Health);

using clk = std::chrono::high_resolution_clock;

static double bench_runtime(ecs_world_t *w, int N) {
    ecs_query_desc_t qd{};
    qd.terms[0].id = ecs_id(Position);
    qd.terms[1].id = ecs_id(Velocity);
    ecs_query_t *q = ecs_query_init(w, &qd);

    volatile uint64_t sink = 0;
    auto t0 = clk::now();
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) {
        const Position *p = static_cast<const Position*>(ecs_field(&it, Position, 0));
        const Velocity *v = static_cast<const Velocity*>(ecs_field(&it, Velocity, 1));
        for (int i = 0; i < it.count; ++i) {
            sink += static_cast<uint64_t>(p[i].x) + static_cast<uint64_t>(v[i].dx);
        }
    }
    auto t1 = clk::now();
    ecs_query_fini(q);
    (void)sink;
    return std::chrono::duration<double>(t1 - t0).count();
}

static double bench_template(ecs_world_t *w, int N) {
    flecs::view<Position, Velocity> v(w, "view_bench");

    volatile uint64_t sink = 0;
    auto t0 = clk::now();
    v.each([&sink](flecs::entity_t /*e*/, Position& p, Velocity& vv) {
        sink += static_cast<uint64_t>(p.x) + static_cast<uint64_t>(vv.dx);
    });
    auto t1 = clk::now();
    (void)sink;
    return std::chrono::duration<double>(t1 - t0).count();
}

int main(int argc, char **argv) {
    const int N = (argc > 1) ? atoi(argv[1]) : 1000000;
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

    /* Warm-up. */
    bench_runtime(w, N);
    bench_template(w, N);

    /* Measured. */
    double t_rt = bench_runtime(w, N);
    double t_tpl = bench_template(w, N);
    double thr_rt = (double)N / t_rt;
    double thr_tpl = (double)N / t_tpl;
    double speedup = t_rt / t_tpl;

    printf("[Tier-A2 bench] N=%d\n", N);
    printf("  runtime  : %8.3f s  (%8.0f ent/s)\n", t_rt, thr_rt);
    printf("  template : %8.3f s  (%8.0f ent/s)\n", t_tpl, thr_tpl);
    printf("  speedup  : x%.2f\n", speedup);

    ecs_fini(w);
    return 0;
}