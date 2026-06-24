/* Microbenchmark v3: ecs_field_w_size with L1-resident working set.
 * Tests if Tier-X1+ field cache helps when data is in L1 cache.
 *
 * Setup: 1000 entities (3.6 KB data) — fits in 32KB L1.
 * Compare: cache miss (first call) vs cache hit (subsequent calls).
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
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

    /* Small N = 1000 entities = 3.6 KB data, fits in L1 cache */
    const int N = 1000;
    ecs_entity_t *entities = ecs_bulk_new_w_id(world, ecs_id(Position), N);

    Position p = {1.0f, 2.0f, 3.0f};
    Velocity v = {10.0f, 20.0f, 30.0f};
    Scale s = {100.0f, 200.0f, 300.0f};

    ecs_bulk_set_id(world, entities, N, ecs_id(Position), sizeof(Position), &p);

    ecs_id_t ids[] = { ecs_id(Velocity), ecs_id(Scale) };
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, entities[i], ids, 2);
    }
    ecs_bulk_set_id(world, entities, N, ecs_id(Velocity), sizeof(Velocity), &v);
    ecs_bulk_set_id(world, entities, N, ecs_id(Scale), sizeof(Scale), &s);

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Position) },
            { .id = ecs_id(Velocity) },
            { .id = ecs_id(Scale) }
        }
    });

    /* Many iterations to amortize the loop overhead */
    const int ITERS = 100000;  /* 100k iters × 1k ent × 3 fields = 300M calls */

    /* Warmup */
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            Position *p = ecs_field(&it, Position, 0);
            (void)p[i].x;
        }
    }

    double t0 = now_us();
    int64_t total = 0;
    for (int iter = 0; iter < ITERS; iter++) {
        it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                Position *p = ecs_field(&it, Position, 0);
                Velocity *v = ecs_field(&it, Velocity, 1);
                Scale *s = ecs_field(&it, Scale, 2);
                total += (int64_t)(p[i].x + v[i].dx + s[i].sx);
            }
        }
    }
    double t1 = now_us();
    double elapsed_sec = (t1 - t0) / 1e6;
    double calls = (double)ITERS * N * 3;
    double ns_per_call = (elapsed_sec * 1e9) / calls;

    int64_t expected = (int64_t)ITERS * N * (int64_t)(p.x + v.dx + s.sx);

    printf("[bench_field_l1 N=%d] %.0f M calls | %.3f sec | %.2f ns/call | %.2f M/sec\n",
        N, calls / 1e6, elapsed_sec, ns_per_call, calls / elapsed_sec / 1e6);
    printf("  expected: %lld, measured: %lld (%s)\n",
        (long long)expected, (long long)total,
        expected == total ? "MATCH" : "MISMATCH");

    ecs_fini(world);
    return 0;
}
