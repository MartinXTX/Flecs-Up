/* Microbenchmark: ecs_field_w_size indirection overhead.
 *
 * Compares 3 access patterns:
 *   A) Current: ecs_field(&it, T, i) - 6 loads + 1 mul
 *   B) Tier-X1+: field_cache[index] + 1 mul - 2 loads + 1 mul
 *   C) Direct: precomputed base + offset*size - 1 mul
 *
 * Expected: C ~ B < A by 20-50% per call.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <time.h>

typedef struct { float x, y, z; } Position;
typedef struct { float dx, dy, dz; } Velocity;
typedef struct { float sx, sy, sz; } Scale;

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Scale);

static double now_us(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

int main(void) {
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, Velocity);
    ECS_COMPONENT_DEFINE(world, Scale);

    /* Create 100k entities with all 3 components */
    const int N = 100000;
    ecs_entity_t *entities = ecs_bulk_new_w_id(world, ecs_id(Position), N);
    Position p = {1, 2, 3};
    ecs_bulk_set_id(world, entities, N, ecs_id(Position), sizeof(Position), &p);

    for (int i = 0; i < N; i++) {
        ecs_add(world, entities[i], Velocity);
        ecs_add(world, entities[i], Scale);
    }

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Position) },
            { .id = ecs_id(Velocity) },
            { .id = ecs_id(Scale) }
        }
    });

    /* Warmup */
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            Position *p = ecs_field(&it, Position, 0);
            Velocity *v = ecs_field(&it, Velocity, 1);
            Scale *s = ecs_field(&it, Scale, 2);
            volatile float x = p[i].x + v[i].dx + s[i].sx;
            (void)x;
        }
    }

    /* Benchmark: 1000 iters x 100k entities = 100M field calls */
    const int ITERS = 1000;
    double t0 = now_us();
    long total = 0;
    for (int iter = 0; iter < ITERS; iter++) {
        it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                Position *p = ecs_field(&it, Position, 0);
                Velocity *v = ecs_field(&it, Velocity, 1);
                Scale *s = ecs_field(&it, Scale, 2);
                total += (long)(p[i].x + v[i].dx + s[i].sx);
            }
        }
    }
    double t1 = now_us();
    double elapsed_sec = (t1 - t0) / 1e6;
    double calls = (double)ITERS * 100000 * 3;  /* 3 fields per row */
    double ns_per_call = (elapsed_sec * 1e9) / calls;
    double m_call_per_sec = calls / elapsed_sec / 1e6;

    printf("[bench_field_indirect] %d iter x %d ent x 3 fields = %.0f M calls\n",
        ITERS, 100000, calls / 1e6);
    printf("  elapsed: %.3f sec\n", elapsed_sec);
    printf("  ns/call: %.2f ns\n", ns_per_call);
    printf("  throughput: %.2f M calls/sec\n", m_call_per_sec);
    printf("  total (sanity): %ld\n", total);

    ecs_fini(world);
    return 0;
}
