#include "flecs_patched.h"
#include <stdio.h>

typedef struct { int x; } Pos;

ECS_COMPONENT_DECLARE(Pos);

int main(void) {
    printf("TEST wildcard with DontFragment minimal\n"); fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    /* Mark Pos as EcsDontFragment */
    ecs_add_id(world, ecs_id(Pos), EcsDontFragment);
    printf("marked Pos as EcsDontFragment\n"); fflush(stdout);

    /* Create one entity with Pos */
    ecs_entity_t e = ecs_new(world);
    Pos p = {.x = 42};
    ecs_set(world, e, Pos, {42});
    printf("created entity %llu with Pos=42\n", (unsigned long long)e); fflush(stdout);

    /* Wildcard query (Position self) */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = EcsWildcard }}
    });
    if (!q) { printf("FAIL: query init NULL\n"); return 1; }
    printf("query created\n"); fflush(stdout);

    /* Iterate */
    int matched = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        printf("iter count=%d\n", it.count); fflush(stdout);
        matched += it.count;
    }
    printf("matched=%d\n", matched); fflush(stdout);

    ecs_query_fini(q);
    ecs_fini(world);
    return matched == 1 ? 0 : 1;
}