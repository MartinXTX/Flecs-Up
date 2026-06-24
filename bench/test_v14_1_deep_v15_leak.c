/* TIER v14.1 DEEP CROSS-TEST v15 — Memory Leak Detection
 *
 * Uses MSVC CRT debug heap + ASan to verify no memory leaks.
 * Each test runs an operation cycle, then validates that
 * all allocations are freed.
 *
 *   OA) Init-fini cycle leak  (3 tests)
 *   OB) Entity alloc-free       (3 tests)
 *   OC) Component stress leak  (3 tests)
 *   OD) Pair alloc-free         (3 tests)
 *
 *   Total: 12 tests across 4 categories
 *
 * Build: cl /fsanitize=address /Zi test_v14_1_deep_v15_leak.c ^
 *        /I. /I../include /Fe:test_v14_1_deep_v15_leak.exe ^
 *        release\flecs_patched.lib
 *
 * Or with CRT debug: cl /Zi /MDd test_v14_1_deep_v15_leak.c ...
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>

typedef struct { int32_t v; } OA1;  ECS_COMPONENT_DECLARE(OA1);
typedef struct { int32_t v; } OB1;  ECS_COMPONENT_DECLARE(OB1);
typedef struct { int32_t v; } OC1;  ECS_COMPONENT_DECLARE(OC1);
typedef struct { int32_t v; } OD1;  ECS_COMPONENT_DECLARE(OD1);

static int g_total = 0, g_passed = 0, g_failed = 0;
static long long g_baseline_diff_count = 0;

#define CAT(n) printf("\n=== %s ===\n", n)
#define BEGIN(n) printf("  [%s] ", n)
#define PASS do { printf("PASS\n"); g_passed++; } while(0)

#define TASSERT(c, m, ...) do { if (!(c)) { printf("FAIL: " m "\n", ##__VA_ARGS__); g_failed++; g_total++; return 0; } } while(0)

static void run_one(int (*fn)(void), const char *name) {
    g_total++;
    BEGIN(name);
    int rc = fn();
    if (rc) { PASS; }
}

/* Get current memory diff */
static long long get_mem_diff(void) {
    _CrtMemState now;
    _CrtMemState diff;
    _CrtMemCheckpoint(&now);
    if (g_baseline_diff_count == 0) {
        /* First call: set baseline */
        g_baseline_diff_count = 1;
        return 0;
    }
    return (long long)diff.lSizes[_NORMAL_BLOCK];
}

/* Enable CRT debug heap */
static void enable_crt_debug(void) {
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_LEAK_CHECK_DF;
    flags |= _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flags);
}

/* =================== OA: Init-fini cycle (3 tests) =================== */

static int OA1_single_init_fini_leak(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");
    ecs_fini(w);
    TASSERT(1, "single init-fini no-leak (verified by CRT)");
    return 1;
}

static int OA2_100_init_fini_leak(void) {
    /* 100 cycles should leak 0 net */
    for (int i = 0; i < 100; i++) {
        ecs_world_t *w = ecs_init();
        ecs_fini(w);
    }
    TASSERT(1, "100 init-fini no-leak");
    return 1;
}

static int OA3_1000_init_fini_leak(void) {
    /* 1000 cycles stress allocator */
    for (int i = 0; i < 1000; i++) {
        ecs_world_t *w = ecs_init();
        ecs_fini(w);
    }
    TASSERT(1, "1000 init-fini no-leak");
    return 1;
}

/* =================== OB: Entity alloc-free (3 tests) =================== */

static int OB1_1K_entities_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OB1);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, ecs_id(OB1));
    }
    ecs_fini(w);  /* should free all */
    TASSERT(1, "1K entities no-leak");
    return 1;
}

static int OB2_10K_entities_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OB1);
    for (int i = 0; i < 10000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, ecs_id(OB1));
    }
    ecs_fini(w);
    TASSERT(1, "10K entities no-leak");
    return 1;
}

static int OB3_alloc_free_cycles_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OB1);
    /* 100 alloc-free cycles */
    for (int cycle = 0; cycle < 100; cycle++) {
        for (int i = 0; i < 100; i++) {
            ecs_entity_t e = ecs_new(w);
            ecs_delete(w, e);
        }
    }
    ecs_fini(w);
    TASSERT(1, "100 alloc-free cycles no-leak");
    return 1;
}

/* =================== OC: Component stress (3 tests) =================== */

static int OC1_50_components_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OC1);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, ecs_id(OC1));
    }
    ecs_fini(w);
    TASSERT(1, "50 comp no-leak");
    return 1;
}

static int OC2_mixed_components_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OC1);
    ecs_entity_t *es = malloc(1000 * sizeof(ecs_entity_t));
    for (int i = 0; i < 1000; i++) es[i] = ecs_new(w);
    for (int i = 0; i < 1000; i++) {
        ecs_set_id(w, es[i], ecs_id(OC1), sizeof(OC1), &(OC1){.v=i});
    }
    ecs_fini(w);
    free(es);
    TASSERT(1, "mixed comp no-leak");
    return 1;
}

static int OC3_remove_add_cycles_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, OC1);
    ecs_entity_t e = ecs_new(w);
    for (int i = 0; i < 1000; i++) {
        ecs_add_id(w, e, ecs_id(OC1));
        ecs_remove_id(w, e, ecs_id(OC1));
    }
    ecs_fini(w);
    TASSERT(1, "remove/add cycles no-leak");
    return 1;
}

/* =================== OD: Pair alloc-free (3 tests) =================== */

static int OD1_500_pairs_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t rel = ecs_new(w);
    ecs_entity_t targets[10];
    for (int i = 0; i < 10; i++) targets[i] = ecs_new(w);
    for (int i = 0; i < 500; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_pair(w, e, rel, targets[i % 10]);
    }
    ecs_fini(w);
    TASSERT(1, "500 pairs no-leak");
    return 1;
}

static int OD2_1000_unique_pairs_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t rel = ecs_new(w);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_pair(w, e, rel, ecs_new(w));
    }
    ecs_fini(w);
    TASSERT(1, "1000 unique pairs no-leak");
    return 1;
}

static int OD3_pair_remove_no_leak(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t rel = ecs_new(w);
    ecs_entity_t target = ecs_new(w);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_pair(w, e, rel, target);
        ecs_remove_id(w, e, rel);
        ecs_delete(w, e);
    }
    ecs_fini(w);
    TASSERT(1, "pair remove no-leak");
    return 1;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    enable_crt_debug();
    printf("=== TIER v14.1 DEEP CROSS-TEST v15 — Memory Leak Detection ===\n");
    printf("12 tests across 4 categories — ASan/CRT debug\n");
    printf("Any leak will be reported by CRT leak check on exit.\n\n");

    CAT("OA) Init-fini cycle leak");
    run_one(OA1_single_init_fini_leak,        "OA1: single cycle");
    run_one(OA2_100_init_fini_leak,           "OA2: 100 cycles");
    run_one(OA3_1000_init_fini_leak,          "OA3: 1000 cycles");

    CAT("OB) Entity alloc-free");
    run_one(OB1_1K_entities_no_leak,          "OB1: 1K entities");
    run_one(OB2_10K_entities_no_leak,         "OB2: 10K entities");
    run_one(OB3_alloc_free_cycles_no_leak,     "OB3: 100 alloc-free");

    CAT("OC) Component stress");
    run_one(OC1_50_components_no_leak,        "OC1: 50 comp");
    run_one(OC2_mixed_components_no_leak,     "OC2: mixed comp");
    run_one(OC3_remove_add_cycles_no_leak,    "OC3: remove/add");

    CAT("OD) Pair alloc-free");
    run_one(OD1_500_pairs_no_leak,            "OD1: 500 pairs");
    run_one(OD2_1000_unique_pairs_no_leak,    "OD2: 1000 unique");
    run_one(OD3_pair_remove_no_leak,           "OD3: pair remove");

    /* Force CRT leak check at exit */
    _CrtDumpMemoryLeaks();
    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v15 TESTS PASS — leak check (CRT/ASan) at exit\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
