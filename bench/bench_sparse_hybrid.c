/* Tier-A1 sparse-set hybrid BENCHMARK
 *
 * Compares archetype storage (regular ECS_COMPONENT) vs sparse-set storage
 * (Tier-A1.3 LAZY auto-mark / EcsDontFragment) for a 100k entity workload
 * with mixed components:
 *   - 4 "normal" components  -> archetype columns
 *   - 4 "data-only" components -> cr->sparse (Tier-A1 hybrid)
 *
 * The hypothesis: when many entities have the SAME archetype (only data
 * components vary), the Tier-A1 path skips archetype transitions on every
 * set, giving +%20-100 throughput on bulk add/set/remove cycles.
 *
 * Builds against bench/release/v17_flecs_patched.lib.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct { int x; } Pos;
typedef struct { int y; } Vel;
typedef struct { int z; } Hp;
typedef struct { int w; } Mp;

typedef struct { int dx; } DPos;   /* data-only candidate */
typedef struct { int dy; } DVel;   /* data-only candidate */
typedef struct { int dz; } DHp;    /* data-only candidate */
typedef struct { int dw; } DMp;    /* data-only candidate */

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* Variant A: all components go to archetype columns (baseline) */
static double bench_baseline(int n_iters, int n_entities) {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);
    ECS_COMPONENT(world, Hp);
    ECS_COMPONENT(world, Mp);
    ECS_COMPONENT(world, DPos);
    ECS_COMPONENT(world, DVel);
    ECS_COMPONENT(world, DHp);
    ECS_COMPONENT(world, DMp);

    ecs_entity_t *ids = malloc(sizeof(ecs_entity_t) * n_entities);
    for (int i = 0; i < n_entities; i++) ids[i] = ecs_new(world);

    /* warmup */
    for (int i = 0; i < 1000; i++) {
        ecs_add_id(world, ids[i % n_entities], ecs_id(Pos));
        ecs_remove_id(world, ids[i % n_entities], ecs_id(Pos));
    }

    double start = now_ms();
    long total_ops = 0;
    for (int r = 0; r < n_iters; r++) {
        for (int i = 0; i < n_entities; i++) {
            /* add+remove 8 components */
            ecs_add_id(world, ids[i], ecs_id(Pos));
            ecs_add_id(world, ids[i], ecs_id(Vel));
            ecs_add_id(world, ids[i], ecs_id(Hp));
            ecs_add_id(world, ids[i], ecs_id(Mp));
            ecs_add_id(world, ids[i], ecs_id(DPos));
            ecs_add_id(world, ids[i], ecs_id(DVel));
            ecs_add_id(world, ids[i], ecs_id(DHp));
            ecs_add_id(world, ids[i], ecs_id(DMp));

            ecs_remove_id(world, ids[i], ecs_id(Pos));
            ecs_remove_id(world, ids[i], ecs_id(Vel));
            ecs_remove_id(world, ids[i], ecs_id(Hp));
            ecs_remove_id(world, ids[i], ecs_id(Mp));
            ecs_remove_id(world, ids[i], ecs_id(DPos));
            ecs_remove_id(world, ids[i], ecs_id(DVel));
            ecs_remove_id(world, ids[i], ecs_id(DHp));
            ecs_remove_id(world, ids[i], ecs_id(DMp));
            total_ops += 16;
        }
    }
    double elapsed = now_ms() - start;

    free(ids);
    ecs_fini(world);
    return (double)total_ops / (elapsed / 1000.0);
}

/* Variant B: 4 data-only components auto-marked EcsDontFragment (Tier-A1 hybrid) */
static double bench_tier_a1(int n_iters, int n_entities) {
    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    /* The 4 "D*" components will be auto-marked (no hooks) */
    ECS_COMPONENT(world, Pos);    /* baseline archetype */
    ECS_COMPONENT(world, Vel);
    ECS_COMPONENT(world, Hp);
    ECS_COMPONENT(world, Mp);
    ECS_COMPONENT(world, DPos);   /* Tier-A1 sparse */
    ECS_COMPONENT(world, DVel);
    ECS_COMPONENT(world, DHp);
    ECS_COMPONENT(world, DMp);

    /* Verify auto-mark took effect */
    int auto_marked = 0;
    auto_marked += ecs_has_id(world, ecs_id(DPos), EcsDontFragment);
    auto_marked += ecs_has_id(world, ecs_id(DVel), EcsDontFragment);
    auto_marked += ecs_has_id(world, ecs_id(DHp), EcsDontFragment);
    auto_marked += ecs_has_id(world, ecs_id(DMp), EcsDontFragment);
    if (auto_marked != 4) {
        fprintf(stderr, "WARN: auto-mark failed (%d/4)\n", auto_marked);
    }

    ecs_entity_t *ids = malloc(sizeof(ecs_entity_t) * n_entities);
    for (int i = 0; i < n_entities; i++) ids[i] = ecs_new(world);

    /* warmup */
    for (int i = 0; i < 1000; i++) {
        ecs_add_id(world, ids[i % n_entities], ecs_id(Pos));
        ecs_remove_id(world, ids[i % n_entities], ecs_id(Pos));
    }

    double start = now_ms();
    long total_ops = 0;
    for (int r = 0; r < n_iters; r++) {
        for (int i = 0; i < n_entities; i++) {
            /* Same 8 add + 8 remove */
            ecs_add_id(world, ids[i], ecs_id(Pos));
            ecs_add_id(world, ids[i], ecs_id(Vel));
            ecs_add_id(world, ids[i], ecs_id(Hp));
            ecs_add_id(world, ids[i], ecs_id(Mp));
            ecs_add_id(world, ids[i], ecs_id(DPos));   /* sparse path */
            ecs_add_id(world, ids[i], ecs_id(DVel));   /* sparse path */
            ecs_add_id(world, ids[i], ecs_id(DHp));    /* sparse path */
            ecs_add_id(world, ids[i], ecs_id(DMp));    /* sparse path */

            ecs_remove_id(world, ids[i], ecs_id(Pos));
            ecs_remove_id(world, ids[i], ecs_id(Vel));
            ecs_remove_id(world, ids[i], ecs_id(Hp));
            ecs_remove_id(world, ids[i], ecs_id(Mp));
            ecs_remove_id(world, ids[i], ecs_id(DPos));/* sparse path */
            ecs_remove_id(world, ids[i], ecs_id(DVel));/* sparse path */
            ecs_remove_id(world, ids[i], ecs_id(DHp)); /* sparse path */
            ecs_remove_id(world, ids[i], ecs_id(DMp)); /* sparse path */
            total_ops += 16;
        }
    }
    double elapsed = now_ms() - start;

    free(ids);
    ecs_fini(world);
    return (double)total_ops / (elapsed / 1000.0);
}

int main(int argc, char **argv) {
    int n_iters = 100;
    int n_entities = 10000;
    if (argc > 1) n_iters = atoi(argv[1]);
    if (argc > 2) n_entities = atoi(argv[2]);

    printf("=== Tier-A1 sparse-set hybrid BENCH ===\n");
    printf("n_iters=%d, n_entities=%d, ops/iter/entity=%d\n\n",
        n_iters, n_entities, 16);

    /* best-of-3 to reduce noise */
    double b_max = 0, t_max = 0;
    for (int trial = 0; trial < 3; trial++) {
        double b = bench_baseline(n_iters, n_entities);
        double t = bench_tier_a1(n_iters, n_entities);
        if (b > b_max) b_max = b;
        if (t > t_max) t_max = t;
        printf("trial %d: baseline=%.2f M ops/sec | Tier-A1=%.2f M ops/sec | delta=+%5.1f%%\n",
            trial, b / 1e6, t / 1e6, (t / b - 1.0) * 100.0);
    }

    double delta_pct = (t_max / b_max - 1.0) * 100.0;
    printf("\nbest-of-3: baseline=%.2f M | Tier-A1=%.2f M | delta=+%5.1f%%\n",
        b_max / 1e6, t_max / 1e6, delta_pct);
    printf("\nTier-A1 target: +20-100%% (sparse packed-array locality)\n");
    printf("Tier-A1 status: %s\n",
        delta_pct >= 20.0 ? "PASS" : (delta_pct >= 0.0 ? "NEUTRAL" : "REGRESSION"));
    return 0;
}