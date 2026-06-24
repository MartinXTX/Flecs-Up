/* Tier-X1+ cache stale test.
 * Creates 2 archetypes with Position column at DIFFERENT column indices,
 * iterates both via single query. If Tier-X1+ cache is stale, the data
 * pointer will be wrong.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>

typedef struct { float x, y, z; } Position;
typedef struct { float v; } Velocity;
typedef struct { float s; } Scale;

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Velocity);
ECS_COMPONENT_DECLARE(Scale);

int main(void) {
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, Velocity);
    ECS_COMPONENT_DEFINE(world, Scale);

    /* Archetype A: {Position} — Position at column 0 */
    ecs_entity_t a1 = ecs_new(world);
    ecs_add(world, a1, Position);

    /* Archetype B: {Scale, Velocity, Position} — Position at column 2 */
    ecs_entity_t b1 = ecs_new(world);
    ecs_add(world, b1, Scale);
    ecs_add(world, b1, Velocity);
    ecs_add(world, b1, Position);

    /* Set distinctive values so we can detect wrong column access */
    ecs_set(world, a1, Position, {1.0f, 2.0f, 3.0f});
    ecs_set(world, b1, Position, {100.0f, 200.0f, 300.0f});
    ecs_set(world, b1, Velocity, {7.0f});
    ecs_set(world, b1, Scale, {8.0f});

    /* Query iterates both archetypes */
    ecs_query_t *q = ecs_query(world, {
        .terms = { { .id = ecs_id(Position) } }
    });

    int found_a = 0, found_b = 0;
    int wrong_data = 0;

    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            const Position *p = ecs_field(&it, Position, 0);
            if (it.count == 1 && p[i].x == 1.0f) {
                found_a++;
            } else if (it.count == 1 && p[i].x == 100.0f) {
                found_b++;
            } else {
                /* Cache stale: returned wrong data */
                printf("  WRONG: count=%d x=%.1f expected 1.0 or 100.0\n",
                    it.count, p[i].x);
                wrong_data++;
            }
        }
    }

    printf("[test_multitable_cache]\n");
    printf("  archetype A (col 0): found=%d (expect 1)\n", found_a);
    printf("  archetype B (col 2): found=%d (expect 1)\n", found_b);
    printf("  wrong data: %d (expect 0)\n", wrong_data);

    if (found_a == 1 && found_b == 1 && wrong_data == 0) {
        printf("  PASS — Tier-X1+ cache correct across tables\n");
    } else {
        printf("  FAIL — cache stale bug detected\n");
    }

    ecs_fini(world);
    return (found_a == 1 && found_b == 1 && wrong_data == 0) ? 0 : 1;
}
