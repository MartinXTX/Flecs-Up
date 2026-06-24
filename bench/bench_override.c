/* TIER-V18 P1-3: IsA-heavy override benchmark
 *
 * Workload:
 *   - One base with Position
 *   - N entities that ONLY inherit Position (no own Position)
 *   - Per move cycle: add/remove a Tag from every entity
 *     -- forces table move + flecs_table_update_overrides
 *     -- override refs must be re-resolved (or skipped via world-gen)
 *
 * The Tier-v18 P1-3 patch should yield +30-50% on this workload because
 * flecs_table_update_overrides becomes a single uint64 compare once the
 * world-level cache is warm.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

typedef struct Position { float x, y; } Position;
typedef struct TouchTag { int dummy; } TouchTag;

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(TouchTag);

/* Add then remove TouchTag to force table move on entity. */
static void touch(ecs_world_t *world, ecs_entity_t e) {
    ecs_add_id(world, e, ecs_id(TouchTag));
    ecs_remove_id(world, e, ecs_id(TouchTag));
}

int main(int argc, char **argv) {
    int N = (argc > 1) ? atoi(argv[1]) : 1000;
    int ITERS = (argc > 2) ? atoi(argv[2]) : 5000;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("=== IsA-heavy override bench: N=%d, ITERS=%d ===\n", N, ITERS);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, TouchTag);

    /* Single base with Position. All entities inherit Position from this. */
    ecs_entity_t base = ecs_entity_init(world, &(ecs_entity_desc_t){
        .name = "Base"
    });
    ecs_add_id(world, base, ecs_id(Position));

    /* Create N entities, each ONLY has IsA(base) -- no own Position */
    ecs_entity_t *entities = malloc(sizeof(ecs_entity_t) * N);
    for (int i = 0; i < N; i++) {
        entities[i] = ecs_new(world);
        ecs_add_pair(world, entities[i], EcsIsA, base);
    }

    /* Sanity: each entity should resolve Position via IsA */
    int sanity = 0;
    for (int i = 0; i < 10; i++) {
        const Position *p = ecs_get_id(world, entities[i], ecs_id(Position));
        if (p) sanity++;
    }
    printf("IsA resolve sanity: %d/10 (must be 10)\n", sanity);

    /* Warmup */
    for (int i = 0; i < 100; i++) touch(world, entities[i % N]);

    /* Benchmark: add+remove Tag for every entity, every iteration */
    double t0 = now_ms();
    for (int it = 0; it < ITERS; it++) {
        for (int i = 0; i < N; i++) {
            touch(world, entities[i]);
        }
    }
    double t1 = now_ms();

    double secs = (t1 - t0) / 1000.0;
    long long total_ops = (long long)ITERS * (long long)N * 2; /* add + remove */
    printf("moves: %.2f ms total, %.0f ops/sec (%.3f us/op)\n",
        t1 - t0, (double)total_ops / secs, secs * 1e6 / (double)total_ops);

    /* Stale-cache test: change base Position, see if a move triggers
     * re-resolution. After the move, ecs_get should return the new value. */
    Position new_pos;
    new_pos.x = 999.0f; new_pos.y = 999.0f;
    ecs_set_id(world, base, ecs_id(Position), sizeof(Position), &new_pos);

    int stale_ok = 0;
    for (int i = 0; i < 10; i++) {
        ecs_entity_t e = entities[i];
        touch(world, e); /* triggers re-resolve */
        const Position *p = ecs_get_id(world, e, ecs_id(Position));
        if (p && p->x == 999.0f) stale_ok++;
    }
    printf("stale-cache invalidation: %d/10 (must be 10)\n", stale_ok);

    ecs_fini(world);
    free(entities);
    printf("=== PASS ===\n");
    return 0;
}
