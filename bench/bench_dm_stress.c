/* TIER-V17 P0-3 dense_map stress test
 * Create 50K+ unique pair ids to make id_index_hi large,
 * then benchmark lookups.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <time.h>

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

typedef struct { int x; } Position;

int main(int argc, char **argv) {
    int N_pairs = (argc > 1) ? atoi(argv[1]) : 50000;
    int N_lookups = (argc > 2) ? atoi(argv[2]) : 1000000;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== dense_map stress: %d pairs, %d lookups ===\n", N_pairs, N_lookups);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Position);

    /* Create many unique high-id component records via pair relations */
    ecs_entity_t rel = ecs_new(world);
    ecs_entity_t *entities = ecs_bulk_new(world, Position, N_pairs);

    /* Build N_pairs unique (rel, tgt) pairs and add to entities */
    ecs_id_t *pair_ids = malloc(sizeof(ecs_id_t) * N_pairs);
    double t0 = now_ms();
    for (int i = 0; i < N_pairs; i++) {
        pair_ids[i] = ecs_pair(rel, entities[i]);
        ecs_add_id(world, entities[i], pair_ids[i]);
    }
    double t1 = now_ms();
    printf("created %d pairs: %.2f ms\n", N_pairs, t1 - t0);

    /* Now benchmark lookups: sequential pair ids → flecs_components_get path */
    int hits = 0;
    double t2 = now_ms();
    for (int i = 0; i < N_lookups; i++) {
        int idx = i % N_pairs;
        /* This exercises ecs_get_id which uses id_index_hi lookup */
        Position *p = ecs_get_id(world, entities[idx], pair_ids[idx]);
        if (p) hits++;
    }
    double t3 = now_ms();
    printf("lookup %d pairs: %.2f ms (%.0f/sec), hits=%d\n",
        N_lookups, t3 - t2, N_lookups / ((t3 - t2) / 1000.0), hits);

    double t4 = now_ms();
    ecs_fini(world);
    double t5 = now_ms();
    printf("fini: %.2f ms\n", t5 - t4);

    free(pair_ids);
    printf("=== PASS ===\n");
    return 0;
}