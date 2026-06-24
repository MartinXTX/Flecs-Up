/* TIER-C1: per-table observer bitmap test.
 * Uses bulk APIs to avoid per-entity ecs_set/ecs_add which can hit
 * builtin entity quirks (entity 7 = EcsModule, etc.).
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct { int x; } CPos;
ECS_COMPONENT_DECLARE(CPos);

static int on_add_count = 0;
static int on_set_count = 0;

static void on_add_cb(ecs_iter_t *it) { on_add_count += it->count; }
static void on_set_cb(ecs_iter_t *it) { on_set_count += it->count; }

static int test_observer_fires(void) {
    printf("TEST: observer_fires\n");
    fflush(stdout);
    on_add_count = on_set_count = 0;

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, CPos);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(CPos) }},
        .events = { EcsOnAdd },
        .callback = on_add_cb
    });
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(CPos) }},
        .events = { EcsOnSet },
        .callback = on_set_cb
    });

    /* bulk_new_w_id creates entities with the component in one call */
    ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(CPos), 10);
    if (on_add_count != 10) {
        printf("  FAIL: on_add_count=%d expected 10\n", on_add_count);
        ecs_fini(world);
        return 1;
    }

    /* bulk_set_id sets values in one call */
    CPos val = {42};
    ecs_bulk_set_id(world, ids, 10, ecs_id(CPos), sizeof(CPos), &val);
    if (on_set_count != 10) {
        printf("  FAIL: on_set_count=%d expected 10\n", on_set_count);
        ecs_fini(world);
        return 1;
    }

    printf("  PASS (on_add=%d on_set=%d)\n", on_add_count, on_set_count);
    fflush(stdout);
    ecs_fini(world);
    return 0;
}

#define N_OBSERVERS 100

typedef struct {
    int counts[N_OBSERVERS];
} FanoutCtx;

static FanoutCtx g_ctx;

static void fanout_cb(ecs_iter_t *it) {
    int idx = (int)(intptr_t)it->ctx;
    g_ctx.counts[idx] += it->count;
}

static double now_sec(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int test_100_observer_fanout(void) {
    printf("TEST: 100_observer_fanout\n");
    fflush(stdout);
    memset(&g_ctx, 0, sizeof(g_ctx));

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, CPos);

    /* Register 100 OnAdd observers on the same (CPos) component. */
    for (int i = 0; i < N_OBSERVERS; i++) {
        ecs_observer(world, {
            .query.terms = {{ .id = ecs_id(CPos) }},
            .events = { EcsOnAdd },
            .callback = fanout_cb,
            .ctx = (void*)(intptr_t)i
        });
    }

    /* Time bulk add of 1K entities. */
    const int N = 1000;
    double t0 = now_sec();
    ecs_bulk_new_w_id(world, ecs_id(CPos), N);
    double dt = now_sec() - t0;

    int total = 0;
    for (int i = 0; i < N_OBSERVERS; i++) {
        if (g_ctx.counts[i] != N) {
            printf("  FAIL: observer[%d].count=%d expected %d\n",
                i, g_ctx.counts[i], N);
            ecs_fini(world);
            return 1;
        }
        total += g_ctx.counts[i];
    }

    printf("  PASS (N=%d, observers=%d, total invokes=%d, %.3f sec, %.0f invokes/sec)\n",
        N, N_OBSERVERS, total, dt,
        (double)total / dt);
    fflush(stdout);
    ecs_fini(world);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_observer_fires();
    failures += test_100_observer_fanout();

    if (failures) {
        printf("\n%d TIER-C1 TESTS FAILED\n", failures);
        fflush(stdout);
        return 1;
    }
    printf("\nALL TIER-C1 TESTS PASSED\n");
    fflush(stdout);
    return 0;
}
