#include "flecs_patched.h"
#include <stdio.h>

typedef struct { int x; } TestPos;

ECS_COMPONENT_DECLARE(TestPos);

int main(void) {
    printf("TEST minimal\n"); fflush(stdout);
    ecs_world_t *world = ecs_init();
    printf("after init\n"); fflush(stdout);
    ECS_COMPONENT_DEFINE(world, TestPos);
    printf("after define\n"); fflush(stdout);
    ecs_entity_t e = ecs_new(world);
    printf("after new %llu\n", (unsigned long long)e); fflush(stdout);
    TestPos t = {.x = 42};
    ecs_set(world, e, TestPos, {42});
    printf("after set\n"); fflush(stdout);
    const TestPos *p = ecs_get(world, e, TestPos);
    printf("after get %p %d\n", p, p ? p->x : 0); fflush(stdout);
    ecs_fini(world);
    printf("PASS\n");
    return 0;
}