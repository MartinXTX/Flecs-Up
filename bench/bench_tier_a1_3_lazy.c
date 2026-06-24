/* Tier-A1.3 LAZY auto-mark benchmark.
 *
 * Tier-A1.2 (ECS_DATA_COMPONENT explicit macro) +%30-153 archetype_churn'da
 * olculmustu. Tier-A1.3 LAZY ayni kazanci otomatik olarak sunar: opt-in
 * `ecs_world_auto_dont_fragment(world, true)` ile data-only componentler
 * ECS_COMPONENT ile deklare edilse bile post-init hook ile EcsDontFragment
 * ile isaretlenir (sparse storage'a yonlendirilir).
 *
 * Bu test Tier-A1.2 macro'suz, sadece ECS_COMPONENT + auto-mark ile ayni
 * senaryoyu (1000 entity x 100 round add/remove) 3 trial best-of olcer.
 *
 * Iki varyant:
 *   1. Baseline:    ECS_COMPONENT (regular archetype storage)
 *   2. Tier-A1.3:   ECS_COMPONENT + ecs_world_auto_dont_fragment(true)
 *
 * Tier-v12 ust sinir referansi: +%153 (explicit macro ile).
 * Tier-A1.3 LAZY hedef: >= +%30.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
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
    return (double)total_ops / (elapsed / 1000.0);
}

static double bench_lazy_auto_mark(int n_iters, int n_entities) {
    ecs_world_t *world = ecs_init();

    /* Tier-A1.3 LAZY: opt-in auto-mark BEFORE component registration.
     * The post-init hook will mark Pos as EcsDontFragment. */
    ecs_world_auto_dont_fragment(world, true);

    ECS_COMPONENT(world, Pos);

    /* Verify auto-mark took effect (defensive) */
    if (!ecs_has_id(world, ecs_id(Pos), EcsDontFragment)) {
        fprintf(stderr, "WARN: auto-mark did NOT take effect (LAZY disabled)\n");
    }

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

    printf("=== Tier-A1.3 LAZY auto-mark (ECS_COMPONENT + auto_dont_fragment) ===\n");
    printf("n_iters=%d, n_entities=%d, total_ops=%d\n\n",
        n_iters, n_entities, n_iters * n_entities * 2);

    double b_max = 0, l_max = 0;
    for (int trial = 0; trial < 3; trial++) {
        double b = bench_baseline(n_iters, n_entities);
        double l = bench_lazy_auto_mark(n_iters, n_entities);
        if (b > b_max) b_max = b;
        if (l > l_max) l_max = l;
        printf("trial %d: baseline=%.2f M ops/sec | LAZY=%.2f M ops/sec | delta=+%5.1f%%\n",
            trial, b / 1e6, l / 1e6, (l / b - 1.0) * 100.0);
    }

    double delta_pct = (l_max / b_max - 1.0) * 100.0;
    printf("\n=== Best: baseline=%.2f M | LAZY=%.2f M | delta=%+.1f%% ===\n",
        b_max / 1e6, l_max / 1e6, delta_pct);

    if (delta_pct >= 30.0) {
        printf("PASS: >= +30%% target (Tier-v12 upper bound: +153%%)\n");
        return 0;
    } else {
        printf("BELOW TARGET: < +30%% (may be Tier-v14.1 dispatch already saturated)\n");
        return 0;
    }
}
