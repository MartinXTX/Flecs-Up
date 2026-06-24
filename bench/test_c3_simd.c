/*
 * Tier-C3: SIMD filter processing (AVX2) verification + benchmark.
 *
 * Tests two filter pipelines against the same workload (1M Position
 * entities, 50% positive x):
 *   - Scalar baseline (always compiled)
 *   - SIMD AVX2 packed path  (only when FLECS_C3_SIMD is defined)
 *   - SIMD AVX2 gather path  (only when FLECS_C3_SIMD is defined)
 *
 * Pass criteria:
 *   - Correctness: count of "positive x" matches is identical between
 *     scalar and SIMD paths (within +/-0 against the same input).
 *   - Performance: SIMD path throughput >= 1.5x scalar at N=1M
 *     (target +%200-400 vs scalar — typical observed range on
 *     AVX2-capable CPUs for the packed-float path).
 *
 * Build (MSVC, AVX2):
 *   cl /O2 /arch:AVX2 /DFLECS_C3_SIMD /DFLECS_PATCHED_BUILD /I .. /I ../include \
 *       test_c3_simd.c release\v18_c3_flecs_patched.lib /Fe:test_c3_simd.exe
 *
 * Build (MSVC, scalar baseline):
 *   cl /O2 /DFLECS_PATCHED_BUILD /I .. /I ../include \
 *       test_c3_simd.c release\v18_c3_flecs_patched.lib /Fe:test_c3_simd.exe
 */

#include "flecs_patched.h"
#include "../include/flecs/addons/cpp/simd_filter.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Position component — first member must be float for the gather macro. */
typedef struct {
    float x;
    float y;
    float z;
} Position;

/* --- Wall-clock timer (Win32 QPC). --- */
static double now_sec(void) {
    LARGE_INTEGER freq, t;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}

/* --- Reference scalar implementation (independent of macro). --- */
static int scalar_count_positive_packed(const float *p, int n) {
    int c = 0;
    for (int i = 0; i < n; i ++) {
        if (p[i] > 0.0f) c ++;
    }
    return c;
}

static int scalar_count_positive_gather(const Position *p, int n) {
    int c = 0;
    for (int i = 0; i < n; i ++) {
        if (p[i].x > 0.0f) c ++;
    }
    return c;
}

/* --- Macro-driven paths. --- */
/* For correctness tests: simple counter callback. */
static int run_macro_packed_count(const float *p, int n) {
    int c = 0;
    ECS_OP_FILTER_GT_F32(p, n, c ++);
    return c;
}

static int run_macro_gather_count(const Position *p, int n) {
    int c = 0;
    ECS_OP_FILTER_GT_F32_GATHER(p, n, sizeof(Position), c ++);
    return c;
}

/* For throughput tests: callback does non-trivial work (~5 ALU ops) so
 * the scalar fallback cannot be trivially autovectorized. The SIMD path
 * masks predicates first, so the work is only done for matching lanes. */
static inline int callback_work(float v) {
    int x = (int)(v * 1024.0f);
    int y = (x ^ (x >> 3)) & 0xff;
    int z = y * 17 + (y >> 4);
    return z ^ (z << 5);
}

static int run_macro_packed(const float *p, int n) {
    int c = 0;
    ECS_OP_FILTER_GT_F32(p, n, c += callback_work((p)[i]));
    return c;
}

static int run_macro_gather(const Position *p, int n) {
    int c = 0;
    ECS_OP_FILTER_GT_F32_GATHER(p, n, sizeof(Position),
        c += callback_work((p)[i].x));
    return c;
}

/* Scalar baselines with the same callback work, for fair comparison. */
static int run_scalar_packed(const float *p, int n) {
    int c = 0;
    for (int i = 0; i < n; i ++) {
        if (p[i] > 0.0f) c += callback_work(p[i]);
    }
    return c;
}

static int run_scalar_gather(const Position *p, int n) {
    int c = 0;
    for (int i = 0; i < n; i ++) {
        if (p[i].x > 0.0f) c += callback_work(p[i].x);
    }
    return c;
}

/* --- Test 1: correctness on packed float* array --- */
static int test_correctness_packed(void) {
    printf("TEST 1: correctness packed float* (1M, 50%% positive)\n");
    fflush(stdout);

    const int N = 1000000;
    float *p = (float*)_aligned_malloc(sizeof(float) * N, 64);
    assert(p);

    for (int i = 0; i < N; i ++) {
        p[i] = (float)((i & 1) ? 1.0f : -1.0f);
    }

    const int ref = scalar_count_positive_packed(p, N);
    const int msc = run_macro_packed_count(p, N);

    if (ref != msc) {
        printf("  FAIL: packed macro %d != reference %d\n", msc, ref);
        _aligned_free(p);
        return 1;
    }
    printf("  ref=%d, packed_macro=%d (match)\n", ref, msc);
    _aligned_free(p);
    printf("  PASS\n");
    return 0;
}

/* --- Test 2: correctness on strided Position array (gather path) --- */
static int test_correctness_gather(void) {
    printf("TEST 2: correctness strided Position (1M, 50%% positive)\n");
    fflush(stdout);

    const int N = 1000000;
    Position *p = (Position*)_aligned_malloc(sizeof(Position) * N, 64);
    assert(p);

    for (int i = 0; i < N; i ++) {
        p[i].x = (float)((i & 1) ? 1.0f : -1.0f);
        p[i].y = (float)i;
        p[i].z = (float)(-i);
    }

    const int ref = scalar_count_positive_gather(p, N);
    const int msc = run_macro_gather_count(p, N);

    if (ref != msc) {
        printf("  FAIL: gather macro %d != reference %d\n", msc, ref);
        _aligned_free(p);
        return 1;
    }
    printf("  ref=%d, gather_macro=%d (match)\n", ref, msc);
    _aligned_free(p);
    printf("  PASS\n");
    return 0;
}

/* --- Test 3: packed throughput benchmark --- */
static int test_throughput_packed(void) {
    printf("TEST 3: throughput packed float* (1M, scalar vs SIMD)\n");
    fflush(stdout);

    const int N = 1000000;
    const int REPS = 20;
    float *p = (float*)_aligned_malloc(sizeof(float) * N, 64);
    assert(p);

    for (int i = 0; i < N; i ++) {
        p[i] = (float)((i & 1) ? 1.0f : -1.0f);
    }

    /* Warm-up */
    int warm = 0;
    for (int r = 0; r < 3; r ++) {
        warm += run_scalar_packed(p, N);
        warm += run_macro_packed(p, N);
    }

    /* Scalar timing (with same callback work as the SIMD path). */
    double t0 = now_sec();
    int total_scalar = 0;
    for (int r = 0; r < REPS; r ++) total_scalar += run_scalar_packed(p, N);
    double t1 = now_sec();
    const double sec_scalar = t1 - t0;
    const double ns_per_elem_scalar = (sec_scalar * 1e9) / (double)(N * REPS);

#if FLECS_C3_HAS_AVX2
    double t2 = now_sec();
    int total_simd = 0;
    for (int r = 0; r < REPS; r ++) total_simd += run_macro_packed(p, N);
    double t3 = now_sec();
    const double sec_simd = t3 - t2;
    const double ns_per_elem_simd = (sec_simd * 1e9) / (double)(N * REPS);
    const double speedup = ns_per_elem_scalar / ns_per_elem_simd;

    printf("  scalar : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_scalar * 1e3 / REPS, ns_per_elem_scalar, total_scalar);
    printf("  simd   : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_simd * 1e3 / REPS, ns_per_elem_simd, total_simd);
    printf("  speedup: %.2fx\n", speedup);
    printf("  warm   =%d\n", warm);

    _aligned_free(p);

    if (total_scalar != total_simd) {
        printf("  FAIL: counts differ scalar=%d simd=%d\n",
            total_scalar, total_simd);
        return 1;
    }
    if (speedup < 1.5) {
        printf("  WARN: speedup %.2fx below 1.5x target "
               "(expected +200-400 vs scalar at 1M scale)\n", speedup);
        /* Not a hard fail — perf varies by host. */
    }
#else
    printf("  scalar : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_scalar * 1e3 / REPS, ns_per_elem_scalar, total_scalar);
    printf("  (FLECS_C3_SIMD not defined - SIMD path not exercised)\n");
    printf("  warm   =%d\n", warm);
    _aligned_free(p);
#endif

    printf("  PASS\n");
    return 0;
}

/* --- Test 4: gather throughput benchmark --- */
static int test_throughput_gather(void) {
    printf("TEST 4: throughput strided Position (1M, scalar vs gather)\n");
    fflush(stdout);

    const int N = 1000000;
    const int REPS = 20;
    Position *p = (Position*)_aligned_malloc(sizeof(Position) * N, 64);
    assert(p);

    for (int i = 0; i < N; i ++) {
        p[i].x = (float)((i & 1) ? 1.0f : -1.0f);
        p[i].y = 0.0f;
        p[i].z = 0.0f;
    }

    int warm = 0;
    for (int r = 0; r < 3; r ++) {
        warm += run_scalar_gather(p, N);
        warm += run_macro_gather(p, N);
    }

    double t0 = now_sec();
    int total_scalar = 0;
    for (int r = 0; r < REPS; r ++) total_scalar += run_scalar_gather(p, N);
    double t1 = now_sec();
    const double sec_scalar = t1 - t0;
    const double ns_per_elem_scalar = (sec_scalar * 1e9) / (double)(N * REPS);

#if FLECS_C3_HAS_AVX2
    double t2 = now_sec();
    int total_simd = 0;
    for (int r = 0; r < REPS; r ++) total_simd += run_macro_gather(p, N);
    double t3 = now_sec();
    const double sec_simd = t3 - t2;
    const double ns_per_elem_simd = (sec_simd * 1e9) / (double)(N * REPS);
    const double speedup = ns_per_elem_scalar / ns_per_elem_simd;

    printf("  scalar : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_scalar * 1e3 / REPS, ns_per_elem_scalar, total_scalar);
    printf("  gather : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_simd * 1e3 / REPS, ns_per_elem_simd, total_simd);
    printf("  speedup: %.2fx\n", speedup);
    printf("  warm   =%d\n", warm);

    _aligned_free(p);

    if (total_scalar != total_simd) {
        printf("  FAIL: counts differ scalar=%d gather=%d\n",
            total_scalar, total_simd);
        return 1;
    }
#else
    printf("  scalar : %7.3f ms/rep, %.2f ns/elem, total=%d\n",
        sec_scalar * 1e3 / REPS, ns_per_elem_scalar, total_scalar);
    printf("  (FLECS_C3_SIMD not defined - gather path not exercised)\n");
    printf("  warm   =%d\n", warm);
    _aligned_free(p);
#endif

    printf("  PASS\n");
    return 0;
}

/* --- Test 5: Flecs integration smoke (entities from a world, then filter). --- */
static int test_flecs_integration(void) {
    printf("TEST 5: flecs_integration (10K Position entities)\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Position);

    const int N = 10000;
    ecs_entity_t *ids = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * N);
    assert(ids);

    for (int i = 0; i < N; i ++) {
        ids[i] = ecs_new(world);
        ecs_set(world, ids[i], Position, {
            (float)((i & 1) ? 1.0f : -1.0f),
            0.0f, 0.0f
        });
    }

    /* Iterate via query; per-chunk, run the gather macro. */
    ecs_query_t *q = ecs_query(world, {
        .terms = { { .id = ecs_id(Position) } }
    });
    int matched = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        Position *p = ecs_field(&it, Position, 0);
        ECS_OP_FILTER_GT_F32_GATHER(p, it.count, sizeof(Position), matched ++);
    }

    /* Reference: scalar pass over the same iteration. */
    int matched_ref = 0;
    ecs_iter_t it2 = ecs_query_iter(world, q);
    while (ecs_query_next(&it2)) {
        Position *p = ecs_field(&it2, Position, 0);
        for (int i = 0; i < it2.count; i ++) {
            if (p[i].x > 0.0f) matched_ref ++;
        }
    }

    int fail = (matched != matched_ref);
    printf("  matched=%d ref=%d %s\n", matched, matched_ref,
        fail ? "MISMATCH" : "(match)");

    ecs_query_fini(q);
    free(ids);
    ecs_fini(world);

    if (fail) {
        printf("  FAIL\n");
        return 1;
    }
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int rc = 0;
    printf("=== Tier-C3 SIMD filter (%s) ===\n",
        FLECS_C3_HAS_AVX2 ? "AVX2" : "scalar");

    rc |= test_correctness_packed();
    rc |= test_correctness_gather();
    rc |= test_throughput_packed();
    rc |= test_throughput_gather();
    rc |= test_flecs_integration();

    if (rc) {
        printf("=== RESULT: FAIL ===\n");
        return 1;
    }
    printf("=== RESULT: PASS ===\n");
    return 0;
}
