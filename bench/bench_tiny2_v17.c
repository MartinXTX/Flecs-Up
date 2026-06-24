/* Bulk new test */
#include "flecs_patched.h"
#include <stdio.h>

typedef struct { int x; } Position;

int main(int argc, char **argv) {
    int N = (argc > 1) ? atoi(argv[1]) : 10;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("starting N=%d\n", N);
    ecs_world_t *world = ecs_init();
    printf("after init\n");
    ECS_COMPONENT(world, Position);
    printf("after ECS_COMPONENT pos=%llu\n", (unsigned long long)ecs_id(Position));
    ecs_entity_t *es = ecs_bulk_new(world, Position, N);
    printf("after bulk_new, es=%p\n", (void*)es);
    if (es) printf("  first id=%llu\n", (unsigned long long)es[0]);
    printf("calling fini\n");
    ecs_fini(world);
    printf("after fini\n");
    return 0;
}