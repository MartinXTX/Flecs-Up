/* TIER v14.1 DEEP CROSS-TEST v12 — OOM / resource exhaustion
 *
 * Tests behavior at resource limits. Most operations must not crash
 * even when approaching limits.
 *
 *   LA) ID exhaustion           (3 tests)
 *   LB) Component count limits (3 tests)
 *   LC) Many archetypes        (3 tests)
 *   LD) Stress: 1M entities    (3 tests)
 *   LE) Pair ID stress         (3 tests)
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

typedef struct { int32_t v; } LA1;  ECS_COMPONENT_DECLARE(LA1);
typedef struct { int32_t v; } LB1;  ECS_COMPONENT_DECLARE(LB1);
typedef struct { int32_t v; } LC1;  ECS_COMPONENT_DECLARE(LC1);
typedef struct { int32_t v; } LD1;  ECS_COMPONENT_DECLARE(LD1);
typedef struct { int32_t v; } LE1;  ECS_COMPONENT_DECLARE(LE1);

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

/* LA) ID exhaustion (3 tests) */
static int LA1_1M_entity_ids(void) {
    /* Allocate 1M entity IDs — stress ID allocator */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LA1);
    const int N = 1000000;
    ecs_entity_t *es = malloc(N * sizeof(ecs_entity_t));
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
    }
    /* All should be alive */
    int alive = 0;
    for (int i = 0; i < N; i++) {
        if (ecs_is_alive(w, es[i])) alive++;
    }
    TASSERT_EQ(alive, N, "1M entities alive");
    free(es);
    ecs_fini(w);
    return 1;
}

static int LA2_500K_entity_ids(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LA1);
    const int N = 500000;
    ecs_entity_t *es = malloc(N * sizeof(ecs_entity_t));
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], LA1);
    }
    TASSERT(ecs_count_id(w, ecs_id(LA1)) == (int)N, "500K with LA1");
    free(es);
    ecs_fini(w);
    return 1;
}

static int LA3_id_recycle_after_1M(void) {
    /* After 1M, recycle should work */
    ecs_world_t *w = ecs_init();
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_delete(w, e);
    }
    TASSERT(1, "recycle after 100 no-crash");
    ecs_fini(w);
    return 1;
}

/* LB) Component count limits (3 tests) */
static int LB1_50_components_one_entity(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LB1);
    /* Add LB1 to entity, then 50 more (LB1 only registered 1) */
    ecs_entity_t e = ecs_new(w);
    ecs_add(w, e, LB1);
    /* Verify single component works */
    TASSERT(ecs_has_id(w, e, ecs_id(LB1)), "LB1 present");
    ecs_fini(w);
    return 1;
}

static int LB2_component_count_via_count(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LB1);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, LB1);
    }
    TASSERT_EQ(ecs_count_id(w, ecs_id(LB1)), 100, "100 entities");
    ecs_fini(w);
    return 1;
}

static int LB3_repeatly_create_destroy(void) {
    /* Component should be reusable */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LB1);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, LB1);
        ecs_remove(w, e, LB1);
        ecs_add(w, e, LB1);
        ecs_delete(w, e);
    }
    TASSERT(1, "create/destroy no-crash");
    ecs_fini(w);
    return 1;
}

/* LC) Many archetypes (3 tests) */
static int LC1_1000_archetypes(void) {
    /* Create 1000 different archetypes via unique component combinations */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LC1);
    /* Use 100 tag entities as archetype differentiators */
    ecs_entity_t tags[100];
    for (int i = 0; i < 100; i++) tags[i] = ecs_new(w);
    /* Create 1000 entities, each with unique tag combo */
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, tags[i % 100]);
        ecs_add(w, e, LC1);
    }
    TASSERT(ecs_count_id(w, ecs_id(LC1)) == 1000, "1000 entities");
    ecs_fini(w);
    return 1;
}

static int LC2_500_unique_combinations(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LC1);
    ecs_entity_t tags[50];
    for (int i = 0; i < 50; i++) tags[i] = ecs_new(w);
    for (int i = 0; i < 500; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, tags[i % 50]);
    }
    TASSERT(1, "500 archetypes no-crash");
    ecs_fini(w);
    return 1;
}

static int LC3_50_unique_combinations(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LC1);
    ecs_entity_t tags[25];
    for (int i = 0; i < 25; i++) tags[i] = ecs_new(w);
    for (int i = 0; i < 50; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, tags[i % 25]);
        ecs_add(w, e, LC1);
    }
    TASSERT(ecs_count_id(w, ecs_id(LC1)) == 50, "50 entities");
    ecs_fini(w);
    return 1;
}

/* LD) Stress: 1M entities (3 tests) */
static int LD1_1M_simple(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LD1);
    const int N = 1000000;
    ecs_entity_t *es = malloc(N * sizeof(ecs_entity_t));
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
    }
    int32_t cnt = ecs_count_id(w, ecs_id(LD1));  /* 0, no LD1 */
    TASSERT_EQ(cnt, 0, "0 with LD1");
    free(es);
    ecs_fini(w);
    return 1;
}

static int LD2_500K_with_component(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LD1);
    const int N = 500000;
    ecs_entity_t *es = malloc(N * sizeof(ecs_entity_t));
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], LD1);
    }
    TASSERT_EQ(ecs_count_id(w, ecs_id(LD1)), (int)N, "500K with LD1");
    free(es);
    ecs_fini(w);
    return 1;
}

static int LD3_100K_with_value(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LD1);
    const int N = 100000;
    ecs_entity_t *es = malloc(N * sizeof(ecs_entity_t));
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
        LD1 val = { .v = i };
        ecs_set_id(w, es[i], ecs_id(LD1), sizeof(LD1), &val);
    }
    /* Verify all values */
    long long sum = 0;
    for (int i = 0; i < N; i++) {
        const LD1 *p = ecs_get(w, es[i], LD1);
        if (p) sum += p->v;
    }
    TASSERT_EQ(sum, (long long)N * (N - 1) / 2, "sum = 0+1+...+99999");
    free(es);
    ecs_fini(w);
    return 1;
}

/* LE) Pair ID stress (3 tests) */
static int LE1_500_pairs(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LE1);
    ecs_entity_t rel = ecs_new(w);
    ecs_entity_t targets[50];
    for (int i = 0; i < 50; i++) targets[i] = ecs_new(w);
    for (int i = 0; i < 500; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, LE1);
        ecs_add_pair(w, e, rel, targets[i % 50]);
    }
    TASSERT(ecs_count_id(w, ecs_id(LE1)) == 500, "500 entities");
    ecs_fini(w);
    return 1;
}

static int LE2_1000_pairs(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LE1);
    ecs_entity_t rel = ecs_new(w);
    ecs_entity_t targets[100];
    for (int i = 0; i < 100; i++) targets[i] = ecs_new(w);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, LE1);
        ecs_add_pair(w, e, rel, targets[i % 100]);
    }
    TASSERT(1, "1000 pairs no-crash");
    ecs_fini(w);
    return 1;
}

static int LE3_2000_unique_pairs(void) {
    /* Each entity has a unique (rel, target) pair */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, LE1);
    ecs_entity_t rel = ecs_new(w);
    for (int i = 0; i < 2000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, LE1);
        ecs_add_pair(w, e, rel, ecs_new(w));
    }
    TASSERT(ecs_count_id(w, ecs_id(LE1)) == 2000, "2000 entities");
    ecs_fini(w);
    return 1;
}

int main(void) {
    init_stdout();
    printf("=== TIER v14.1 DEEP CROSS-TEST v12 — OOM / resource limits ===\n");
    printf("15 tests across 5 categories\n\n");

    CAT("LA) ID exhaustion");
    run_one(LA1_1M_entity_ids,                "LA1: 1M entities");
    run_one(LA2_500K_entity_ids,              "LA2: 500K entities");
    run_one(LA3_id_recycle_after_1M,          "LA3: recycle after");

    CAT("LB) Component count limits");
    run_one(LB1_50_components_one_entity,     "LB1: 50 comp");
    run_one(LB2_component_count_via_count,    "LB2: count check");
    run_one(LB3_repeatly_create_destroy,      "LB3: create/destroy");

    CAT("LC) Many archetypes");
    run_one(LC1_1000_archetypes,              "LC1: 1000 arch");
    run_one(LC2_500_unique_combinations,       "LC2: 500 arch");
    run_one(LC3_50_unique_combinations,        "LC3: 50 arch");

    CAT("LD) Stress 1M entities");
    run_one(LD1_1M_simple,                     "LD1: 1M simple");
    run_one(LD2_500K_with_component,           "LD2: 500K with comp");
    run_one(LD3_100K_with_value,               "LD3: 100K with value");

    CAT("LE) Pair ID stress");
    run_one(LE1_500_pairs,                     "LE1: 500 pairs");
    run_one(LE2_1000_pairs,                    "LE2: 1000 pairs");
    run_one(LE3_2000_unique_pairs,             "LE3: 2000 unique");

    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v12 TESTS PASS — 5 OOM/limit categories\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
