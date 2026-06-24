/* Distr smoke test: verify ecs_init/new/fini work with distr build. */
#include "flecs.h"
#include <stdio.h>

int main(void) {
    ecs_world_t *w = ecs_init();
    if (!w) { printf("FAIL: ecs_init\n"); return 1; }
    ecs_entity_t e = ecs_new(w);
    if (!e) { printf("FAIL: ecs_new\n"); ecs_fini(w); return 1; }
    ecs_fini(w);
    printf("Distr smoke PASS\n");
    return 0;
}