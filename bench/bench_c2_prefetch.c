/*
 * bench_c2_prefetch.c — Tier-C2 (tam surum) multi-archetype iter benchmark
 *
 * Stresses the flecs_table_cache_next_ path which is the most-traversed
 * linked-list iteration primitive in Flecs (used by ecs_each, observers,
 * component-record walk, etc.).
 *
 * The benchmark creates N entities spread across M archetypes (each
 * archetype adds a unique tag), then walks each archetype one-by-one and
 * reads a few bytes from each entity's first column data.  This matches
 * real workloads where a system touches every entity but only a subset of
 * archetypes per frame.
 *
 * Compile-time switch:
 *   FLECS_C2_PREFETCH=1 -> enables Tier-C2 prefetch in flecs_table_cache_next_
 *   (otherwise no-op, used as baseline)
 *
 * Usage:
 *   bench_c2_prefetch.exe [archetypes] [entities_per_arch]
 *   defaults: 64 archetypes, 256 entities per archetype
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int x;
    float y;
    double z;
} Position;

typedef struct {
    int w;
} TagA;
typedef struct {
    int w;
} TagB;
typedef struct {
    int w;
} TagC;
typedef struct {
    int w;
} TagD;

#define TAG_COUNT 256

/* Each archetype gets Position + TagA + ... + TagK (variable K).
 * To get N archetypes we just compose tags from TAG_COUNT slots. */
typedef struct {
    int data;
} Extra;

typedef struct {
    ecs_entity_t tags[TAG_COUNT];
} BenchWorld;

static
double now_sec(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Warm-up: walk each archetype, sum entities to keep loop from being
 * optimized out. Repeats `reps` times so timing is meaningful. */
static
int64_t walk_archetypes(ecs_world_t *world, ecs_query_t *q, int reps) {
    int64_t checksum = 0;
    for (int r = 0; r < reps; r++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Position *p = ecs_field(&it, Position, 0);
            for (int i = 0; i < it.count; i++) {
                /* Touch the data so prefetch actually matters. */
                checksum += p[i].x;
                checksum += (int)(p[i].y * 7.0f);
                checksum += (int)p[i].z;
            }
        }
    }
    return checksum;
}

int main(int argc, char **argv) {
    int archetypes = 128;
    int per_arch = 512;
    int inner_reps = 200;

    if (argc > 1) archetypes = atoi(argv[1]);
    if (argc > 2) per_arch = atoi(argv[2]);
    if (argc > 3) inner_reps = atoi(argv[3]);

    if (archetypes < 1) archetypes = 1;
    if (archetypes > TAG_COUNT) archetypes = TAG_COUNT;

    printf("bench_c2_prefetch: archetypes=%d per_arch=%d total_entities=%d\n",
        archetypes, per_arch, archetypes * per_arch);
#ifdef FLECS_C2_PREFETCH
    printf("  build mode: FLECS_C2_PREFETCH=ON (Tier-C2 tam surum enabled)\n");
#else
    printf("  build mode: FLECS_C2_PREFETCH=OFF (baseline)\n");
#endif

    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_TAG(world, TagA);
    ECS_TAG(world, TagB);
    ECS_TAG(world, TagC);
    ECS_TAG(world, TagD);
    ecs_entity_t extra_tags[TAG_COUNT];
    for (int i = 0; i < TAG_COUNT; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Tag%d", i);
        extra_tags[i] = ecs_new(world);
        ecs_set_name(world, extra_tags[i], name);
    }

    /* Seed archetypes: archetype[a] has Position + extra_tags[a % TAG_COUNT].
     * Each archetype only differs by one tag — guarantees N distinct archetypes. */
    ecs_entity_t *all_ents = malloc(sizeof(ecs_entity_t) * archetypes * per_arch);
    for (int a = 0; a < archetypes; a++) {
        for (int e = 0; e < per_arch; e++) {
            ecs_entity_t ent = ecs_new(world);
            ecs_add(world, ent, Position);
            /* Add 2-4 unique tags to spread across archetypes */
            int tag_count = 2 + (a % 3);
            for (int t = 0; t < tag_count && t < TAG_COUNT; t++) {
                ecs_add_id(world, ent, extra_tags[(a * 7 + t * 13) % TAG_COUNT]);
            }
            ecs_set(world, ent, Position, {a, (float)a * 0.5f, (double)a * 1.25});
            all_ents[a * per_arch + e] = ent;
        }
    }

    printf("  created world with %d archetypes, %d total entities\n",
        archetypes, archetypes * per_arch);

    ecs_query_t *q = ecs_query(world, { .terms = {{ .id = ecs_id(Position) }} });

    /* Warm up caches and verify correctness. */
    int64_t cs = walk_archetypes(world, q, inner_reps);
    printf("  warm-up checksum: %lld\n", (long long)cs);

    /* Iter benchmark: 5 trials, each runs `inner_reps` full sweeps. */
    const int trials = 5;
    double total_sec = 0.0;
    double best_sec = 1e9;
    int64_t last_cs = 0;
    for (int t = 0; t < trials; t++) {
        double t0 = now_sec();
        last_cs = walk_archetypes(world, q, inner_reps);
        double t1 = now_sec();
        double dt = t1 - t0;
        total_sec += dt;
        if (dt < best_sec) best_sec = dt;
        printf("  trial %d (x%d reps): %.4f sec (checksum=%lld)\n",
            t, inner_reps, dt, (long long)last_cs);
    }
    double mean_sec = total_sec / trials;
    double per_iter_us = (mean_sec * 1e6) / (double)inner_reps;
    printf("  mean: %.4f sec  best: %.4f sec  per_iter: %.2f us\n",
        mean_sec, best_sec, per_iter_us);
    printf("  final checksum: %lld (must match warm-up = %lld)\n",
        (long long)last_cs, (long long)cs);

    if (last_cs != cs) {
        fprintf(stderr, "ERROR: checksum mismatch! prefetch corrupted data?\n");
        free(all_ents);
        ecs_fini(world);
        return 2;
    }

    free(all_ents);
    ecs_fini(world);
    return 0;
}
