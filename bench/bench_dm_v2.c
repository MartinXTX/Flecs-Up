/* TIER-V17 P0-3 dense_map focused bench v2
 *
 * Fill id_index_hi with many components, then exercise flecs_components_get.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

typedef struct { int x; } Position;

int main(int argc, char **argv) {
    int N_comps = (argc > 1) ? atoi(argv[1]) : 200;
    int N_lookups = (argc > 2) ? atoi(argv[2]) : 1000000;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== dense_map v2: %d comps, %d lookups ===\n", N_comps, N_lookups);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Position);

    /* Create N_comps unique high-id components via ecs_component_init.
     * Each gets a unique id >= FLECS_HI_COMPONENT_ID (256). */
    ecs_id_t *high_ids = malloc(sizeof(ecs_id_t) * N_comps);
    double t0 = now_ms();
    for (int i = 0; i < N_comps; i++) {
        char name[64];
        snprintf(name, sizeof(name), "HiComp%d", i);
        ecs_entity_t e = ecs_entity(world, { .name = name });
        high_ids[i] = e;
    }
    double t1 = now_ms();
    printf("created %d high-id entities: %.2f ms (first id=%llu)\n",
        N_comps, t1 - t0, (unsigned long long)high_ids[0]);

    /* Benchmark: ecs_get_id calls that exercise flecs_components_get.
     * Note: most calls return NULL since ids aren't added to entity, but
     * the lookup itself is what we're measuring. */
    ecs_entity_t e = ecs_new(world);
    srand(42);
    double t2 = now_ms();
    for (int i = 0; i < N_lookups; i++) {
        int idx = rand() % N_comps;
        ecs_get_id(world, e, high_ids[idx]);
    }
    double t3 = now_ms();
    printf("ecs_get_id %d (hi-id lookup): %.2f ms (%.0f/sec)\n",
        N_lookups, t3 - t2, N_lookups / ((t3 - t2) / 1000.0));

    double t4 = now_ms();
    ecs_fini(world);
    double t5 = now_ms();
    printf("fini: %.2f ms\n", t5 - t4);

    free(high_ids);
    printf("=== PASS ===\n");
    return 0;
}