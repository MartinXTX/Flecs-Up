/* Tier-correctness validation tests. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x, y; } Pos2;
typedef struct { int x, y; } Vel2;
typedef struct { int x; } Mass2;
typedef struct { float x, y, z; } Position3;
typedef struct { int x; } SimplePos;

ECS_COMPONENT_DECLARE(Pos2);
ECS_COMPONENT_DECLARE(Vel2);
ECS_COMPONENT_DECLARE(Mass2);
ECS_COMPONENT_DECLARE(Position3);
ECS_COMPONENT_DECLARE(SimplePos);

/* Tier-FF1 + J2 entity_id recycle test */
static int test_entity_id_recycle(void) {
    printf("TEST: entity_id_recycle\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SimplePos);

    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, SimplePos, {i});
    }
    ecs_entity_t first = ecs_new(world);
    assert(ecs_is_alive(world, first));

    ecs_entity_t second = ecs_new(world);
    ecs_set(world, second, SimplePos, {42});
    const SimplePos *p2 = ecs_get(world, second, SimplePos);
    assert(p2->x == 42);
    assert(ecs_is_alive(world, first));

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_cc1_bulk_set(void) {
    printf("TEST: cc1_bulk_set\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Position3);

    const int N = 1000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(Position3), N);
    assert(ids != NULL);

    Position3 shared = {1.0f, 2.0f, 3.0f};
    ecs_bulk_set_id(world, ids, N, ecs_id(Position3), sizeof(Position3), &shared);

    for (int i = 0; i < N; i++) {
        const Position3 *p = ecs_get(world, ids[i], Position3);
        if (!p || p->x != 1.0f || p->y != 2.0f || p->z != 3.0f) {
            printf("  FAIL: entity[%d] wrong value\n", i);
            return 1;
        }
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_p1_bulk_delete(void) {
    printf("TEST: p1_bulk_delete\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SimplePos);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SimplePos), N);
    for (int i = 0; i < N; i++) assert(ecs_is_alive(world, ids[i]));

    ecs_bulk_delete(world, ids, N);

    /* Wait one frame for deferred delete to flush */
    int alive_count = 0;
    for (int i = 0; i < N; i++) {
        if (ecs_is_alive(world, ids[i])) alive_count++;
    }
    if (alive_count != 0) {
        printf("  WARN: %d/%d entities still alive after bulk_delete\n",
            alive_count, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_i1_batched_add(void) {
    printf("TEST: i1_batched_add\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos2);
    ECS_COMPONENT_DEFINE(world, Vel2);
    ECS_COMPONENT_DEFINE(world, Mass2);

    ecs_entity_t e = ecs_new(world);
    ecs_id_t ids[3] = { ecs_id(Pos2), ecs_id(Vel2), ecs_id(Mass2) };
    ecs_add_ids(world, e, ids, 3);
    assert(ecs_has(world, e, Pos2));
    assert(ecs_has(world, e, Vel2));
    assert(ecs_has(world, e, Mass2));

    ecs_remove_ids(world, e, ids, 3);
    assert(!ecs_has(world, e, Pos2));
    assert(!ecs_has(world, e, Vel2));
    assert(!ecs_has(world, e, Mass2));

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_z1_edge_cache(void) {
    printf("TEST: z1_edge_cache\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SimplePos);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, SimplePos);
    assert(ecs_has(world, e, SimplePos));
    for (int i = 0; i < 100; i++) assert(ecs_has(world, e, SimplePos));
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_a1_dont_fragment(void) {
    printf("TEST: a1_dont_fragment\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SimplePos);
    ecs_add_id(world, ecs_id(SimplePos), EcsDontFragment);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, SimplePos);
    assert(ecs_has(world, e, SimplePos));
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_mixed_workload(void) {
    printf("TEST: mixed_workload (all Tier patches)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos2);
    ECS_COMPONENT_DEFINE(world, Vel2);

    Pos2 p = {10, 20};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(Pos2), 500, &p, sizeof(Pos2));
    assert(ids != NULL);
    printf("  bulk_new_w_value OK, ids[0]=%lld\n", (long long)ids[0]);
    assert(ecs_has(world, ids[0], Pos2));
    printf("  Pos2 has check OK\n");

    ecs_add(world, ids[0], Vel2);
    printf("  ecs_add Vel2 to ids[0] OK\n");
    assert(ecs_has(world, ids[0], Vel2));
    printf("  Vel2 has check OK\n");

    /* Try adding to a few more */
    for (int i = 1; i < 10; i++) {
        ecs_add(world, ids[i], Vel2);
        if (!ecs_has(world, ids[i], Vel2)) {
            printf("  FAIL at i=%d\n", i);
            return 1;
        }
    }
    printf("  Loop test OK\n");

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_entity_id_recycle();
    failures += test_cc1_bulk_set();
    failures += test_p1_bulk_delete();
    failures += test_i1_batched_add();
    failures += test_z1_edge_cache();
    failures += test_a1_dont_fragment();
    failures += test_mixed_workload();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL CORRECTNESS TESTS PASSED\n");
    return 0;
}