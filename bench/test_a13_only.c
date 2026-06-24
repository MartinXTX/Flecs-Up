/* Standalone test for Tier-A1.3 auto-mark */
#include "flecs_patched.h"
#include <stdio.h>

typedef struct { int x; int y; } AutoDF;

int main(void) {
    printf("A: init\n"); fflush(stdout);
    ecs_world_t *world = ecs_init();

    printf("B: enable auto-dont-fragment\n"); fflush(stdout);
    ecs_world_auto_dont_fragment(world, true);

    printf("C: ECS_COMPONENT(AutoDF)\n"); fflush(stdout);
    ECS_COMPONENT(world, AutoDF);

    printf("D: check EcsDontFragment\n"); fflush(stdout);
    bool has = ecs_has_id(world, ecs_id(AutoDF), EcsDontFragment);
    printf("  has=%d\n", has); fflush(stdout);

    if (!has) {
        printf("FAIL: AutoDF not auto-marked\n");
        ecs_fini(world);
        return 1;
    }

    printf("E: ecs_set/get test\n"); fflush(stdout);
    ecs_entity_t e = ecs_new(world);
    AutoDF v = {10, 20};
    ecs_set(world, e, AutoDF, {10, 20});
    const AutoDF *p = ecs_get(world, e, AutoDF);
    if (!p || p->x != 10 || p->y != 20) {
        printf("FAIL: data path\n");
        ecs_fini(world);
        return 1;
    }
    printf("  OK x=%d y=%d\n", p->x, p->y);

    ecs_fini(world);
    printf("PASS\n");
    return 0;
}