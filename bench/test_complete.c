/* FINAL COMPREHENSIVE STRESS + VERIFICATION + STABILITY SUITE */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } GX;
typedef struct { int y; } GY;
typedef struct { int z; } GZ;

ECS_COMPONENT_DECLARE(GX);
ECS_COMPONENT_DECLARE(GY);
ECS_COMPONENT_DECLARE(GZ);

/* TEST 1: 5000 round bulk lifecycle */
static int test_5000_round(void) {
    printf("TEST 1: 5000_round_lifecycle\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);
    ECS_COMPONENT_DEFINE(world, GY);

    for (int round = 0; round < 5000; round++) {
        const int N = 5;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);

        GX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);

        ecs_id_t add_id = ecs_id(GY);
        ecs_add_ids_w_entities(world, ids, N, &add_id, 1);

        GY vy = {round + 1};
        ecs_bulk_set_id(world, ids, N, ecs_id(GY), sizeof(GY), &vy);

        for (int i = 0; i < N; i++) {
            const GX *px = ecs_get(world, ids[i], GX);
            const GY *py = ecs_get(world, ids[i], GY);
            if (!px || !py || px->x != round || py->y != round + 1) {
                printf("  FAIL: round=%d i=%d\n", round, i);
                return 1;
            }
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (5000 rounds)\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: All Tier combinations */
static int test_all_combinations(void) {
    printf("TEST 2: all_tier_combinations\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);
    ECS_COMPONENT_DEFINE(world, GY);
    ECS_COMPONENT_DEFINE(world, GZ);

    for (int round = 0; round < 50; round++) {
        const int N = 10;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);

        int op = round % 4;
        if (op == 0) {
            GX v = {round};
            ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);
        } else if (op == 1) {
            ecs_id_t add_ids[2] = {ecs_id(GY), ecs_id(GZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
        } else if (op == 2) {
            GX v = {round};
            ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);
            ecs_id_t add_ids[1] = {ecs_id(GY)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 1);
        } else {
            ecs_id_t add_ids[2] = {ecs_id(GY), ecs_id(GZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
            GX vx = {round};
            ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &vx);
        }

        for (int i = 0; i < N; i++) {
            assert(ecs_is_alive(world, ids[i]));
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (50 rounds, 4 combinations)\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: API consistency - same behavior across calls */
static int test_api_consistency(void) {
    printf("TEST 3: api_consistency\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);

    /* Verify ecs_new + ecs_set works */
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, GX, {i});
        const GX *p = ecs_get(world, e, GX);
        assert(p->x == i);
        ecs_delete(world, e);
    }

    /* Verify ecs_bulk_new_w_id + ecs_bulk_set_id */
    for (int i = 0; i < 100; i++) {
        const int N = 10;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);
        GX v = {i};
        ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);
        for (int j = 0; j < N; j++) {
            const GX *p = ecs_get(world, ids[j], GX);
            assert(p->x == i);
        }
        ecs_bulk_delete(world, ids, N);
    }

    /* Verify ecs_bulk_new_w_value */
    for (int i = 0; i < 100; i++) {
        const int N = 10;
        GX v = {i};
        const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(GX), N, &v, sizeof(GX));
        for (int j = 0; j < N; j++) {
            const GX *p = ecs_get(world, ids[j], GX);
            assert(p->x == i);
        }
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Stress 10000 mixed ops */
static int test_10000_mixed(void) {
    printf("TEST 4: stress_10000_mixed\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);
    ECS_COMPONENT_DEFINE(world, GY);
    ECS_COMPONENT_DEFINE(world, GZ);

    srand(99);
    ecs_entity_t alive[500];
    int alive_count = 0;

    for (int op = 0; op < 10000; op++) {
        int action = rand() % 6;
        if (action == 0) {
            if (alive_count < 500) {
                ecs_entity_t e = ecs_new(world);
                alive[alive_count++] = e;
            }
        } else if (action == 1) {
            if (alive_count > 0) {
                int idx = rand() % alive_count;
                ecs_set(world, alive[idx], GX, {op});
            }
        } else if (action == 2) {
            if (alive_count > 0) {
                int idx = rand() % alive_count;
                ecs_delete(world, alive[idx]);
                alive[idx] = alive[--alive_count];
            }
        } else if (action == 3) {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), 10);
            ecs_bulk_delete(world, ids, 10);
        } else if (action == 4) {
            if (alive_count >= 10) {
                ecs_entity_t batch[10];
                for (int i = 0; i < 10; i++) batch[i] = alive[i];
                GX v = {op};
                ecs_bulk_set_id(world, batch, 10, ecs_id(GX), sizeof(GX), &v);
            }
        } else {
            if (alive_count > 0) {
                int idx = rand() % alive_count;
                ecs_add_id(world, alive[idx], ecs_id(GY));
            }
        }
    }

    while (alive_count > 0) {
        ecs_delete(world, alive[--alive_count]);
    }

    ecs_fini(world);
    printf("  PASS (10000 ops)\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Final stability */
static int test_final_stability(void) {
    printf("TEST 5: final_stability_check\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);
    ECS_COMPONENT_DEFINE(world, GY);

    /* Verify world is healthy after many operations */
    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, GX, {1});
    ecs_set(world, e, GY, {2});
    const GX *px = ecs_get(world, e, GX);
    const GY *py = ecs_get(world, e, GY);
    assert(px->x == 1 && py->y == 2);

    /* Many operations */
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e2 = ecs_new(world);
        ecs_add(world, e2, GX);
        ecs_remove(world, e2, GX);
        ecs_delete(world, e2);
    }

    /* Original entity should be intact */
    px = ecs_get(world, e, GX);
    py = ecs_get(world, e, GY);
    assert(px->x == 1 && py->y == 2);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Bulk with varying sizes */
static int test_varying_sizes(void) {
    printf("TEST 6: varying_sizes\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);

    int sizes[] = {1, 5, 10, 50, 100, 500, 1000};
    int num_sizes = sizeof(sizes) / sizeof(int);

    for (int s = 0; s < num_sizes; s++) {
        const int N = sizes[s];
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);
        GX v = {s};
        ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);
        for (int i = 0; i < N; i++) {
            const GX *p = ecs_get(world, ids[i], GX);
            assert(p->x == s);
        }
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (7 sizes tested)\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Mixed bulk with different archetypes */
static int test_mixed_bulk_archetypes(void) {
    printf("TEST 7: mixed_bulk_archetypes\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);
    ECS_COMPONENT_DEFINE(world, GY);
    ECS_COMPONENT_DEFINE(world, GZ);

    for (int round = 0; round < 50; round++) {
        const int N = 30;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);

        /* Add components in different patterns */
        for (int i = 0; i < N; i++) {
            if (i % 4 == 0) {
                ecs_id_t ids2[2] = {ecs_id(GY), ecs_id(GZ)};
                ecs_add_ids(world, ids[i], ids2, 2);
            } else if (i % 4 == 1) {
                ecs_add_id(world, ids[i], ecs_id(GY));
            } else if (i % 4 == 2) {
                ecs_add_id(world, ids[i], ecs_id(GZ));
            }
        }

        GX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);

        for (int i = 0; i < N; i++) {
            const GX *p = ecs_get(world, ids[i], GX);
            assert(p->x == round);
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 8: Tier-I1 batched add/remove comprehensive */
static int test_i1_batched_comprehensive(void) {
    printf("TEST 8: i1_batched_comprehensive\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);

    for (int round = 0; round < 50; round++) {
        const int N = 10;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);

        ecs_entity_t comp_a = ecs_entity(world, { .name = "CompA" });
        ecs_entity_t comp_b = ecs_entity(world, { .name = "CompB" });
        ecs_entity_t comp_c = ecs_entity(world, { .name = "CompC" });

        ecs_id_t add_ids[3] = {comp_a, comp_b, comp_c};
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], add_ids, 3);
        }

        for (int i = 0; i < N; i++) {
            assert(ecs_has_id(world, ids[i], comp_a));
            assert(ecs_has_id(world, ids[i], comp_b));
            assert(ecs_has_id(world, ids[i], comp_c));
        }

        for (int i = 0; i < N; i++) {
            ecs_remove_ids(world, ids[i], add_ids, 3);
        }

        for (int i = 0; i < N; i++) {
            assert(!ecs_has_id(world, ids[i], comp_a));
            assert(!ecs_has_id(world, ids[i], comp_b));
            assert(!ecs_has_id(world, ids[i], comp_c));
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 9: Bulk + observer + query combined */
static int obs_count_g = 0;
static void on_add_cb_g(ecs_iter_t *it) {
    obs_count_g += it->count;
}

static int test_bulk_obs_query(void) {
    printf("TEST 9: bulk_obs_query_combined\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(GX) }},
        .events = { EcsOnAdd },
        .callback = on_add_cb_g
    });

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(GX) }}
    });

    for (int round = 0; round < 30; round++) {
        obs_count_g = 0;
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);

        /* Observer should have fired */
        if (obs_count_g != N) {
            printf("  FAIL round %d: obs_count=%d (expected %d)\n", round, obs_count_g, N);
            return 1;
        }

        /* Query should return N */
        int count = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) count += it.count;
        if (count != N) {
            printf("  FAIL round %d: count=%d\n", round, count);
            return 1;
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 10: Final 10000 ops stability */
static int test_final_10000(void) {
    printf("TEST 10: final_10000_stability\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, GX);

    for (int op = 0; op < 10000; op++) {
        const int N = 10;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(GX), N);
        GX v = {op};
        ecs_bulk_set_id(world, ids, N, ecs_id(GX), sizeof(GX), &v);
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (10000 ops)\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_5000_round();
    failures += test_all_combinations();
    failures += test_api_consistency();
    failures += test_10000_mixed();
    failures += test_final_stability();
    failures += test_varying_sizes();
    failures += test_mixed_bulk_archetypes();
    failures += test_i1_batched_comprehensive();
    failures += test_bulk_obs_query();
    failures += test_final_10000();

    if (failures) {
        printf("\n%d COMPLETE TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 COMPLETE TESTS PASSED\n");
    return 0;
}