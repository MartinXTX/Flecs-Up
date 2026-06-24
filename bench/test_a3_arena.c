/* TIER-A3 arena benchmark.
 * Persistent per-query iter arena: eliminates per-frame ecs_stack_calloc +
 * ecs_stack_free pairs on vars/written/op_ctx/profile. Compares query setup
 * + iteration throughput under repeated ecs_query_iter cycles.
 *
 * Build:
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . /I ..\include ^
 *      test_a3_arena.c /Fe:release\test_a3_arena.exe ^
 *      release\v18_a3_flecs_patched.lib
 */
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
typedef struct { float sx, sy; } Scl;

int main(int argc, char **argv) {
    int n_iters = (argc > 1) ? atoi(argv[1]) : 1000;
    int n_ent   = (argc > 2) ? atoi(argv[2]) : 10000;
    int n_terms = (argc > 3) ? atoi(argv[3]) : 3;  /* 1..5 */

    printf("=== TIER-A3 arena test: %d iters x %d entities x %d terms ===\n",
           n_iters, n_ent, n_terms);
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);
    ECS_COMPONENT(world, Scl);

    /* Create entities with all components. */
    for (int i = 0; i < n_ent; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, Pos, {(float)i, 0.0f, 0.0f});
        ecs_set(world, e, Vel, {1.0f, 0.0f, 0.0f});
        ecs_set(world, e, Scl, {1.0f, 1.0f});
    }

    /* Build a query with the requested number of terms. */
    ecs_query_desc_t desc = {0};
    if (n_terms >= 1) desc.terms[0].id = ecs_id(Pos);
    if (n_terms >= 2) desc.terms[1].id = ecs_id(Vel);
    if (n_terms >= 3) desc.terms[2].id = ecs_id(Scl);
    ecs_query_t *q = ecs_query_init(world, &desc);
    if (!q) { fprintf(stderr, "query init failed\n"); return 1; }

    /* Warmup. */
    long long checksum = 0;
    for (int w = 0; w < 10; w++) {
        ecs_iter_t warm = ecs_query_iter(world, q);
        while (ecs_query_next(&warm)) {
            Pos *p = ecs_field(&warm, Pos, 0);
            for (int j = 0; j < warm.count; j++) checksum += (long long)p[j].x;
        }
    }

    /* Phase A: time pure iter setup + iter_fini cycles (no yield).
     * This isolates the TIER-A3 savings: arena allocations only happen in
     * ecs_query_iter (not in ecs_query_next). We measure setup+fini by
     * calling ecs_query_iter + ecs_iter_fini without ecs_query_next. */
    double t_setup0 = now_sec();
    for (int i = 0; i < n_iters; i++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        ecs_iter_fini(&it);
    }
    double t_setup1 = now_sec();
    double setup_secs = t_setup1 - t_setup0;

    /* Phase A2: same but only the iter setup (no fini) - measures pure
     * arena reuse cost after warmup. */
    ecs_iter_t it_static;
    double t_setup2_0 = now_sec();
    for (int i = 0; i < n_iters; i++) {
        it_static = ecs_query_iter(world, q);
        ecs_iter_fini(&it_static);
    }
    double t_setup2_1 = now_sec();
    double setup2_secs = t_setup2_1 - t_setup2_0;

    /* Phase B: full iter cycle (iter + next + fini) — measures the cumulative
     * effect of TIER-A3 amortised against per-frame cost. */
    double t_full0 = now_sec();
    long long c2 = 0;
    for (int i = 0; i < n_iters; i++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Pos *p = ecs_field(&it, Pos, 0);
            for (int j = 0; j < it.count; j++) {
                p[j].x += 0.001f;
                c2 += (long long)(p[j].x * 1000.0f);
            }
        }
    }
    double t_full1 = now_sec();
    double full_secs = t_full1 - t_full0;

    /* Phase C: 1000 iters/frame x N frames — measures memory pressure &
     * arena cache stability. */
    int per_frame = 1000;
    int n_frames = (n_iters + per_frame - 1) / per_frame;
    double t_frame0 = now_sec();
    long long c3 = 0;
    for (int f = 0; f < n_frames; f++) {
        for (int i = 0; i < per_frame; i++) {
            ecs_iter_t it = ecs_query_iter(world, q);
            while (ecs_query_next(&it)) {
                Pos *p = ecs_field(&it, Pos, 0);
                for (int j = 0; j < it.count; j++) c3 += (long long)p[j].x;
            }
        }
    }
    double t_frame1 = now_sec();
    double frame_secs = t_frame1 - t_frame0;

    double setup_us = (setup_secs / (double)n_iters) * 1e6;
    double setup2_us = (setup2_secs / (double)n_iters) * 1e6;
    double full_us  = (full_secs  / (double)n_iters) * 1e6;
    double setup_per_sec = (double)n_iters / setup_secs;
    double setup2_per_sec = (double)n_iters / setup2_secs;
    double full_per_sec  = (double)n_iters / full_secs;

    printf("\n--- Phase A: pure iter setup+fini (no yield) ---\n");
    printf("  Time: %.4f sec | %.2f us/call | %.0f K setup/sec\n",
           setup_secs, setup_us, setup_per_sec / 1000.0);

    printf("\n--- Phase A2: repeated iter+fini (steady-state arena) ---\n");
    printf("  Time: %.4f sec | %.2f us/call | %.0f K setup/sec\n",
           setup2_secs, setup2_us, setup2_per_sec / 1000.0);

    printf("\n--- Phase B: full iter (iter + next + fini) ---\n");
    printf("  Time: %.4f sec | %.2f us/call | %.0f K full/sec\n",
           full_secs, full_us, full_per_sec / 1000.0);

    printf("\n--- Phase C: %d frames x %d iters/frame ---\n", n_frames, per_frame);
    printf("  Time: %.4f sec | %.0f K iters/sec\n",
           frame_secs, (double)(n_frames * per_frame) / frame_secs / 1000.0);

    printf("\nChecksums: warm=%lld  full=%lld  frame=%lld\n",
           checksum, c2, c3);

    ecs_fini(world);
    return 0;
}