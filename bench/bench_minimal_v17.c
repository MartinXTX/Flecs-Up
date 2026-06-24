/* Minimal v17 sanity: just init and fini, nothing else */
#include "flecs_patched.h"
#include <stdio.h>

int main(void) {
    printf("starting\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    printf("after init\n");
    fflush(stdout);
    if (!world) return 1;
    ecs_fini(world);
    printf("after fini\n");
    fflush(stdout);
    return 0;
}