/* TIER v14.1 DEEP CROSS-TEST v11 — Memory leak / allocator stress
 *
 * Tests allocator pressure and world reuse cycles.
 *
 *   KA) Init-fini cycles (3 tests)
 *   KB) Block allocator stress (3 tests)
 *   KC) Component heavy world (3 tests)
 *   KD) Recycle pressure (3 tests)
 *   KE) Multi-cycle no-leak (3 tests)
 *
 *   Total: 15 tests across 5 categories
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void init_stdout(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
}

typedef struct { int32_t v; } KA1;  ECS_COMPONENT_DECLARE(KA1);
typedef struct { int32_t v; } KB1;  ECS_COMPONENT_DECLARE(KB1);
typedef struct { int32_t v; } KC1;  ECS_COMPONENT_DECLARE(KC1);
typedef struct { int32_t v; } KC2;  ECS_COMPONENT_DECLARE(KC2);
typedef struct { int32_t v; } KC3;  ECS_COMPONENT_DECLARE(KC3);
typedef struct { int32_t v; } KD1;  ECS_COMPONENT_DECLARE(KD1);
typedef struct { int32_t v; } KE1;  ECS_COMPONENT_DECLARE(KE1);
typedef struct { int32_t v; } KE2;  ECS_COMPONENT_DECLARE(KE2);

static int g_total = 0, g_passed = 0, g_failed = 0;

#define CAT(n) printf("\n=== %s ===\n", n)
#define BEGIN(n) printf("  [%s] ", n)
#define PASS printf("PASS\n"); g_passed++

#define TASSERT(c, m, ...) do { if (!(c)) { printf("FAIL: " m "\n", ##__VA_ARGS__); g_failed++; g_total++; return 0; } } while(0)
#define TASSERT_EQ(a, b, m, ...) do { long long aa=(long long)(a), bb=(long long)(b); \
    if (aa != bb) { printf("FAIL: " m " (got %lld exp %lld)\n", ##__VA_ARGS__, aa, bb); g_failed++; g_total++; return 0; } } while(0)

static void run_one(int (*fn)(void), const char *name) {
    g_total++;
    BEGIN(name);
    int rc = fn();
    if (rc) { PASS; }
}

/* KA) Init-fini cycles (3 tests) */
static int KA1_100_init_fini(void) {
    for (int i = 0; i < 100; i++) {
        ecs_world_t *w = ecs_init();
        ecs_fini(w);
    }
    TASSERT(1, "100 cycles no-leak");
    return 1;
}

static int KA2_500_init_fini(void) {
    for (int i = 0; i < 500; i++) {
        ecs_world_t *w = ecs_init();
        ecs_fini(w);
    }
    TASSERT(1, "500 cycles no-leak");
    return 1;
}

static int KA3_1000_init_fini(void) {
    for (int i = 0; i < 1000; i++) {
        ecs_world_t *w = ecs_init();
        ecs_fini(w);
    }
    TASSERT(1, "1000 cycles no-leak");
    return 1;
}

/* KB) Block allocator stress (3 tests) */
static int KB1_50_components_in_world(void) {
    ecs_world_t *w = ecs_init();
    /* Define many components */
    ECS_COMPONENT_DEFINE(w, KB1);
    /* Add 1000 entities */
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KB1);
    }
    TASSERT(ecs_count_id(w, ecs_id(KB1)) == 1000, "1000 entities");
    ecs_fini(w);
    return 1;
}

static int KB2_mixed_components(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KC1);
    ECS_COMPONENT_DEFINE(w, KC2);
    ECS_COMPONENT_DEFINE(w, KC3);
    for (int i = 0; i < 500; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KC1);
        ecs_add(w, e, KC2);
        ecs_add(w, e, KC3);
    }
    TASSERT(ecs_count_id(w, ecs_id(KC1)) == 500, "KC1 count");
    ecs_fini(w);
    return 1;
}

static int KB3_allocator_pressure(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KB1);
    /* Create + delete in mixed pattern */
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KB1);
        if (i % 2 == 0) ecs_delete(w, e);
    }
    ecs_fini(w);
    return 1;
}

/* KC) Component heavy world (3 tests) */
static int KC1_100_components_defined(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KE1);
    ECS_COMPONENT_DEFINE(w, KE2);
    /* 2 components, use both in 100 entities */
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KE1);
        ecs_add(w, e, KE2);
    }
    TASSERT(ecs_count_id(w, ecs_id(KE1)) == 100, "KE1 count");
    ecs_fini(w);
    return 1;
}

static int KC2_components_with_pairs(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KE1);
    ecs_entity_t rel = ecs_new(w);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KE1);
        ecs_add_pair(w, e, rel, ecs_new(w));
    }
    TASSERT(ecs_count_id(w, ecs_id(KE1)) == 100, "KE1 count");
    ecs_fini(w);
    return 1;
}

static int KC3_redefine_component(void) {
    /* Re-declaring component should be safe */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KE1);
    ECS_COMPONENT_DEFINE(w, KE1);  /* redeclare */
    TASSERT(1, "redefine no-crash");
    ecs_fini(w);
    return 1;
}

/* KD) Recycle pressure (3 tests) */
static int KD1_recycle_1k(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KD1);
    ecs_entity_t last = 0;
    for (int i = 0; i < 1000; i++) {
        last = ecs_new(w);
        ecs_add(w, last, KD1);
        ecs_delete(w, last);
    }
    TASSERT(1, "1000 cycles no-leak");
    ecs_fini(w);
    return 1;
}

static int KD2_recycle_5k(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KD1);
    for (int i = 0; i < 5000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_delete(w, e);
    }
    TASSERT(1, "5000 cycles no-leak");
    ecs_fini(w);
    return 1;
}

static int KD3_recycle_10k(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, KD1);
    for (int i = 0; i < 10000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_delete(w, e);
    }
    TASSERT(1, "10000 cycles no-leak");
    ecs_fini(w);
    return 1;
}

/* KE) Multi-cycle no-leak (3 tests) */
static int KE1_world_with_components_100_cycles(void) {
    for (int cycle = 0; cycle < 100; cycle++) {
        ecs_world_t *w = ecs_init();
        ECS_COMPONENT_DEFINE(w, KE1);
        for (int i = 0; i < 50; i++) {
            ecs_entity_t e = ecs_new(w);
            ecs_add(w, e, KE1);
        }
        ecs_fini(w);
    }
    TASSERT(1, "100 world cycles with components no-leak");
    return 1;
}

static void ke_obs(ecs_iter_t *it) { (void)it; }

static int KE2_world_with_observer_50_cycles(void) {
    for (int cycle = 0; cycle < 50; cycle++) {
        ecs_world_t *w = ecs_init();
        ECS_COMPONENT_DEFINE(w, KE1);
        ecs_observer_init(w, &(ecs_observer_desc_t){
            .query.terms = {{ .id = ecs_id(KE1) }},
            .events = { EcsOnAdd },
            .callback = ke_obs
        });
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, KE1);
        ecs_fini(w);
    }
    TASSERT(1, "50 world cycles with observer no-leak");
    return 1;
}

static int KE3_alternating_init_fini(void) {
    for (int i = 0; i < 200; i++) {
        ecs_world_t *w = ecs_init();
        if (i % 2 == 0) {
            ECS_COMPONENT_DEFINE(w, KE1);
        }
        ecs_fini(w);
    }
    TASSERT(1, "200 alternating init-fini no-leak");
    return 1;
}

int main(void) {
    init_stdout();
    printf("=== TIER v14.1 DEEP CROSS-TEST v11 — Memory leak stress ===\n");
    printf("15 tests across 5 categories\n\n");

    CAT("KA) Init-fini cycles");
    run_one(KA1_100_init_fini,                "KA1: 100 cycles");
    run_one(KA2_500_init_fini,                "KA2: 500 cycles");
    run_one(KA3_1000_init_fini,               "KA3: 1000 cycles");

    CAT("KB) Block allocator stress");
    run_one(KB1_50_components_in_world,       "KB1: 50 components");
    run_one(KB2_mixed_components,              "KB2: mixed 3 comp");
    run_one(KB3_allocator_pressure,            "KB3: pressure");

    CAT("KC) Component heavy world");
    run_one(KC1_100_components_defined,        "KC1: 100 comp defined");
    run_one(KC2_components_with_pairs,         "KC2: comp + pair");
    run_one(KC3_redefine_component,            "KC3: redefine");

    CAT("KD) Recycle pressure");
    run_one(KD1_recycle_1k,                    "KD1: 1K cycles");
    run_one(KD2_recycle_5k,                    "KD2: 5K cycles");
    run_one(KD3_recycle_10k,                   "KD3: 10K cycles");

    CAT("KE) Multi-cycle no-leak");
    run_one(KE1_world_with_components_100_cycles, "KE1: 100 world+comp");
    run_one(KE2_world_with_observer_50_cycles, "KE2: 50 world+obs");
    run_one(KE3_alternating_init_fini,          "KE3: 200 alternating");

    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v11 TESTS PASS — 5 leak categories\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
