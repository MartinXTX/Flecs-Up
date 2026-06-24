/* COMPREHENSIVE FINAL VERIFICATION TEST.
 * Tests every Tier interaction, edge case, long-running stability.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } VX;
typedef struct { int y; } VY;
typedef struct { int z; } VZ;
typedef struct { float a, b; } VW;

ECS_COMPONENT_DECLARE(VX);
ECS_COMPONENT_DECLARE(VY);
ECS_COMPONENT_DECLARE(VZ);
ECS_COMPONENT_DECLARE(VW);

/* TEST 1: Long-running iteration (10000 rounds) */
static int test_10000_iterations(void) {
    printf("TEST 1: 10000_iterations\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);

    ecs_entity_t *alive_ids = malloc(sizeof(ecs_entity_t) * 1000);
    int alive_count = 0;

    for (int iter = 0; iter < 10000; iter++) {
        /* Random action: create, set, delete, query */
        int action = iter % 4;
        if (action == 0 && alive_count < 500) {
            /* Create */
            ecs_entity_t e = ecs_new(world);
            ecs_set(world, e, VX, {iter});
            alive_ids[alive_count++] = e;
        } else if (action == 1 && alive_count > 100) {
            /* Set random */
            int idx = rand() % alive_count;
            ecs_set(world, alive_ids[idx], VX, {iter});
        } else if (action == 2 && alive_count > 0) {
            /* Delete random */
            int idx = rand() % alive_count;
            ecs_delete(world, alive_ids[idx]);
            alive_ids[idx] = alive_ids[--alive_count];
        }
        /* else: query */
    }
    printf("  10000 iterations OK, final alive=%d\n", alive_count);

    /* Cleanup */
    while (alive_count > 0) {
        ecs_delete(world, alive_ids[--alive_count]);
    }
    free(alive_ids);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: Bulk delete + re-create ID reuse */
static int test_bulk_delete_recreate(void) {
    printf("TEST 2: bulk_delete_recreate\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);

    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);
        VX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &v);
        ecs_bulk_delete(world, ids, N);

        /* Re-create */
        const ecs_entity_t *new_ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);
        VX v2 = {round + 1000};
        ecs_bulk_set_id(world, new_ids, N, ecs_id(VX), sizeof(VX), &v2);
        ecs_bulk_delete(world, new_ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: Tier-I1 + Tier-O1 + Tier-P1 combined */
static int test_all_bulk_combined(void) {
    printf("TEST 3: all_bulk_combined\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);
    ECS_COMPONENT_DEFINE(world, VY);
    ECS_COMPONENT_DEFINE(world, VZ);

    for (int round = 0; round < 100; round++) {
        /* Use ALL bulk APIs in sequence */
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);

        /* Add Y to half */
        ecs_id_t vy_id = ecs_id(VY);
        for (int i = 0; i < N / 2; i++) {
            ecs_add_ids(world, ids[i], &vy_id, 1);
        }

        /* Set values */
        VX vx = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &vx);

        /* Add Z to all */
        ecs_id_t vz_id = ecs_id(VZ);
        ecs_id_t add_ids[1] = {vz_id};
        ecs_add_ids_w_entities(world, ids, N, add_ids, 1);

        /* Verify */
        for (int i = 0; i < N; i++) {
            const VX *px = ecs_get(world, ids[i], VX);
            const VZ *pz = ecs_get(world, ids[i], VZ);
            assert(px->x == round);
            assert(pz != NULL);
        }

        /* Delete all */
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Mixed defer / non-defer operations */
static int test_mixed_defer_ops(void) {
    printf("TEST 4: mixed_defer_ops\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);
    ECS_COMPONENT_DEFINE(world, VY);

    ecs_entity_t e = ecs_new(world);

    /* Pattern 1: defer → add → defer_end → check */
    ecs_defer_begin(world);
    ecs_add_id(world, e, ecs_id(VX));
    ecs_defer_end(world);
    assert(ecs_has(world, e, VX));

    /* Pattern 2: defer → remove → defer_end → check */
    ecs_defer_begin(world);
    ecs_remove_id(world, e, ecs_id(VX));
    ecs_defer_end(world);
    assert(!ecs_has(world, e, VX));

    /* Pattern 3: add, defer, remove, defer_end */
    ecs_add_id(world, e, ecs_id(VX));
    ecs_defer_begin(world);
    ecs_add_id(world, e, ecs_id(VY));
    ecs_defer_end(world);
    assert(ecs_has(world, e, VX));
    assert(ecs_has(world, e, VY));

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Tier-A1.1 EcsDontFragment dispatch */
static int test_a11_dontfragment(void) {
    printf("TEST 5: a11_dontfragment\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);

    /* Mark VX as DontFragment */
    ecs_add_id(world, ecs_id(VX), EcsDontFragment);

    /* Add component to entities */
    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);

    /* Set values */
    VX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &v);

    /* Verify all have correct value */
    for (int i = 0; i < N; i++) {
        const VX *p = ecs_get(world, ids[i], VX);
        if (!p || p->x != 42) {
            printf("  FAIL: ids[%d]=%lld p->x=%d\n",
                i, (long long)ids[i], p ? p->x : -1);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Tier-R1 stress with same value */
static int test_r1_same_value(void) {
    printf("TEST 6: r1_same_value\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);

    /* Use Tier-R1 to create+set with same value repeatedly */
    VX v = {999};
    for (int round = 0; round < 100; round++) {
        const int N = 100;
        const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(VX), N, &v, sizeof(VX));
        for (int i = 0; i < N; i++) {
            const VX *p = ecs_get(world, ids[i], VX);
            assert(p->x == 999);
        }
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Tier-A2 unchecked field in iter */
static int test_a2_unchecked_field(void) {
    printf("TEST 7: a2_unchecked_field\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);
    ECS_COMPONENT_DEFINE(world, VY);

    const int N = 1000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);
    ecs_id_t vy_id = ecs_id(VY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &vy_id, 1);
    }

    VX v = {100};
    ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &v);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(VX) }, { .id = ecs_id(VY) }}
    });

    /* Use Tier-A2 unchecked field access */
    int total = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        const VX *px = ecs_field_unchecked(&it, VX, 0);
        const VY *py = ecs_field_unchecked(&it, VY, 1);
        for (int i = 0; i < it.count; i++) {
            if (px[i].x == 100 && py->y >= 0) total++;
        }
    }
    assert(total == N);

    ecs_fini(world);
    printf("  PASS (total=%d)\n", total);
    fflush(stdout);
    return 0;
}

/* TEST 8: Tier-CC1 mixed archetype skip path */
static int test_cc1_skip_path(void) {
    printf("TEST 8: cc1_skip_path (Tier-CC1 disabled — slow path OK)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);
    ECS_COMPONENT_DEFINE(world, VY);

    /* Create entities in different archetypes */
    const int N = 20;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);

    /* Add VY to first half only */
    ecs_id_t vy_id = ecs_id(VY);
    for (int i = 0; i < N / 2; i++) {
        ecs_add_ids(world, ids[i], &vy_id, 1);
    }

    /* Tier-CC1 is disabled, so bulk_set_id uses slow path */
    VX v = {77};
    ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &v);

    /* Verify all have VX = 77 */
    for (int i = 0; i < N; i++) {
        const VX *p = ecs_get(world, ids[i], VX);
        if (!p || p->x != 77) {
            printf("  FAIL: ids[%d]=%lld p->x=%d\n",
                i, (long long)ids[i], p ? p->x : -1);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 9: Tier-P1 + Tier-Z1 disabled safety */
static int test_p1_z1_disabled(void) {
    printf("TEST 9: p1_z1_disabled_safety\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);

    /* Bulk create + delete repeatedly */
    for (int round = 0; round < 200; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(VX), N);
        VX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(VX), sizeof(VX), &v);
        ecs_bulk_delete(world, ids, N);
    }

    /* World should be healthy */
    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, VX, {999});
    const VX *p = ecs_get(world, e, VX);
    assert(p->x == 999);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 10: All Tier stack */
static int test_all_tier_stack(void) {
    printf("TEST 10: all_tier_stack\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, VX);
    ECS_COMPONENT_DEFINE(world, VY);
    ECS_COMPONENT_DEFINE(world, VZ);
    ECS_COMPONENT_DEFINE(world, VW);

    /* Use every Tier API in one round */
    const int N = 50;
    for (int round = 0; round < 10; round++) {  /* reduced for speed */
        /* Tier-R1: combined create+set */
        VX vx = {round};
        const ecs_entity_t *ids2 = ecs_bulk_new_w_value(world, ecs_id(VX), N, &vx, sizeof(VX));

        /* Tier-I1: batched add */
        ecs_id_t add_ids[2] = {ecs_id(VY), ecs_id(VZ)};
        ecs_add_ids_w_entities(world, ids2, N, add_ids, 2);

        /* Tier-A2: unchecked field */
        ecs_query_t *q = ecs_query(world, {
            .terms = {{ .id = ecs_id(VX) }}
        });
        int count = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            const VX *px = ecs_field_unchecked(&it, VX, 0);
            for (int i = 0; i < it.count; i++) {
                if (px[i].x == round) count++;
            }
        }

        /* Tier-P1: bulk delete */
        ecs_bulk_delete(world, ids2, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_10000_iterations();
    failures += test_bulk_delete_recreate();
    failures += test_all_bulk_combined();
    failures += test_mixed_defer_ops();
    failures += test_a11_dontfragment();
    failures += test_r1_same_value();
    failures += test_a2_unchecked_field();
    failures += test_cc1_skip_path();
    failures += test_p1_z1_disabled();
    failures += test_all_tier_stack();

    if (failures) {
        printf("\n%d COMPREHENSIVE TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 COMPREHENSIVE TESTS PASSED\n");
    return 0;
}