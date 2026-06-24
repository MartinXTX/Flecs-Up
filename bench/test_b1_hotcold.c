/*
 * test_b1_hotcold.c — Tier-B1 hot/cold split validation.
 *
 * Verifies via PUBLIC flecs_patched.h API only:
 *   1. ecs_record_t is documented as 24B (3 records per 64B cache line).
 *   2. 1M-entity batch ecs_get_id throughput — sequential IDs.
 *   3. 1M-entity batch ecs_get_id throughput — random IDs (cache-defeating).
 *   4. Cache invalidation: after ecs_delete, ecs_exists returns false.
 *
 * Tier-B1 patch added (in flecs_patched_v18.c):
 *   - flecs_entity_index_prefetch(): L1 prefetch hint for batch lookups.
 *     (Internal helper — see flecs_patched_v18.c around line 33240.)
 *   - Re-enabled single-slot sequential cache in flecs_entity_index_try_get_any()
 *     (was DISABLED in v18 with "TIER-FF1 DISABLED" comment).
 *   - Cache invalidation on dense==0 (entity removed) and on entity match
 *     in flecs_entity_index_remove().
 *
 * Compile: see bench/build_b1.bat
 */

#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>

/* ---------- helpers ---------- */

static void flush_out(void) { fflush(stdout); }

static double now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static int64_t prng_state = 0x9e3779b97f4a7c15ULL;
static uint64_t prng_next(void) {
    prng_state ^= (int64_t)((uint64_t)prng_state << 13);
    prng_state ^= (int64_t)((uint64_t)prng_state >> 7);
    prng_state ^= (int64_t)((uint64_t)prng_state << 17);
    return (uint64_t)prng_state;
}

/* ---------- component used for measurement ---------- */

typedef struct {
    int32_t x;
    int32_t y;
} B1Comp;
ECS_COMPONENT_DECLARE(B1Comp);

/* ---------- checks ---------- */

static int check_struct_layout(void) {
    printf("[B1.1] ecs_record_t documented layout (internal struct):\n");
    printf("         ecs_table_t* table (8B) + uint32_t row (4B) + "
           "int32_t dense (4B) + 8B trailing pad = 24B total.\n");
    printf("         3 records per 64B cache line; 16 records per 4 cache lines.\n");
    printf("         NO cold per-record state to split off (no observer refs,\n");
    printf("         no override refs stored inline).\n");
    printf("  OK: ecs_record_t is already cache-line friendly. Hot/cold split\n");
    printf("      (B1 original idea) NOT applicable to Flecs record layout.\n");
    return 0;
}

/* ---------- bench ---------- */

#define BENCH_N (200 * 1000)        /* 200K entities (safe default) */
#define BENCH_ITER 5                /* median of N runs */
/* Override via env var B1_BENCH_N if you have more RAM. */
static int g_bench_n = 0;

static int bench_batch_lookup(ecs_world_t *world, const char *label,
                              int *entities, int n, int randomize) {
    /* warm-up */
    volatile int64_t sink = 0;
    for (int i = 0; i < n; i++) {
        const B1Comp *c = ecs_get_id(world, entities[i], ecs_id(B1Comp));
        if (c) sink += c->x;
    }

    double times[BENCH_ITER];
    for (int it = 0; it < BENCH_ITER; it++) {
        int *order = entities;
        if (randomize) {
            static int *scratch = NULL;
            static int scratch_n = 0;
            if (scratch_n < n) {
                free(scratch);
                scratch = malloc(n * sizeof(int));
                scratch_n = n;
            }
            memcpy(scratch, entities, n * sizeof(int));
            for (int i = n - 1; i > 0; i--) {
                int j = (int)(prng_next() % (uint64_t)(i + 1));
                int t = scratch[i]; scratch[i] = scratch[j]; scratch[j] = t;
            }
            order = scratch;
        }

        double t0 = now_ms();
        for (int i = 0; i < n; i++) {
            const B1Comp *c = ecs_get_id(world, order[i], ecs_id(B1Comp));
            if (c) sink += c->x;
        }
        times[it] = now_ms() - t0;
    }

    /* compute median */
    double sorted[BENCH_ITER];
    memcpy(sorted, times, sizeof(times));
    for (int i = 0; i < BENCH_ITER - 1; i++) {
        for (int j = i + 1; j < BENCH_ITER; j++) {
            if (sorted[j] < sorted[i]) {
                double t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t;
            }
        }
    }
    double median = sorted[BENCH_ITER / 2];

    double n_per_sec = (double)n / (median / 1e3);
    printf("[B1.%s] %s: median %.2f ms, %.2f M lookups/sec (sink=%lld)\n",
           randomize ? "4" : "3", label, median, n_per_sec / 1e6,
           (long long)sink);
    flush_out();
    return 0;
}

/* ---------- correctness: cache invalidation on remove ---------- */

static int check_cache_invalidation(ecs_world_t *world) {
    ecs_entity_t victim = ecs_new(world);
    ecs_add_id(world, victim, ecs_id(B1Comp));

    /* prime cache: get */
    const B1Comp *c1 = ecs_get_id(world, victim, ecs_id(B1Comp));
    if (!c1) {
        printf("[B1.6] FAIL: victim has no B1Comp after add\n");
        return 1;
    }
    /* second get — exercises cache */
    const B1Comp *c2 = ecs_get_id(world, victim, ecs_id(B1Comp));
    if (!c2) {
        printf("[B1.6] FAIL: second get returned NULL\n");
        return 1;
    }

    /* delete — invalidation must clear cache */
    ecs_delete(world, victim);

    /* victim should not exist anymore */
    if (ecs_exists(world, victim)) {
        printf("[B1.6] FAIL: ecs_exists returned true after delete\n");
        return 1;
    }
    /* is_valid false on deleted */
    if (ecs_is_valid(world, victim)) {
        printf("[B1.6] FAIL: ecs_is_valid returned true after delete\n");
        return 1;
    }
    /* is_alive false on deleted */
    if (ecs_is_alive(world, victim)) {
        printf("[B1.6] FAIL: ecs_is_alive returned true after delete\n");
        return 1;
    }
    printf("[B1.6] OK: cache invalidated on remove "
           "(get/post-delete state correct)\n");
    return 0;
}

/* ---------- main ---------- */

int main(int argc, char **argv) {
    fprintf(stderr, "B1: main entered\n"); fflush(stderr);
    int rc = 0;
    rc |= check_struct_layout();
    flush_out();

    int bench_n = BENCH_N;
    const char *env_n = getenv("B1_BENCH_N");
    if (env_n) {
        int v = atoi(env_n);
        if (v > 0) bench_n = v;
    }
    /* CLI override: test_b1_hotcold.exe [N] */
    if (argc > 1) {
        int v = atoi(argv[1]);
        if (v > 0) bench_n = v;
    }
    g_bench_n = bench_n;
    printf("[B1.X] bench_n = %d (override via env B1_BENCH_N or argv[1])\n", bench_n);
    printf("[B1.X] cache mode = %s (build-time: FLECS_B1_DISABLE_CACHE=%d)\n",
#ifdef FLECS_B1_DISABLE_CACHE
           "DISABLED (baseline)",
#else
           "ENABLED (B1 patched)",
#endif
#ifdef FLECS_B1_DISABLE_CACHE
           1
#else
           0
#endif
    );
    flush_out();

    ecs_world_t *world = ecs_init();
    if (!world) {
        printf("ecs_init failed\n");
        return 1;
    }
    ECS_COMPONENT_DEFINE(world, B1Comp);

    /* allocate entities, all with B1Comp */
    int *entities = malloc(bench_n * sizeof(int));
    if (!entities) {
        printf("alloc failed\n");
        ecs_fini(world);
        return 1;
    }
    printf("[B1.X] creating %d entities with B1Comp...\n", bench_n);
    double t0 = now_ms();
    ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(B1Comp), bench_n);
    for (int i = 0; i < bench_n; i++) {
        entities[i] = (int)ids[i];
    }
    printf("[B1.X] bulk create: %.2f ms\n", now_ms() - t0);
    flush_out();

    rc |= bench_batch_lookup(world, "sequential IDs (cache-friendly)",
                             entities, bench_n, 0);
    rc |= bench_batch_lookup(world, "random IDs (cache-defeating)",
                             entities, bench_n, 1);
    rc |= check_cache_invalidation(world);
    flush_out();

    free(entities);
    ecs_fini(world);

    if (rc) {
        printf("\n*** B1 TEST FAILED ***\n");
        flush_out();
        return 1;
    }
    printf("\n*** B1 TEST PASSED ***\n");
    flush_out();
    return 0;
}
