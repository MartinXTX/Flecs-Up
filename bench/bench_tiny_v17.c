/* Tiny v17 sanity: just one entity add */
#include "flecs_patched.h"
#include <stdio.h>

typedef struct { int x; } Position;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("1: starting\n");
    ecs_world_t *world = ecs_init();
    printf("2: after init\n");
    ECS_COMPONENT(world, Position);
    printf("3: after ECS_COMPONENT\n");
    ecs_entity_t e = ecs_new(world);
    printf("4: e=%llu\n", (unsigned long long)e);
    ecs_add_id(world, e, ecs_id(Position));
    printf("5: after add\n");
    ecs_fini(world);
    printf("6: after fini\n");
    return 0;
}