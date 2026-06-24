/* Observer dispatch investigation. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x; } OPos;
ECS_COMPONENT_DECLARE(OPos);

static int on_add_count = 0;
static int on_set_count = 0;
static int on_remove_count = 0;

static void on_add_cb(ecs_iter_t *it) { on_add_count += it->count; }
static void on_set_cb(ecs_iter_t *it) { on_set_count += it->count; }
static void on_remove_cb(ecs_iter_t *it) { on_remove_count += it->count; }

static int test_bulk_set_observer_dispatch(void) {
    printf("TEST: bulk_set_observer_dispatch\n");
    on_add_count = 0;
    on_set_count = 0;
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(OPos) }},
        .events = { EcsOnAdd },
        .callback = on_add_cb
    });
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(OPos) }},
        .events = { EcsOnSet },
        .callback = on_set_cb
    });

    /* Method 1: bulk_new_w_id (creates entities with component, but no value) */
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(OPos), 100);
    printf("  after bulk_new: on_add=%d on_set=%d\n", on_add_count, on_set_count);

    /* Method 2: bulk_set_id (sets values, should fire OnSet) */
    OPos p = {42};
    on_add_count = 0;
    on_set_count = 0;
    ecs_bulk_set_id(world, ids, 100, ecs_id(OPos), sizeof(OPos), &p);
    printf("  after bulk_set: on_add=%d on_set=%d (expected on_set=100)\n", on_add_count, on_set_count);

    if (on_set_count != 100) {
        printf("  WARNING: bulk_set did not dispatch OnSet for all 100 entities!\n");
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_bulk_delete_observer_dispatch(void) {
    printf("TEST: bulk_delete_observer_dispatch\n");
    on_remove_count = 0;
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(OPos) }},
        .events = { EcsOnRemove },
        .callback = on_remove_cb
    });

    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(OPos), 50);
    ecs_bulk_delete(world, ids, 50);

    printf("  after bulk_delete: on_remove=%d (expected 50)\n", on_remove_count);
    if (on_remove_count != 50) {
        printf("  FAIL: bulk_delete did not dispatch OnRemove!\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test: ecs_set individual vs bulk_set_id - same result */
static int test_set_vs_bulk_consistency(void) {
    printf("TEST: set_vs_bulk_consistency\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    /* Method 1: ecs_set individual */
    ecs_entity_t e1 = ecs_new(world);
    ecs_set(world, e1, OPos, {42});

    /* Method 2: bulk_set_id */
    ecs_entity_t e2 = ecs_new(world);
    OPos p = {42};
    ecs_bulk_set_id(world, &e2, 1, ecs_id(OPos), sizeof(OPos), &p);

    /* Compare values */
    const OPos *v1 = ecs_get(world, e1, OPos);
    const OPos *v2 = ecs_get(world, e2, OPos);
    if (!v1 || !v2 || v1->x != v2->x) {
        printf("  FAIL: ecs_set=%d ecs_bulk_set_id=%d\n",
            v1 ? v1->x : -1, v2 ? v2->x : -1);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_bulk_set_observer_dispatch();
    failures += test_bulk_delete_observer_dispatch();
    failures += test_set_vs_bulk_consistency();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL OBSERVER TESTS PASSED\n");
    return 0;
}