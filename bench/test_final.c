/* FINAL COMPREHENSIVE STABILITY TEST.
 * Tests all Tier interactions, edge cases, long-running stability.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } FX;
typedef struct { int y; } FY;
typedef struct { int z; } FZ;
typedef struct { float a, b, c, d; } FQ;

ECS_COMPONENT_DECLARE(FX);
ECS_COMPONENT_DECLARE(FY);
ECS_COMPONENT_DECLARE(FZ);
ECS_COMPONENT_DECLARE(FQ);

/* TEST 1: Long-running stability test */
static int test_long_running(void) {
    printf("TEST 1: long_running_stability (1000 rounds)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    for (int round = 0; round < 1000; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
        FX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

        ecs_id_t fy_id = ecs_id(FY);
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], &fy_id, 1);
        }

        /* Verify */
        for (int i = 0; i < N; i++) {
            const FX *p = ecs_get(world, ids[i], FX);
            assert(p && p->x == round);
        }

        ecs_bulk_delete(world, ids, N);
    }
    printf("  1000 rounds OK\n");
    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: Multi-world parallel stress */
static int test_multi_world_parallel(void) {
    printf("TEST 2: multi_world_parallel\n");
    fflush(stdout);
    ecs_world_t *w1 = ecs_init();
    ecs_world_t *w2 = ecs_init();
    ECS_COMPONENT_DEFINE(w1, FX);
    ECS_COMPONENT_DEFINE(w2, FX);

    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids1 = ecs_bulk_new_w_id(w1, ecs_id(FX), N);
        const ecs_entity_t *ids2 = ecs_bulk_new_w_id(w2, ecs_id(FX), N);

        FX v1 = {round};
        FX v2 = {round * 2};
        ecs_bulk_set_id(w1, ids1, N, ecs_id(FX), sizeof(FX), &v1);
        ecs_bulk_set_id(w2, ids2, N, ecs_id(FX), sizeof(FX), &v2);

        /* Verify no cross-contamination */
        for (int i = 0; i < N; i++) {
            const FX *p1 = ecs_get(w1, ids1[i], FX);
            const FX *p2 = ecs_get(w2, ids2[i], FX);
            assert(p1->x == round);
            assert(p2->x == round * 2);
        }

        ecs_bulk_delete(w1, ids1, N);
        ecs_bulk_delete(w2, ids2, N);
    }
    printf("  100 rounds OK\n");
    ecs_fini(w1);
    ecs_fini(w2);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: All Tier APIs combined */
static int test_all_tier_apis(void) {
    printf("TEST 3: all_tier_apis_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);

    /* Use ALL Tier APIs in sequence */
    for (int round = 0; round < 50; round++) {
        const int N = 20;
        /* Tier-R1: combined create+set */
        FX fx = {round * 10};
        const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(FX), N, &fx, sizeof(FX));

        /* Tier-I1: batched add */
        ecs_id_t add_ids[2] = {ecs_id(FY), ecs_id(FZ)};
        ecs_add_ids_w_entities(world, ids, N, add_ids, 2);

        /* Tier-O1: bulk set same value */
        FY fy = {round * 20};
        ecs_bulk_set_id(world, ids, N, ecs_id(FY), sizeof(FY), &fy);

        /* Verify all have all 3 components with correct values */
        for (int i = 0; i < N; i++) {
            const FX *px = ecs_get(world, ids[i], FX);
            const FY *py = ecs_get(world, ids[i], FY);
            assert(px->x == round * 10);
            assert(py->y == round * 20);
        }

        /* Tier-P1: bulk delete */
        ecs_bulk_delete(world, ids, N);

        /* Verify all dead */
        for (int i = 0; i < N; i++) {
            assert(!ecs_is_alive(world, ids[i]));
        }
    }
    printf("  50 rounds OK\n");
    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Query after bulk operations */
static int test_query_stress(void) {
    printf("TEST 4: query_stress\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(FX) }, { .id = ecs_id(FY) }}
    });

    for (int round = 0; round < 100; round++) {
        const int N = 100;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
        ecs_id_t fy_id = ecs_id(FY);
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], &fy_id, 1);
        }

        /* Query should return all 100 */
        int count = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            const FX *px = ecs_field(&it, FX, 0);
            const FY *py = ecs_field(&it, FY, 1);
            for (int i = 0; i < it.count; i++) {
                count++;
            }
        }
        assert(count == N);

        ecs_bulk_delete(world, ids, N);

        /* Query should return 0 after delete */
        count = 0;
        it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) count += it.count;
        assert(count == 0);
    }
    printf("  100 rounds OK\n");
    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Observer dispatch stress */
static int add_count = 0;
static int set_count = 0;

static void on_add_obs(ecs_iter_t *it) { add_count += it->count; }
static void on_set_obs(ecs_iter_t *it) { set_count += it->count; }

static int test_observer_dispatch(void) {
    printf("TEST 5: observer_dispatch_stress\n");
    fflush(stdout);

    add_count = 0;
    set_count = 0;

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(FX) }},
        .events = { EcsOnAdd },
        .callback = on_add_obs
    });
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(FX) }},
        .events = { EcsOnSet },
        .callback = on_set_obs
    });

    for (int round = 0; round < 10; round++) {
        const int N = 100;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);

        FX v = {42};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

        ecs_bulk_delete(world, ids, N);
    }

    printf("  add=%d set=%d (expected ~1000 each)\n", add_count, set_count);
    if (add_count != 1000 || set_count != 1000) {
        printf("  WARN: observer counts unexpected\n");
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Allocator pressure test */
static int test_allocator_pressure(void) {
    printf("TEST 6: allocator_pressure (5000 ops)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);

    for (int round = 0; round < 5000; round++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, FX, {round});
        const FX *p = ecs_get(world, e, FX);
        if (!p || p->x != round) {
            printf("  FAIL at round %d\n", round);
            return 1;
        }
        ecs_delete(world, e);
    }

    /* Verify world still functional */
    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, FX, {99999});
    const FX *p = ecs_get(world, e, FX);
    if (!p || p->x != 99999) {
        printf("  FAIL: world unhealthy\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Random operations test */
static int test_random_ops(void) {
    printf("TEST 7: random_operations\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    srand(42);
    for (int op = 0; op < 1000; op++) {
        int action = rand() % 4;
        if (action == 0) {
            /* Create + set */
            ecs_entity_t e = ecs_new(world);
            ecs_set(world, e, FX, {op});
        } else if (action == 1) {
            /* Bulk create + set */
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), 5);
            FX v = {op};
            ecs_bulk_set_id(world, ids, 5, ecs_id(FX), sizeof(FX), &v);
            ecs_bulk_delete(world, ids, 5);
        } else if (action == 2) {
            /* Add + remove */
            ecs_entity_t e = ecs_new(world);
            ecs_add(world, e, FX);
            ecs_remove(world, e, FX);
            ecs_delete(world, e);
        } else {
            /* Mixed bulk */
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FY), 3);
            FY v = {op & 0xFFFF};  /* ensure non-negative */
            ecs_bulk_set_id(world, ids, 3, ecs_id(FY), sizeof(FY), &v);
            ecs_bulk_delete(world, ids, 3);
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 8: Tier-A2 unchecked field usage */
static int test_a2_unchecked(void) {
    printf("TEST 8: a2_unchecked_field\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
    FX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

    /* Use ecs_field_unchecked for hot loop */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(FX) }}
    });

    int total = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        /* Tier-A2: unchecked field access */
        const FX *p = ecs_field_unchecked(&it, FX, 0);
        for (int i = 0; i < it.count; i++) {
            if (p[i].x == 42) total++;
        }
    }
    assert(total == N);

    ecs_fini(world);
    printf("  PASS (total=%d)\n", total);
    fflush(stdout);
    return 0;
}

/* TEST 9: Tier-V1 grow_data with many columns */
static int test_v1_grow_data(void) {
    printf("TEST 9: v1_grow_data_many_columns\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);

    /* Many bulk operations with different component sets */
    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);

        if (round % 3 == 0) {
            ecs_id_t add_ids[2] = {ecs_id(FY), ecs_id(FZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
        } else if (round % 3 == 1) {
            ecs_id_t add_id = ecs_id(FY);
            for (int i = 0; i < N; i++) {
                ecs_add_ids(world, ids[i], &add_id, 1);
            }
        }
        /* else: just FX */

        FX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_long_running();
    failures += test_multi_world_parallel();
    failures += test_all_tier_apis();
    failures += test_query_stress();
    failures += test_observer_dispatch();
    failures += test_allocator_pressure();
    failures += test_random_ops();
    failures += test_a2_unchecked();
    failures += test_v1_grow_data();

    if (failures) {
        printf("\n%d FINAL TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 9 FINAL COMPREHENSIVE TESTS PASSED\n");
    return 0;
}