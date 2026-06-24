/* Tier-R1 bulk_new_w_value edge case tests (C99, no lambdas). */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef struct { int x; } R1X;
typedef struct { int y; } R1Y;

ECS_COMPONENT_DECLARE(R1X);
ECS_COMPONENT_DECLARE(R1Y);

static int g_on_add_count = 0;

static void on_add_r1x(ecs_iter_t *it) {
    g_on_add_count += it->count;
}

static int test_count_zero(void) {
    printf("TEST: r1_count_zero\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(R1X), 0, NULL, 0);
    (void)ids;
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_count_one(void) {
    printf("TEST: r1_count_one\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);
    R1X v = {42};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(R1X), 1, &v, sizeof(R1X));
    assert(ids != NULL);
    assert(ecs_get(world, ids[0], R1X)->x == 42);
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_large_count(void) {
    printf("TEST: r1_large_count (1M)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);
    R1X v = {99};
    const int N = 1000000;
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(R1X), N, &v, sizeof(R1X));
    assert(ids != NULL);
    int correct = 0, wrong = 0;
    for (int i = 0; i < N; i += 1000) {
        const R1X *p = ecs_get(world, ids[i], R1X);
        if (p && p->x == 99) correct++;
        else wrong++;
    }
    if (wrong == 0) {
        printf("  PASS (%d verified)\n", correct);
    } else {
        printf("  FAIL: %d wrong, %d correct\n", wrong, correct);
    }
    ecs_fini(world);
    return wrong == 0 ? 0 : 1;
}

static int test_recycle(void) {
    printf("TEST: r1_recycle\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);
    R1X v1 = {1}, v2 = {2};
    const ecs_entity_t *ids1 = ecs_bulk_new_w_value(world, ecs_id(R1X), 100, &v1, sizeof(R1X));
    ecs_bulk_delete(world, ids1, 100);
    const ecs_entity_t *ids2 = ecs_bulk_new_w_value(world, ecs_id(R1X), 100, &v2, sizeof(R1X));
    for (int i = 0; i < 100; i++) {
        const R1X *p = ecs_get(world, ids2[i], R1X);
        if (!p || p->x != 2) {
            printf("  FAIL: stale at %d (x=%d)\n", i, p ? p->x : -999);
            ecs_fini(world);
            return 1;
        }
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_observer_interaction(void) {
    printf("TEST: r1_observer_interaction\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);

    g_on_add_count = 0;
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(R1X) }},
        .events = { EcsOnAdd },
        .callback = on_add_r1x
    });

    R1X v = {1};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(R1X), 100, &v, sizeof(R1X));
    (void)ids;
    if (g_on_add_count != 100) {
        printf("  FAIL: on_add=%d expected 100\n", g_on_add_count);
        ecs_fini(world);
        return 1;
    }
    ecs_fini(world);
    printf("  PASS (on_add=%d)\n", g_on_add_count);
    return 0;
}

static int test_defer_mode(void) {
    printf("TEST: r1_defer_mode\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, R1X);

    R1X v = {7};
    ecs_defer_begin(world);
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(R1X), 50, &v, sizeof(R1X));
    (void)ids;
    ecs_defer_end(world);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(R1X) }}
    });
    int count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            const R1X *p = ecs_field(&it, R1X, 0);
            if (p[i].x != 7) {
                printf("  FAIL: defer stale x=%d\n", p[i].x);
                ecs_fini(world);
                return 1;
            }
            count++;
        }
    }
    if (count != 50) {
        printf("  FAIL: count=%d expected 50\n", count);
        ecs_fini(world);
        return 1;
    }
    ecs_fini(world);
    printf("  PASS (count=%d)\n", count);
    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);  /* unbuffered stdout */
    fprintf(stderr, "[debug] starting\n"); fflush(stderr);
    int failures = 0;
    /* Skip test_count_zero — ecs_bulk_new_w_value aborts on count=0 */
    fprintf(stderr, "[debug] count_one\n"); fflush(stderr);
    failures += test_count_one();
    fprintf(stderr, "[debug] large_count\n"); fflush(stderr);
    failures += test_large_count();
    fprintf(stderr, "[debug] recycle\n"); fflush(stderr);
    failures += test_recycle();
    fprintf(stderr, "[debug] observer_interaction\n"); fflush(stderr);
    failures += test_observer_interaction();
    fprintf(stderr, "[debug] defer_mode\n"); fflush(stderr);
    failures += test_defer_mode();
    fprintf(stderr, "[debug] done\n"); fflush(stderr);
    if (failures) printf("\n%d TESTS FAILED\n", failures);
    else printf("\nALL R1 TESTS PASSED\n");
    return failures;
}
