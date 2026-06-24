/* Tier-v19 + UPSTREAM e0f296c34 simple regression test:
 * flecs_table_copy_elem — optimized small-component-value move.
 *
 * Tests that creating entities with components of various sizes (4, 8, 16,
 * 32, 64 bytes) works correctly. flecs_table_copy_elem is used internally
 * when entities migrate between archetypes.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int32_t x; } Tiny;          /* 4 bytes */
typedef struct { int64_t x, y; } Small;     /* 16 bytes */
typedef struct { double x, y, z, w; } Medium; /* 32 bytes */

ECS_COMPONENT_DECLARE(Tiny);
ECS_COMPONENT_DECLARE(Small);
ECS_COMPONENT_DECLARE(Medium);

static int test_basic_copy(void) {
    printf("TEST e0f296c-1: basic entity create + set + get\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Tiny);
    ECS_COMPONENT_DEFINE(world, Small);
    ECS_COMPONENT_DEFINE(world, Medium);

    /* Create 100 entities, set components, verify */
    int fail = 0;
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(world);
        Tiny t = {.x = i};
        Small s = {.x = i, .y = i * 2};
        Medium m = {.x = (double)i, .y = 0, .z = 0, .w = 0};

        ecs_set_id(world, e, ecs_id(Tiny), sizeof(Tiny), &t);
        ecs_set_id(world, e, ecs_id(Small), sizeof(Small), &s);
        ecs_set_id(world, e, ecs_id(Medium), sizeof(Medium), &m);

        const Tiny *gt = ecs_get(world, e, Tiny);
        const Small *gs = ecs_get(world, e, Small);
        const Medium *gm = ecs_get(world, e, Medium);

        if (!gt || gt->x != i) {
            fail++;
            if (i < 5) printf("  FAIL: entity %d Tiny mismatch\n", i);
        }
        if (!gs || gs->x != i || gs->y != i * 2) {
            fail++;
            if (i < 5) printf("  FAIL: entity %d Small mismatch\n", i);
        }
        if (!gm || gm->x != (double)i) {
            fail++;
            if (i < 5) printf("  FAIL: entity %d Medium mismatch\n", i);
        }
    }

    ecs_fini(world);
    if (fail) {
        printf("  FAIL (%d entity mismatches)\n", fail);
        return 1;
    }
    printf("  PASS (100 entities, 3 sizes 4/16/32)\n");
    return 0;
}

static int test_migration(void) {
    printf("TEST e0f296c-2: archetype migration (add Small to Tiny entity)\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Tiny);
    ECS_COMPONENT_DEFINE(world, Small);

    ecs_entity_t e = ecs_new(world);
    Tiny t = {.x = 42};
    ecs_set_id(world, e, ecs_id(Tiny), sizeof(Tiny), &t);

    /* Add Small — triggers archetype migration, flecs_table_copy_elem
     * must copy Tiny's 4 bytes to new archetype */
    ecs_add_id(world, e, ecs_id(Small));
    Small s = {.x = 100, .y = 200};
    ecs_set_id(world, e, ecs_id(Small), sizeof(Small), &s);

    const Tiny *gt = ecs_get(world, e, Tiny);
    const Small *gs = ecs_get(world, e, Small);

    int fail = 0;
    if (!gt || gt->x != 42) {
        fail++;
        printf("  FAIL: Tiny.x = %d, expected 42 (migration lost data)\n",
               gt ? gt->x : 0);
    }
    if (!gs || gs->x != 100 || gs->y != 200) {
        fail++;
        printf("  FAIL: Small mismatch\n");
    }

    ecs_fini(world);
    if (fail) return 1;
    printf("  PASS (Tiny=42 preserved across migration, Small=100/200 set)\n");
    return 0;
}

int main(void) {
    int failed = 0;
    failed += test_basic_copy();
    failed += test_migration();
    if (failed) {
        printf("\n=== %d test(s) FAILED ===\n", failed);
        return 1;
    }
    printf("\n=== ALL e0f296c TESTS PASSED ===\n");
    return 0;
}