/* Minimal edge case test. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x; } MX;
ECS_COMPONENT_DECLARE(MX);

static int test_minimal(void) {
    printf("TEST: minimal\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, MX);

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, MX);
    MX v = {42};
    ecs_set(world, e, MX, {42});

    const MX *p = ecs_get(world, e, MX);
    if (!p || p->x != 42) {
        printf("  FAIL\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    return test_minimal();
}