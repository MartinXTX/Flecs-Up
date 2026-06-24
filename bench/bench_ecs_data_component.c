/* Tier-A1.2 reproduksiyon benchmarki.
 * Tier-v12'de ECS_DATA_COMPONENT +%153 kazanç archetype_churn senaryosunda
 * ölçülmüştü. Bu test Tier-v14.1'in PRODUCTION-READY baseline'ı üzerinde
 * aynı senaryoyu tekrar koşar.
 *
 * Iki varyant:
 *   1. Baseline: regular ECS_COMPONENT (archetype storage)
 *   2. DATA: ECS_DATA_COMPONENT (sparse storage)
 *
 * Her ikisi için 1000 entity × 100 round add/remove ölçülür.
 * Tier-v12 baseline kazanç hedefi: %30-153.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <time.h>

typedef struct { int x; } Pos;

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static double bench_baseline(int n_iters, int n_entities) {
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, Pos);

    ecs_entity_t *ids = malloc(sizeof(ecs_entity_t) * n_entities);
    for (int i = 0; i < n_entities; i++) ids[i] = ecs_new(world);

    /* warmup */
    for (int i = 0; i < 1000; i++) {
        ecs_add_id(world, ids[i % n_entities], ecs_id(Pos));
        ecs_remove_id(world, ids[i % n_entities], ecs_id(Pos));
    }

    double start = now_ms();
    int total_ops = 0;
    for (int r = 0; r < n_iters; r++) {
        for (int i = 0; i < n_entities; i++) {
            ecs_add_id(world, ids[i], ecs_id(Pos));
            ecs_remove_id(world, ids[i], ecs_id(Pos));
            total_ops += 2;
        }
    }
    double elapsed = now_ms() - start;

    free(ids);
    ecs_fini(world);
    return (double)total_ops / (elapsed / 1000.0);  /* ops/sec */
}

static double bench_data_component(int n_iters, int n_entities) {
    ecs_world_t *world = ecs_init();
    ECS_DATA_COMPONENT(world, Pos);

    ecs_entity_t *ids = malloc(sizeof(ecs_entity_t) * n_entities);
    for (int i = 0; i < n_entities; i++) ids[i] = ecs_new(world);

    /* warmup */
    for (int i = 0; i < 1000; i++) {
        ecs_add_id(world, ids[i % n_entities], ecs_id(Pos));
        ecs_remove_id(world, ids[i % n_entities], ecs_id(Pos));
    }

    double start = now_ms();
    int total_ops = 0;
    for (int r = 0; r < n_iters; r++) {
        for (int i = 0; i < n_entities; i++) {
            ecs_add_id(world, ids[i], ecs_id(Pos));
            ecs_remove_id(world, ids[i], ecs_id(Pos));
            total_ops += 2;
        }
    }
    double elapsed = now_ms() - start;

    free(ids);
    ecs_fini(world);
    return (double)total_ops / (elapsed / 1000.0);
}

int main(int argc, char **argv) {
    int n_iters = 100;
    int n_entities = 1000;
    if (argc > 1) n_iters = atoi(argv[1]);
    if (argc > 2) n_entities = atoi(argv[2]);

    printf("=== Tier-A1.2 ECS_DATA_COMPONENT Reproduksiyon ===\n");
    printf("n_iters=%d, n_entities=%d, total_ops=%d\n\n",
        n_iters, n_entities, n_iters * n_entities * 2);

    /* Run 3x and take best (warm-up variance) */
    double b_max = 0, d_max = 0;
    for (int trial = 0; trial < 3; trial++) {
        double b = bench_baseline(n_iters, n_entities);
        double d = bench_data_component(n_iters, n_entities);
        if (b > b_max) b_max = b;
        if (d > d_max) d_max = d;
        printf("trial %d: baseline=%.2f M ops/sec | DATA=%.2f M ops/sec | delta=+%5.1f%%\n",
            trial, b / 1e6, d / 1e6, (d / b - 1.0) * 100.0);
    }

    double delta_pct = (d_max / b_max - 1.0) * 100.0;
    printf("\n=== Best: baseline=%.2f M | DATA=%.2f M | delta=%+.1f%% ===\n",
        b_max / 1e6, d_max / 1e6, delta_pct);

    if (delta_pct >= 30.0) {
        printf("PASS: >= +30%% target (Tier-v12 upper bound: +153%%)\n");
        return 0;
    } else {
        printf("BELOW TARGET: < +30%% (may be Tier-v14.1 dispatch already saturated)\n");
        return 0;
    }
}