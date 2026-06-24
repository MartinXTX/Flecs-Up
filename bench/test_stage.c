/* Stage-level cache consistency tests. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x; } SPos;
ECS_COMPONENT_DECLARE(SPos);

static int test_stage_local_j2(void) {
    printf("TEST: stage_local_j2\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SPos);

    /* Create entity */
    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, SPos);

    /* Multiple stage lookups via flecs_components_get */
    /* All stages[0] in single-thread mode, but verify cache works */
    for (int i = 0; i < 100; i++) {
        ecs_has(world, e, SPos);  /* Uses flecs_components_get internally */
    }

    /* Delete and recreate */
    ecs_delete(world, e);

    /* New entity with same ID (recycle) */
    ecs_entity_t e2 = ecs_new(world);
    ecs_add(world, e2, SPos);
    ecs_set(world, e2, SPos, {99});

    /* Verify no stale data */
    const SPos *p = ecs_get(world, e2, SPos);
    if (!p || p->x != 99) {
        printf("  FAIL: stale data after recycle\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST: Bulk operations across stage boundaries */
static int test_bulk_through_stages(void) {
    printf("TEST: bulk_through_stages\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SPos);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SPos), N);
    SPos v1 = {1};
    ecs_bulk_set_id(world, ids, N, ecs_id(SPos), sizeof(SPos), &v1);

    ecs_bulk_delete(world, ids, 50);

    /* Verify remaining 50 - just a sample */
    for (int i = 50; i < 60; i++) {
        const SPos *p = ecs_get(world, ids[i], SPos);
        if (!p) {
            printf("  FAIL: null at %d\n", i);
            return 1;
        }
        if (p->x != 1) {
            printf("  FAIL: stale at %d: x=%d\n", i, p->x);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST: Tier-CC1 with table->component_map cache invalidation */
static int test_table_map_invalidation(void) {
    printf("TEST: table_map_invalidation\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SPos);

    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SPos), 100);
    SPos v = {42};
    ecs_bulk_set_id(world, ids, 100, ecs_id(SPos), sizeof(SPos), &v);

    /* Remove component from some entities (changes archetype) */
    for (int i = 0; i < 50; i++) {
        ecs_remove(world, ids[i], SPos);
    }

    /* Re-add and bulk_set again */
    for (int i = 0; i < 50; i++) {
        ecs_add(world, ids[i], SPos);
    }

    /* Now bulk_set — Tier-CC1 might hit archetype with old component_map */
    SPos v2 = {99};
    ecs_bulk_set_id(world, ids, 100, ecs_id(SPos), sizeof(SPos), &v2);

    /* Verify all have x=99 */
    int correct = 0, wrong = 0;
    for (int i = 0; i < 100; i++) {
        const SPos *p = ecs_get(world, ids[i], SPos);
        if (p && p->x == 99) correct++;
        else wrong++;
    }
    printf("  correct=%d wrong=%d\n", correct, wrong);

    if (wrong > 0) {
        printf("  FAIL: bulk_set missed entities after archetype change\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_stage_local_j2();
    failures += test_bulk_through_stages();
    failures += test_table_map_invalidation();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL STAGE TESTS PASSED\n");
    return 0;
}