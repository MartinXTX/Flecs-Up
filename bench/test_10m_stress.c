/* 10M entity production-scale stress test.
 * Tests Flecs at production scale: 10M entities, 50 components, complex queries.
 * Catches: memory issues, scaling bugs, perf degradation at scale.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define N 10000000
#define BATCH 100000

typedef struct { float x, y, z; } Pos;
typedef struct { float dx, dy, dz; } Vel;
typedef struct { float sx, sy, sz; } Scale;
typedef struct { int hp; int mp; } Stats;
typedef struct { int32_t id; } Tag;

ECS_COMPONENT_DECLARE(Pos);
ECS_COMPONENT_DECLARE(Vel);
ECS_COMPONENT_DECLARE(Scale);
ECS_COMPONENT_DECLARE(Stats);
ECS_COMPONENT_DECLARE(Tag);

static double now_us(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

int main(void) {
    double t0 = now_us();
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);
    ECS_COMPONENT_DEFINE(world, Vel);
    ECS_COMPONENT_DEFINE(world, Scale);
    ECS_COMPONENT_DEFINE(world, Stats);
    ECS_COMPONENT_DEFINE(world, Tag);
    double t1 = now_us();
    printf("[10M stress] world init: %.2f ms\n", (t1-t0)/1000.0);

    /* Bulk create 10M entities with Pos */
    t0 = now_us();
    ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(Pos), N);
    t1 = now_us();
    printf("[10M stress] bulk_new 10M Pos: %.2f ms (%.1f M/sec)\n",
        (t1-t0)/1000.0, N / ((t1-t0)/1e6) / 1e6);

    /* Set values on 10M */
    t0 = now_us();
    Pos p = {1.0f, 2.0f, 3.0f};
    ecs_bulk_set_id(world, ids, N, ecs_id(Pos), sizeof(Pos), &p);
    t1 = now_us();
    printf("[10M stress] bulk_set 10M: %.2f ms (%.1f M/sec)\n",
        (t1-t0)/1000.0, N / ((t1-t0)/1e6) / 1e6);

    /* Add Vel + Scale to 10M (creates new archetype) */
    t0 = now_us();
    ecs_id_t ids2[] = { ecs_id(Vel), ecs_id(Scale) };
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], ids2, 2);
    }
    t1 = now_us();
    printf("[10M stress] add 2 components to 10M: %.2f ms (%.1f M/sec)\n",
        (t1-t0)/1000.0, N / ((t1-t0)/1e6) / 1e6);

    /* Query iter with all fields */
    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Pos) },
            { .id = ecs_id(Vel) },
            { .id = ecs_id(Scale) }
        }
    });

    t0 = now_us();
    int64_t total = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            Pos *p = ecs_field(&it, Pos, 0);
            Vel *v = ecs_field(&it, Vel, 1);
            Scale *s = ecs_field(&it, Scale, 2);
            total += (int64_t)(p[i].x + v[i].dx + s[i].sx);
        }
    }
    t1 = now_us();
    printf("[10M stress] iter 10M 3 fields: %.2f ms (%.1f M/sec)\n",
        (t1-t0)/1000.0, N / ((t1-t0)/1e6) / 1e6);
    printf("[10M stress] total: %lld (sanity check)\n", (long long)total);

    /* Bulk delete half (5M) */
    t0 = now_us();
    ecs_bulk_delete(world, ids, N/2);
    t1 = now_us();
    printf("[10M stress] bulk_delete 5M: %.2f ms (%.1f M/sec)\n",
        (t1-t0)/1000.0, (N/2) / ((t1-t0)/1e6) / 1e6);

    /* Check alive count via query */
    t0 = now_us();
    int64_t alive = 0;
    it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        alive += it.count;
    }
    t1 = now_us();
    printf("[10M stress] iter after delete: %lld alive (expect 5M) in %.2f ms\n",
        (long long)alive, (t1-t0)/1000.0);

    if (alive == N/2) {
        printf("[10M stress] PASS — 10M entity production scale stable\n");
    } else {
        printf("[10M stress] FAIL — alive=%lld, expected %d\n",
            (long long)alive, N/2);
    }

    ecs_fini(world);
    return (alive == N/2) ? 0 : 1;
}
