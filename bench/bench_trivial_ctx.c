/* TIER-V18 P1-2 trivial-cache op_ctx skip benchmark.
 *
 * Compares per-frame iter init/fini cost for trivial-cache queries.
 * Each "frame" creates a fresh ecs_iter_t via ecs_query_iter and
 * drains it with ecs_query_next. This is the typical game-loop pattern
 * where init/fini overhead accumulates across many queries/frames.
 *
 * Build (with Tier-v17 lib as control):
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
 *      /I . /I ..\include bench_trivial_ctx.c /Fe:bench_trivial_v17.exe ^
 *      release\v17_flecs_patched.lib
 *
 * Build (with Tier-v18 P1-2 lib):
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
 *      /I . /I ..\include bench_trivial_ctx.c /Fe:bench_trivial_v18p12.exe ^
 *      release\v18_p12_flecs_patched.lib
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
typedef struct { float sx, sy; } Scale;

int main(int argc, char **argv) {
    int n_frames = (argc > 1) ? atoi(argv[1]) : 1000;
    int n_q      = (argc > 2) ? atoi(argv[2]) : 100;
    int n_ent    = (argc > 3) ? atoi(argv[3]) : 10000;

    printf("=== bench_trivial_ctx (TIER-V18 P1-2): %d frames x %d queries x %d entities ===\n",
           n_frames, n_q, n_ent);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);
    ECS_COMPONENT(world, Scale);

    /* Create entities with all 3 components. */
    ecs_entity_t *ents = malloc(sizeof(ecs_entity_t) * n_ent);
    for (int i = 0; i < n_ent; i++) {
        ents[i] = ecs_new(world);
        ecs_set(world, ents[i], Pos,   {(float)i, 0.0f, 0.0f});
        ecs_set(world, ents[i], Vel,   {1.0f, 0.0f, 0.0f});
        ecs_set(world, ents[i], Scale, {1.0f, 1.0f});
    }

    /* Create n_q trivial-cache queries (different term combos to avoid
     * accidental caching collisions). All are 2-term And queries on
     * the same archetype -> trivial cache path. */
    ecs_query_t **queries = malloc(sizeof(ecs_query_t*) * n_q);
    for (int q = 0; q < n_q; q++) {
        /* Alternate between (Pos+Vel), (Pos+Scale), (Vel+Scale) for variety. */
        int combo = q % 3;
        ecs_id_t id0, id1;
        switch (combo) {
            case 0: id0 = ecs_id(Pos);   id1 = ecs_id(Vel);   break;
            case 1: id0 = ecs_id(Pos);   id1 = ecs_id(Scale); break;
            default: id0 = ecs_id(Vel);  id1 = ecs_id(Scale); break;
        }
        queries[q] = ecs_query_init(world, &(ecs_query_desc_t){
            .terms = {
                { .id = id0 },
                { .id = id1 },
            }
        });
    }

    /* Warmup. */
    {
        ecs_iter_t it;
        long long warm_total = 0;
        for (int w = 0; w < 5; w++) {
            for (int q = 0; q < n_q; q++) {
                it = ecs_query_iter(world, queries[q]);
                while (ecs_query_next(&it)) {
                    warm_total += it.count;
                }
            }
        }
        printf("warmup entities touched: %lld\n", warm_total);
    }

    /* Timed run: each frame iterates ALL n_q queries (fresh iterator each time).
     * This stresses the per-iter init/fini path that P1-2 optimizes. */
    double t0 = now_sec();
    long long checksum = 0;
    int q_with_skip = 0;
    for (int f = 0; f < n_frames; f++) {
        for (int q = 0; q < n_q; q++) {
            ecs_iter_t it = ecs_query_iter(world, queries[q]);
            int yielded = 0;
            while (ecs_query_next(&it)) {
                yielded += it.count;
                /* Tiny work so compiler doesn't elide the iter. */
                const Pos *p = ecs_field(&it, Pos, 0);
                if (p && it.count > 0) {
                    checksum += (long long)(p[0].x * 1000.0f);
                }
            }
            /* Touch yielded so dead-code elim doesn't drop it. */
            checksum += yielded;
        }
    }
    double t1 = now_sec();

    /* Determine if any query was actually trivial-cache-skipped by inspecting
     * the public EcsQueryIsTrivial flag on each query. */
    int n_trivial = 0;
    int n_cacheable = 0;
    for (int q = 0; q < n_q; q++) {
        const ecs_query_t *qq = queries[q];
        ecs_flags32_t flags = qq->flags;
        if (flags & EcsQueryIsTrivial) n_trivial++;
        if (flags & EcsQueryIsCacheable) n_cacheable++;
    }

    double secs = t1 - t0;
    double iters_per_sec = (double)(n_frames * n_q) / secs;
    double frames_per_sec = (double)n_frames / secs;
    double ns_per_iter = secs * 1e9 / (double)(n_frames * n_q);

    printf("\nResults:\n");
    printf("  queries: %d  (trivial=%d, cacheable=%d)\n", n_q, n_trivial, n_cacheable);
    printf("  frames:  %d\n", n_frames);
    printf("  entities: %d\n", n_ent);
    printf("  total iters: %d\n", n_frames * n_q);
    printf("  Time: %.4f sec\n", secs);
    printf("  Iter throughput: %.1f K iter/sec\n", iters_per_sec / 1000.0);
    printf("  Frame throughput: %.1f frames/sec\n", frames_per_sec);
    printf("  Per-iter cost: %.1f ns\n", ns_per_iter);
    printf("  Per-frame cost (all queries): %.1f us\n", ns_per_iter * n_q / 1000.0);
    printf("  checksum=%lld\n", checksum);

    if (n_trivial == n_q && n_cacheable == n_q) {
        printf("  >>> ALL %d queries qualified for P1-2 op_ctx skip <<<\n", n_q);
    } else if (n_trivial > 0) {
        printf("  >>> %d/%d queries qualified for P1-2 op_ctx skip <<<\n",
               n_trivial, n_q);
    } else {
        printf("  >>> WARNING: 0 queries are EcsQueryIsTrivial — P1-2 will NOT skip.\n");
    }

    free(queries);
    free(ents);
    ecs_fini(world);
    return 0;
}