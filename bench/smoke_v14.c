/* Smoke test: ecs_init/ecs_new/ecs_fini works on restored Tier-v14.1 source. */
#include "flecs_patched.h"
#include <stdio.h>

int main(void) {
    ecs_world_t *w = ecs_init();
    if (!w) { printf("FAIL: ecs_init returned NULL\n"); return 1; }
    printf("ecs_init OK w=%p\n", (void*)w);

    ecs_entity_t e = ecs_new(w);
    if (!e) { printf("FAIL: ecs_new returned 0\n"); ecs_fini(w); return 1; }
    printf("ecs_new OK e=%llu\n", (unsigned long long)e);

    ecs_fini(w);
    printf("ecs_fini OK\n");
    return 0;
}