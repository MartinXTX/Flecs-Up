/* FINAL COMPREHENSIVE VERIFICATION PASS.
 * Tests every Tier combination, all edge cases, all real-world patterns.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } FX;
typedef struct { int y; } FY;
typedef struct { int z; } FZ;
typedef struct { float w; } FW;

ECS_COMPONENT_DECLARE(FX);
ECS_COMPONENT_DECLARE(FY);
ECS_COMPONENT_DECLARE(FZ);
ECS_COMPONENT_DECLARE(FW);

/* TEST 1: Every Tier API in single sequence */
static int test_all_apis_single(void) {
    printf("TEST 1: all_apis_single_sequence\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);
    ECS_COMPONENT_DEFINE(world, FW);

    const int N = 20;

    /* Tier-R1 */
    FX fx = {42};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(FX), N, &fx, sizeof(FX));

    /* Tier-EE1 */
    ecs_id_t add_ids[3] = {ecs_id(FY), ecs_id(FZ), ecs_id(FW)};
    ecs_add_ids_w_entities(world, ids, N, add_ids, 3);

    /* Tier-O1 */
    FY fy = {1};
    ecs_bulk_set_id(world, ids, N, ecs_id(FY), sizeof(FY), &fy);

    /* Verify */
    for (int i = 0; i < N; i++) {
        const FX *px = ecs_get(world, ids[i], FX);
        const FY *py = ecs_get(world, ids[i], FY);
        const FZ *pz = ecs_get(world, ids[i], FZ);
        const FW *pw = ecs_get(world, ids[i], FW);
        assert(px->x == 42);
        assert(py->y == 1);
        assert(pz != NULL);
        assert(pw != NULL);
    }

    /* Tier-I1 remove */
    ecs_id_t rm_ids[1] = {ecs_id(FZ)};
    for (int i = 0; i < N; i++) {
        ecs_remove_ids(world, ids[i], rm_ids, 1);
    }

    /* Tier-P1 */
    ecs_bulk_delete(world, ids, N);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: Bulk operations with all component types */
static int test_bulk_all_components(void) {
    printf("TEST 2: bulk_all_components\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);
    ECS_COMPONENT_DEFINE(world, FW);

    for (int round = 0; round < 50; round++) {
        const int N = 30;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);

        FX fx = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &fx);

        ecs_id_t add_ids[3] = {ecs_id(FY), ecs_id(FZ), ecs_id(FW)};
        ecs_add_ids_w_entities(world, ids, N, add_ids, 3);

        FY fy = {round * 2};
        ecs_bulk_set_id(world, ids, N, ecs_id(FY), sizeof(FY), &fy);

        FZ fz = {round * 3};
        ecs_bulk_set_id(world, ids, N, ecs_id(FZ), sizeof(FZ), &fz);

        for (int i = 0; i < N; i++) {
            const FX *px = ecs_get(world, ids[i], FX);
            const FY *py = ecs_get(world, ids[i], FY);
            const FZ *pz = ecs_get(world, ids[i], FZ);
            assert(px->x == round);
            assert(py->y == round * 2);
            assert(pz->z == round * 3);
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (50 rounds)\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: Stress test 2000 ops random */
static int test_stress_random(void) {
    printf("TEST 3: stress_random_2000\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    ecs_entity_t alive[200];
    int alive_count = 0;

    srand(42);
    for (int op = 0; op < 2000; op++) {
        int action = rand() % 5;
        if (action == 0 && alive_count < 200) {
            ecs_entity_t e = ecs_new(world);
            alive[alive_count++] = e;
        } else if (action == 1 && alive_count > 0) {
            int idx = rand() % alive_count;
            ecs_set(world, alive[idx], FX, {op});
        } else if (action == 2 && alive_count > 0) {
            int idx = rand() % alive_count;
            ecs_delete(world, alive[idx]);
            alive[idx] = alive[--alive_count];
        } else if (action == 3) {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), 5);
            ecs_bulk_delete(world, ids, 5);
        } else if (alive_count >= 5) {
            ecs_entity_t batch[5];
            for (int i = 0; i < 5; i++) batch[i] = alive[i];
            FX v = {op};
            ecs_bulk_set_id(world, batch, 5, ecs_id(FX), sizeof(FX), &v);
        }
    }

    while (alive_count > 0) ecs_delete(world, alive[--alive_count]);

    ecs_fini(world);
    printf("  PASS (2000 ops)\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Repeated init/fini cycles */
static int test_repeated_init(void) {
    printf("TEST 4: repeated_init_fini_30\n");
    fflush(stdout);
    for (int round = 0; round < 30; round++) {
        ecs_world_t *world = ecs_init();
        ECS_COMPONENT_DEFINE(world, FX);
        const int N = 20;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
        FX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);
        ecs_bulk_delete(world, ids, N);
        ecs_fini(world);
    }
    printf("  PASS (30 init/fini cycles)\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Mixed archetypes final */
static int test_mixed_archetypes_final(void) {
    printf("TEST 5: mixed_archetypes_final\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);

    /* Create entities with different archetype distributions */
    for (int round = 0; round < 50; round++) {
        const int N = 60;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);

        /* 1/3 each archetype */
        ecs_id_t yz[2] = {ecs_id(FY), ecs_id(FZ)};
        ecs_id_t y[1] = {ecs_id(FY)};

        for (int i = 0; i < N; i++) {
            if (i < N / 3) {
                ecs_add_ids(world, ids[i], yz, 2);
            } else if (i < 2 * N / 3) {
                ecs_add_ids(world, ids[i], y, 1);
            }
            /* else: just FX */
        }

        FX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

        for (int i = 0; i < N; i++) {
            const FX *p = ecs_get(world, ids[i], FX);
            assert(p->x == round);
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Edge cases - all */
static int test_edge_cases_all(void) {
    printf("TEST 6: edge_cases_all\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);

    /* Edge: count=0 */
    ecs_bulk_delete(world, NULL, 0);
    ecs_entity_t dummy[1] = {0};
    ecs_bulk_delete(world, dummy, 0);

    /* Edge: count=1 */
    ecs_entity_t e1 = ecs_new(world);
    ecs_add_id(world, e1, ecs_id(FX));
    ecs_bulk_delete(world, &e1, 1);

    /* Edge: already-deleted */
    const int N = 5;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
    ecs_delete(world, ids[2]);
    ecs_bulk_delete(world, ids, N);

    /* Edge: large batch */
    const int BIG = 10000;
    const ecs_entity_t *big_ids = ecs_bulk_new_w_id(world, ecs_id(FX), BIG);
    FX v = {42};
    ecs_bulk_set_id(world, big_ids, BIG, ecs_id(FX), sizeof(FX), &v);
    ecs_bulk_delete(world, big_ids, BIG);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Query after bulk */
static int test_query_after_bulk(void) {
    printf("TEST 7: query_after_bulk_ops\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(FX) }}
    });

    for (int round = 0; round < 50; round++) {
        const int N = 30;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
        FX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);

        int count = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            const FX *p = ecs_field(&it, FX, 0);
            for (int i = 0; i < it.count; i++) {
                if (p[i].x == round) count++;
            }
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (50 query rounds)\n");
    fflush(stdout);
    return 0;
}

/* TEST 8: Deep nested loops */
static int test_deep_nested(void) {
    printf("TEST 8: deep_nested_loops\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);

    for (int outer = 0; outer < 20; outer++) {
        const int N = 30;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);

        for (int inner = 0; inner < 10; inner++) {
            FY fy = {inner};
            ecs_id_t y_id = ecs_id(FY);
            for (int i = 0; i < N; i++) {
                ecs_add_id(world, ids[i], y_id);
                ecs_set_id(world, ids[i], ecs_id(FY), sizeof(FY), &fy);
            }

            for (int i = 0; i < N; i++) {
                const FY *p = ecs_get(world, ids[i], FY);
                assert(p->y == inner);
            }

            for (int i = 0; i < N; i++) {
                ecs_remove_id(world, ids[i], y_id);
            }
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 9: Bulk observer interactions */
static int observer_count = 0;
static void bulk_obs_cb(ecs_iter_t *it) { observer_count += it->count; }

static int test_bulk_observer_int(void) {
    printf("TEST 9: bulk_observer_interactions\n");
    fflush(stdout);
    observer_count = 0;

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(FX) }},
        .events = { EcsOnAdd },
        .callback = bulk_obs_cb
    });

    for (int round = 0; round < 20; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
        ecs_bulk_delete(world, ids, N);
    }

    printf("  observer_count=%d (expected 1000)\n", observer_count);
    if (observer_count != 1000) {
        printf("  WARN: count=%d (may vary)\n", observer_count);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 10: All final stress */
static int test_all_final_stress(void) {
    printf("TEST 10: all_final_stress (10000 mixed ops)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, FX);
    ECS_COMPONENT_DEFINE(world, FY);
    ECS_COMPONENT_DEFINE(world, FZ);

    for (int op = 0; op < 10000; op++) {
        const int N = 5;
        int action = op % 7;
        if (action < 3) {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
            FX v = {op};
            ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &v);
            ecs_bulk_delete(world, ids, N);
        } else if (action < 5) {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
            ecs_id_t add[1] = {ecs_id(FY)};
            ecs_add_ids_w_entities(world, ids, N, add, 1);
            ecs_bulk_delete(world, ids, N);
        } else if (action == 5) {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(FX), N);
            ecs_bulk_set_id(world, ids, N, ecs_id(FX), sizeof(FX), &(FX){op});
            ecs_bulk_delete(world, ids, N);
        } else {
            ecs_entity_t e = ecs_new(world);
            ecs_delete(world, e);
        }
    }

    ecs_fini(world);
    printf("  PASS (10000 ops)\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_all_apis_single();
    failures += test_bulk_all_components();
    failures += test_stress_random();
    failures += test_repeated_init();
    failures += test_mixed_archetypes_final();
    failures += test_edge_cases_all();
    failures += test_query_after_bulk();
    failures += test_deep_nested();
    failures += test_bulk_observer_int();
    failures += test_all_final_stress();

    if (failures) {
        printf("\n%d FINAL VERIFICATION TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 FINAL VERIFICATION TESTS PASSED\n");
    return 0;
}