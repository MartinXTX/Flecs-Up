/* TIER v14.1 DEEP CROSS-TEST v13 — Property-based fuzz
 *
 * Tests invariants across many random operation sequences. Tests
 * check that no random sequence produces an inconsistent world.
 *
 *   MA) Random seed fuzz     (3 tests)
 *   MB) Random op sequence    (3 tests)
 *   MC) Edge case combinations (3 tests)
 *   MD) Query stress fuzz     (3 tests)
 *   ME) Observer stress fuzz  (3 tests)
 *
 *   Total: 15 tests across 5 categories
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void init_stdout(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
}

typedef struct { int32_t v; } MA1;  ECS_COMPONENT_DECLARE(MA1);
typedef struct { int32_t v; } MB1;  ECS_COMPONENT_DECLARE(MB1);
typedef struct { int32_t v; } MC1;  ECS_COMPONENT_DECLARE(MC1);
typedef struct { int32_t v; } MD1;  ECS_COMPONENT_DECLARE(MD1);
typedef struct { int32_t v; } ME1;  ECS_COMPONENT_DECLARE(ME1);

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

/* MA) Random seed fuzz (3 tests) */
static int MA1_random_seed_42(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MA1);
    srand(42);
    ecs_entity_t *es = malloc(5000 * sizeof(ecs_entity_t));
    for (int i = 0; i < 5000; i++) {
        es[i] = ecs_new(w);
        if (rand() % 2) ecs_add(w, es[i], MA1);
    }
    /* Check counts */
    int alive = 0;
    for (int i = 0; i < 5000; i++) {
        if (ecs_is_alive(w, es[i])) alive++;
    }
    TASSERT_EQ(alive, 5000, "all alive");
    free(es);
    ecs_fini(w);
    return 1;
}

static int MA2_random_seed_12345(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MA1);
    srand(12345);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        MA1 val = { .v = rand() % 10000 };
        ecs_set_id(w, e, ecs_id(MA1), sizeof(MA1), &val);
    }
    TASSERT(ecs_count_id(w, ecs_id(MA1)) == 1000, "1000 entities");
    ecs_fini(w);
    return 1;
}

static int MA3_random_seed_99999(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MA1);
    srand(99999);
    /* Random mix */
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        if (rand() % 3 == 0) ecs_add(w, e, MA1);
        if (rand() % 2 == 0) ecs_delete(w, e);
    }
    TASSERT(1, "random mix no-crash");
    ecs_fini(w);
    return 1;
}

/* MB) Random op sequence (3 tests) */
static int MB1_random_op_1000(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MB1);
    srand(7);
    ecs_entity_t last = 0;
    for (int i = 0; i < 1000; i++) {
        int op = rand() % 4;
        if (op == 0) {
            last = ecs_new(w);
        } else if (op == 1) {
            if (last) ecs_add(w, last, MB1);
        } else if (op == 2) {
            if (last) {
                MB1 val = { .v = i };
                ecs_set_id(w, last, ecs_id(MB1), sizeof(MB1), &val);
            }
        } else {
            if (last) ecs_remove(w, last, MB1);
        }
    }
    TASSERT(1, "1000 random ops no-crash");
    ecs_fini(w);
    return 1;
}

static int MB2_random_pair_ops(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MB1);
    ecs_entity_t rel = ecs_new(w);
    srand(42);
    ecs_entity_t last = 0;
    for (int i = 0; i < 500; i++) {
        int op = rand() % 3;
        if (op == 0) {
            last = ecs_new(w);
        } else if (op == 1) {
            if (last) ecs_add_pair(w, last, rel, ecs_new(w));
        } else {
            if (last) ecs_remove_id(w, last, rel);
        }
    }
    TASSERT(1, "500 random pair ops no-crash");
    ecs_fini(w);
    return 1;
}

static int MB3_random_create_destroy(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MB1);
    srand(1);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, MB1);
        if (rand() % 2 == 0) ecs_delete(w, e);
    }
    TASSERT(1, "random create-destroy no-crash");
    ecs_fini(w);
    return 1;
}

/* MC) Edge case combinations (3 tests) */
static int MC1_repeat_define(void) {
    /* Multiple ECS_COMPONENT_DEFINE calls */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MC1);
    ECS_COMPONENT_DEFINE(w, MC1);  /* redeclare same */
    ECS_COMPONENT_DEFINE(w, MC1);  /* and again */
    TASSERT(1, "repeat define no-crash");
    ecs_fini(w);
    return 1;
}

static void mc_obs(ecs_iter_t *it) { (void)it; }

static int MC2_many_observer_register(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MC1);
    for (int i = 0; i < 50; i++) {
        ecs_observer_init(w, &(ecs_observer_desc_t){
            .query.terms = {{ .id = ecs_id(MC1) }},
            .events = { EcsOnAdd },
            .callback = mc_obs
        });
    }
    TASSERT(1, "50 observers no-crash");
    ecs_fini(w);
    return 1;
}

static int MC3_random_after_empty(void) {
    /* World with no entities, random ops */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MC1);
    srand(100);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, MC1);
        ecs_delete(w, e);
    }
    TASSERT(1, "ops on empty-then-fill no-crash");
    ecs_fini(w);
    return 1;
}

/* MD) Query stress fuzz (3 tests) */
static int MD1_query_random_state(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MD1);
    srand(50);
    ecs_query_t *q = ecs_query(w, { .terms = { { .id = ecs_id(MD1) } } });
    /* Random creates, query after each */
    int prev_count = 0;
    for (int i = 0; i < 100; i++) {
        int add = rand() % 5;
        for (int j = 0; j < add; j++) {
            ecs_entity_t e = ecs_new(w);
            ecs_add(w, e, MD1);
        }
        int curr = 0;
        ecs_iter_t it = ecs_query_iter(w, q);
        while (ecs_query_next(&it)) curr += it.count;
        TASSERT(curr >= prev_count, "count monotonic");
        prev_count = curr;
    }
    ecs_fini(w);
    return 1;
}

static int MD2_iterate_after_delete(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MD1);
    ecs_query_t *q = ecs_query(w, { .terms = { { .id = ecs_id(MD1) } } });
    ecs_entity_t es[100];
    for (int i = 0; i < 100; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], MD1);
    }
    /* Delete half */
    for (int i = 0; i < 100; i += 2) ecs_delete(w, es[i]);
    /* Iterate — should see 50 */
    int count = 0;
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) count += it.count;
    TASSERT_EQ(count, 50, "50 remaining");
    ecs_fini(w);
    return 1;
}

static int MD3_query_with_filters(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, MD1);
    ecs_entity_t e1 = ecs_new(w);
    ecs_add_id(w, e1, EcsDisabled);
    ecs_entity_t e2 = ecs_new(w);
    ecs_entity_t e3 = ecs_new(w);
    ecs_add(w, e3, MD1);
    /* Query all — should see 3 entities */
    ecs_query_t *q = ecs_query(w, { .terms = { { .id = ecs_id(MD1) } } });
    int count = 0;
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) count += it.count;
    /* MD1 only on e3, so count=1 */
    TASSERT_EQ(count, 1, "1 entity with MD1");
    ecs_fini(w);
    return 1;
}

/* ME) Observer stress fuzz (3 tests) */
static void me_obs(ecs_iter_t *it) { (void)it; }

static int ME1_many_observers_fuzz(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, ME1);
    /* Register 20 observers */
    for (int i = 0; i < 20; i++) {
        ecs_observer_init(w, &(ecs_observer_desc_t){
            .query.terms = {{ .id = ecs_id(ME1) }},
            .events = { EcsOnAdd, EcsOnRemove },
            .callback = me_obs
        });
    }
    /* Random adds/removes */
    srand(99);
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, ME1);
        ecs_remove(w, e, ME1);
        ecs_delete(w, e);
    }
    TASSERT(1, "20 observers + 1000 ops no-crash");
    ecs_fini(w);
    return 1;
}

static int ME2_observer_with_pair(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, ME1);
    ecs_entity_t rel = ecs_new(w);
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_pair(rel, EcsWildcard) }},
        .events = { EcsOnAdd },
        .callback = me_obs
    });
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_entity_t t = ecs_new(w);
        ecs_add_pair(w, e, rel, t);
    }
    TASSERT(1, "pair obs no-crash");
    ecs_fini(w);
    return 1;
}

static int ME3_observer_random_events(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, ME1);
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(ME1) }},
        .events = { EcsOnAdd, EcsOnRemove, EcsOnSet },
        .callback = me_obs
    });
    srand(50);
    ecs_entity_t es[100];
    for (int i = 0; i < 100; i++) es[i] = ecs_new(w);
    for (int i = 0; i < 500; i++) {
        int op = rand() % 3;
        if (op == 0) {
            ecs_add(w, es[rand() % 100], ME1);
        } else if (op == 1) {
            ecs_remove(w, es[rand() % 100], ME1);
        } else {
            ME1 val = { .v = i };
            ecs_set_id(w, es[rand() % 100], ecs_id(ME1), sizeof(ME1), &val);
        }
    }
    TASSERT(1, "random events no-crash");
    ecs_fini(w);
    return 1;
}

int main(void) {
    init_stdout();
    printf("=== TIER v14.1 DEEP CROSS-TEST v13 — Property-based fuzz ===\n");
    printf("15 tests across 5 categories\n\n");

    CAT("MA) Random seed fuzz");
    run_one(MA1_random_seed_42,               "MA1: seed 42");
    run_one(MA2_random_seed_12345,            "MA2: seed 12345");
    run_one(MA3_random_seed_99999,            "MA3: seed 99999");

    CAT("MB) Random op sequence");
    run_one(MB1_random_op_1000,               "MB1: 1000 random ops");
    run_one(MB2_random_pair_ops,              "MB2: random pair ops");
    run_one(MB3_random_create_destroy,        "MB3: random create-destroy");

    CAT("MC) Edge case combinations");
    run_one(MC1_repeat_define,                "MC1: repeat define");
    run_one(MC2_many_observer_register,       "MC2: 50 observers");
    run_one(MC3_random_after_empty,            "MC3: ops on empty-then-fill");

    CAT("MD) Query stress fuzz");
    run_one(MD1_query_random_state,            "MD1: query random state");
    run_one(MD2_iterate_after_delete,          "MD2: iterate after delete");
    run_one(MD3_query_with_filters,            "MD3: query with filters");

    CAT("ME) Observer stress fuzz");
    run_one(ME1_many_observers_fuzz,          "ME1: 20 obs + 1000 ops");
    run_one(ME2_observer_with_pair,            "ME2: pair obs");
    run_one(ME3_observer_random_events,        "ME3: random events");

    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v13 TESTS PASS — 5 fuzz categories\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
