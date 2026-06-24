/* Minimal test: just init/fini a world */
#include "flecs_no_addons.h"
#include <stdio.h>

int main(void) {
    printf("Creating world...\n");
    ecs_world_t *w = ecs_init();
    printf("World created\n");
    ECS_COMPONENT(w, int);
    printf("Component registered\n");
    ecs_entity_t e = ecs_new(w);
    printf("Entity created: %llu\n", (unsigned long long)e);
    ecs_fini(w);
    printf("World freed\n");
    return 0;
}
