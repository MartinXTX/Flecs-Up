/* Simplified edge case tests. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x; } EPos;
typedef struct { float data[64]; } BigStruct;
ECS_COMPONENT_DECLARE(EPos);
ECS_COMPONENT_DECLARE(BigStruct);

static int test_zero_count(void) {
    printf("TEST: zero_count\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, EPos);
    ecs_bulk_delete(world, NULL, 0);  /* should not crash */
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_count_one(void) {
    printf("TEST: count_one\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, EPos);

    ecs_entity_t e = ecs_new(world);
    EPos p = {42};
    ecs_set(world, e, EPos, {42});
    assert(ecs_get(world, e, EPos)->x == 42);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_recursive_delete(void) {
    printf("TEST: recursive_delete\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, EPos);

    for (int round = 0; round < 5; round++) {
        ecs_entity_t es[100];
        for (int i = 0; i < 100; i++) {
            es[i] = ecs_new(world);
        }
        ecs_bulk_delete(world, es, 50);
        for (int i = 0; i < 50; i++) assert(!ecs_is_alive(world, es[i]));
        for (int i = 50; i < 100; i++) assert(ecs_is_alive(world, es[i]));
        ecs_bulk_delete(world, es + 50, 50);
        for (int i = 50; i < 100; i++) assert(!ecs_is_alive(world, es[i]));
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_id_recycle_stress(void) {
    printf("TEST: id_recycle_stress\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, EPos);

    /* Stress test ID recycle */
    for (int round = 0; round < 50; round++) {
        ecs_entity_t es[200];
        for (int i = 0; i < 200; i++) {
            es[i] = ecs_new(world);
            ecs_set(world, es[i], EPos, {i});
        }
        ecs_bulk_delete(world, es, 100);

        /* Create 100 new entities — they'll get recycled IDs */
        ecs_entity_t fresh[100];
        for (int i = 0; i < 100; i++) {
            fresh[i] = ecs_new(world);
            ecs_set(world, fresh[i], EPos, {i + 1000});
        }

        /* Verify no stale data */
        for (int i = 0; i < 100; i++) {
            const EPos *p = ecs_get(world, fresh[i], EPos);
            assert(p != NULL);
            assert(p->x == i + 1000);
        }

        ecs_bulk_delete(world, es + 100, 100);
        ecs_bulk_delete(world, fresh, 100);
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_bulk_after_query(void) {
    printf("TEST: bulk_after_query\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, EPos);

    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(EPos), 500);
    EPos p = {7};
    ecs_bulk_set_id(world, ids, 500, ecs_id(EPos), sizeof(EPos), &p);

    /* Run query — should iterate */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(EPos) }}
    });

    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        const EPos *pp = ecs_field(&it, EPos, 0);
        for (int i = 0; i < it.count; i++) {
            assert(pp[i].x == 7);
            count++;
        }
    }
    assert(count == 500);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_large_component(void) {
    printf("TEST: large_component\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, BigStruct);

    const int N = 1000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(BigStruct), N);

    BigStruct shared = {{0}};
    for (int i = 0; i < 64; i++) shared.data[i] = (float)i;

    ecs_bulk_set_id(world, ids, N, ecs_id(BigStruct), sizeof(BigStruct), &shared);

    for (int i = 0; i < N; i += 100) {
        const BigStruct *p = ecs_get(world, ids[i], BigStruct);
        for (int j = 0; j < 64; j++) {
            assert(p->data[j] == (float)j);
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_zero_count();
    failures += test_count_one();
    failures += test_recursive_delete();
    failures += test_id_recycle_stress();
    failures += test_bulk_after_query();
    failures += test_large_component();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL EDGE TESTS PASSED\n");
    return 0;
}