/* TIER COMBINATION VERIFICATION + STRESS TEST */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } WX;
typedef struct { int y; } WY;
typedef struct { int z; } WZ;

ECS_COMPONENT_DECLARE(WX);
ECS_COMPONENT_DECLARE(WY);
ECS_COMPONENT_DECLARE(WZ);

/* TEST 1: Tier-I1 + Tier-O1 + Tier-P1 + Tier-R1 + Tier-EE1 - all combined */
static int test_all_bulk_tiers(void) {
    printf("TEST 1: all_bulk_tiers_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);
    ECS_COMPONENT_DEFINE(world, WZ);

    for (int round = 0; round < 100; round++) {
        const int N = 20;

        /* Tier-R1: create+set combined */
        WX vx = {round};
        const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(WX), N, &vx, sizeof(WX));

        /* Tier-I1: batched add */
        ecs_id_t add_ids[2] = {ecs_id(WY), ecs_id(WZ)};
        ecs_add_ids_w_entities(world, ids, N, add_ids, 2);

        /* Tier-O1: bulk set */
        WY vy = {round + 100};
        ecs_bulk_set_id(world, ids, N, ecs_id(WY), sizeof(WY), &vy);

        /* Verify */
        for (int i = 0; i < N; i++) {
            const WX *px = ecs_get(world, ids[i], WX);
            const WY *py = ecs_get(world, ids[i], WY);
            const WZ *pz = ecs_get(world, ids[i], WZ);
            assert(px->x == round);
            assert(py->y == round + 100);
            assert(pz != NULL);
        }

        /* Tier-P1: bulk delete */
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (100 rounds, all bulk Tier combined)\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: Tier-A1.1 + Tier-A1.2 + Tier-A2 + Tier-J2 + Tier-Z1 - all cached */
static int test_cached_tiers(void) {
    printf("TEST 2: cached_tiers_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);

    /* Tier-A1.2: ECS_DATA_COMPONENT auto-heuristic */
    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);

    ecs_id_t wy_id = ecs_id(WY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &wy_id, 1);
    }

    /* Tier-J2: world cache - iter multiple times to test cache */
    for (int iter = 0; iter < 10; iter++) {
        for (int i = 0; i < N; i++) {
            const WX *p = ecs_get(world, ids[i], WX);
            assert(p != NULL);
        }
    }

    /* Tier-A2: ecs_field_unchecked in iter */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(WX) }, { .id = ecs_id(WY) }}
    });
    int total = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        const WX *px = ecs_field_unchecked(&it, WX, 0);
        const WY *py = ecs_field_unchecked(&it, WY, 1);
        for (int i = 0; i < it.count; i++) {
            if (px[i].x >= 0 && py->y >= 0) total++;
        }
    }
    assert(total == N);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: Tier-C2 + Tier-V1 + Tier-W1 + Tier-Y1 - all prefetch */
static int test_prefetch_tiers(void) {
    printf("TEST 3: prefetch_tiers_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);
    ECS_COMPONENT_DEFINE(world, WZ);

    /* Many entities with all three components - exercises prefetch */
    const int N = 1000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);
    ecs_id_t add_ids[2] = {ecs_id(WY), ecs_id(WZ)};
    ecs_add_ids_w_entities(world, ids, N, add_ids, 2);

    WX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);

    /* Tier-W1 query iteration - exercises next-table prefetch */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(WX) }, { .id = ecs_id(WY) }}
    });
    int total = 0;
    for (int round = 0; round < 10; round++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) total += it.count;
    }
    assert(total == N * 10);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Stress test 5000 ops all Tier APIs */
static int test_stress_5000(void) {
    printf("TEST 4: stress_5000_ops_all_tiers\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);
    ECS_COMPONENT_DEFINE(world, WZ);

    for (int op = 0; op < 5000; op++) {
        const int N = 10;

        /* Mix Tier APIs */
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);

        if (op % 3 == 0) {
            WX v = {op};
            ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);
        } else if (op % 3 == 1) {
            ecs_id_t add_ids[2] = {ecs_id(WY), ecs_id(WZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
        } else {
            /* Multiple operations */
            WX v = {op};
            ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);
            ecs_id_t add_ids[1] = {ecs_id(WY)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 1);
        }

        /* Verify */
        for (int i = 0; i < N; i++) {
            assert(ecs_is_alive(world, ids[i]));
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (5000 ops)\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Tier-X1 ecs_fields prefetch usage */
static int test_x1_fields_prefetch(void) {
    printf("TEST 5: x1_fields_prefetch\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);
    ecs_id_t wy_id = ecs_id(WY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &wy_id, 1);
    }

    WX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(WX) }, { .id = ecs_id(WY) }}
    });

    /* Use ecs_fields for batched prefetch */
    int total = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        ecs_fields(&it);  /* Tier-X1 */
        const WX *px = ecs_field(&it, WX, 0);
        const WY *py = ecs_field(&it, WY, 1);
        for (int i = 0; i < it.count; i++) {
            if (px[i].x == 42 && py->y >= 0) total++;
        }
    }
    assert(total == N);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Tier-N1 warmup usage pattern */
static int test_n1_warmup_pattern(void) {
    printf("TEST 6: n1_warmup_pattern\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);

    /* Warmup phase */
    for (int i = 0; i < 50; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, WX, {i});
        ecs_delete(world, e);
    }

    /* Measure phase */
    const int N = 100;
    double t0 = 0.0;  /* no time measurement needed */
    for (int round = 0; round < 100; round++) {
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);
        WX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Tier-AA1 flecs_add_id early-out */
static int test_aa1_early_out(void) {
    printf("TEST 7: aa1_early_out\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);

    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);

        /* Verify all entities get WX added */
        for (int i = 0; i < N; i++) {
            assert(ecs_has(world, ids[i], WX));
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 8: Tier-GG1 sparse_get */
static int test_gg1_sparse(void) {
    printf("TEST 8: gg1_sparse_get\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);

    /* Use WY as sparse component (EcsDontFragment) */
    ecs_add_id(world, ecs_id(WY), EcsDontFragment);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);
    ecs_id_t wy_id = ecs_id(WY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &wy_id, 1);
    }

    WY vy = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(WY), sizeof(WY), &vy);

    /* Verify all entities can get sparse component */
    for (int i = 0; i < N; i++) {
        const WY *p = ecs_get(world, ids[i], WY);
        if (!p || p->y != 42) {
            printf("  FAIL: ids[%d]\n", i);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 9: Tier-EE1 + Tier-P1 + Tier-O1 + Tier-R1 - lifecycle */
static int test_full_lifecycle_combined(void) {
    printf("TEST 9: full_lifecycle_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);

    for (int round = 0; round < 50; round++) {
        const int N = 30;

        /* Tier-R1: create + set */
        WX vx = {round};
        const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(WX), N, &vx, sizeof(WX));

        /* Tier-EE1: batched add WY */
        ecs_id_t add_ids[1] = {ecs_id(WY)};
        ecs_add_ids_w_entities(world, ids, N, add_ids, 1);

        /* Tier-O1: bulk set WY */
        WY vy = {round + 1000};
        ecs_bulk_set_id(world, ids, N, ecs_id(WY), sizeof(WY), &vy);

        /* Verify */
        for (int i = 0; i < N; i++) {
            const WX *px = ecs_get(world, ids[i], WX);
            const WY *py = ecs_get(world, ids[i], WY);
            assert(px->x == round);
            assert(py->y == round + 1000);
        }

        /* Tier-P1: bulk delete */
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (50 rounds, full lifecycle)\n");
    fflush(stdout);
    return 0;
}

/* TEST 10: All Tier combination - extreme */
static int test_all_tier_extreme(void) {
    printf("TEST 10: all_tier_extreme (1000 ops)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, WX);
    ECS_COMPONENT_DEFINE(world, WY);
    ECS_COMPONENT_DEFINE(world, WZ);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(WX) }, { .id = ecs_id(WY) }}
    });

    for (int op = 0; op < 1000; op++) {
        const int N = 20;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(WX), N);

        WX v = {op};
        ecs_bulk_set_id(world, ids, N, ecs_id(WX), sizeof(WX), &v);

        ecs_id_t wy_id = ecs_id(WY);
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], &wy_id, 1);
        }

        WY vy = {op + 100};
        ecs_bulk_set_id(world, ids, N, ecs_id(WY), sizeof(WY), &vy);

        /* Iterate with Tier-A2 + Tier-W1 + Tier-X1 */
        int total = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            ecs_fields(&it);
            const WX *px = ecs_field_unchecked(&it, WX, 0);
            const WY *py = ecs_field_unchecked(&it, WY, 1);
            for (int i = 0; i < it.count; i++) {
                if (px[i].x == op && py->y == op + 100) total++;
            }
        }
        assert(total == N);

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (1000 ops)\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_all_bulk_tiers();
    failures += test_cached_tiers();
    failures += test_prefetch_tiers();
    failures += test_stress_5000();
    failures += test_x1_fields_prefetch();
    failures += test_n1_warmup_pattern();
    failures += test_aa1_early_out();
    failures += test_gg1_sparse();
    failures += test_full_lifecycle_combined();
    failures += test_all_tier_extreme();

    if (failures) {
        printf("\n%d VERIFICATION TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 VERIFICATION TESTS PASSED\n");
    return 0;
}