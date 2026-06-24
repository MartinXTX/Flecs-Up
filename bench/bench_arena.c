/* TIER-V17 P2-2 arena benchmark.
 * Focused benchmark: 100k iter over trivial cache query.
 * Compares 4-block-allocator baseline (Tier-v14.1) vs 1-arena (Tier-v17 P2-2).
 *
 * Build:
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include ^
 *      bench_arena.c /Fe:bench_arena_v17p22.exe flecs_v17_p22.obj
 */
#define FLECS_NO_OS_API
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static double now_sec(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}
#else
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

typedef struct { float x, y, z; } Pos;
typedef struct { float vx, vy, vz; } Vel;

int main(int argc, char **argv) {
    int n_iters = (argc > 1) ? atoi(argv[1]) : 100;
    int n_ent   = (argc > 2) ? atoi(argv[2]) : 100000;

    printf("=== bench_arena (TIER-V17 P2-2): %d iters x %d entities ===\n", n_iters, n_ent);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);

    /* Create entities with both components. */
    ecs_entity_t *ents = malloc(sizeof(ecs_entity_t) * n_ent);
    for (int i = 0; i < n_ent; i++) {
        ents[i] = ecs_new(world);
        ecs_set(world, ents[i], Pos, {(float)i, 0.0f, 0.0f});
        ecs_set(world, ents[i], Vel, {1.0f, 0.0f, 0.0f});
    }

    /* Trivial cache query: Pos + Vel. */
    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t){
        .terms = {
            { .id = ecs_id(Pos) },
            { .id = ecs_id(Vel) },
        }
    });

    /* Warmup. */
    ecs_iter_t it;
    int total = 0;
    for (int w = 0; w < 5; w++) {
        ecs_iter_t warm = ecs_query_iter(world, q);
        while (ecs_query_next(&warm)) {
            total += warm.count;
        }
    }

    /* Timed run. */
    double t0 = now_sec();
    long long checksum = 0;
    for (int i = 0; i < n_iters; i++) {
        it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Pos *p = ecs_field(&it, Pos, 0);
            Vel *v = ecs_field(&it, Vel, 1);
            for (int j = 0; j < it.count; j++) {
                p[j].x += v[j].vx * 0.01f;
                checksum += (long long)(p[j].x * 1000.0f);
            }
        }
    }
    double t1 = now_sec();

    double secs = t1 - t0;
    double ent_per_sec = (double)n_iters * n_ent / secs / 1e6;
    double iters_per_sec = (double)n_iters / secs;
    printf("Time: %.4f sec | %.1f M ent/sec | %.1f K iters/sec | checksum=%lld\n",
           secs, ent_per_sec, iters_per_sec / 1000.0, checksum);

    free(ents);
    ecs_fini(world);
    return 0;
}
