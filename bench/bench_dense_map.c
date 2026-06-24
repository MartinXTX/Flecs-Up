/* TIER-V17 P0-3 dense_map bench
 *
 * Exercises id_index_hi by using components with high IDs (>= 16).
 * We create entities + use pair relations which produce high-id hashes.
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
typedef struct { int x; } Velocity;
typedef struct { int x; } Acceleration;

int main(int argc, char **argv) {
    int N = (argc > 1) ? atoi(argv[1]) : 100000;
    int seed = (argc > 2) ? atoi(argv[2]) : 42;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== TIER-V17 P0-3 dense_map bench ===\n");
    printf("N=%d seed=%d\n", N, seed);

    ecs_world_t *world = ecs_init();
    if (!world) { printf("FAIL: ecs_init\n"); return 1; }

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Acceleration);
    printf("Position id=%llu\n", (unsigned long long)ecs_id(Position));

    /* Create many high-id component records via pair relations.
     * Each pair (rel, tgt) produces a unique id >= FLECS_HI_ID_RECORD_ID. */
    double t0 = now_ms();
    ecs_entity_t rel = ecs_new(world);
    for (int i = 0; i < 200; i++) {
        ecs_entity_t tgt = ecs_new(world);
        ecs_id_t pair_id = ecs_pair(rel, tgt);
        ecs_entity_t e = ecs_new(world);
        /* Add pair to entity to ensure component record exists in id_index_hi */
        ecs_add_id(world, e, pair_id);
    }
    double t1 = now_ms();
    printf("create 200 pair ids: %.2f ms\n", t1 - t0);

    /* Test the dense_map get path via ecs_get_id on Position */
    ecs_entity_t *es = ecs_bulk_new(world, Position, N);
    if (!es) { printf("FAIL: bulk_new\n"); return 1; }
    printf("after bulk_new, first id=%llu\n", (unsigned long long)es[0]);

    /* Lookup test */
    srand(seed);
    int hits = 0;
    double t2 = now_ms();
    for (int i = 0; i < N; i++) {
        int idx = rand() % N;
        Position *p = ecs_get_id(world, es[idx], ecs_id(Position));
        if (p) hits++;
    }
    double t3 = now_ms();
    printf("get_id %d lookups: %.2f ms (%.0f/sec), hits=%d\n",
        N, t3 - t2, N / ((t3 - t2) / 1000.0), hits);

    /* Cleanup */
    double t4 = now_ms();
    ecs_fini(world);
    double t5 = now_ms();
    printf("fini: %.2f ms\n", t5 - t4);

    printf("=== PASS ===\n");
    return 0;
}