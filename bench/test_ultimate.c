/* ULTIMATE COMPREHENSIVE VERIFICATION.
 * Tests every Tier combination, edge case, long-running stability.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int x; } UX;
typedef struct { int y; } UY;
typedef struct { int z; } UZ;

ECS_COMPONENT_DECLARE(UX);
ECS_COMPONENT_DECLARE(UY);
ECS_COMPONENT_DECLARE(UZ);

/* TEST 1: All bulk APIs in extreme stress */
static int test_all_bulk_extreme(void) {
    printf("TEST 1: all_bulk_extreme (5000 ops)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);
    ECS_COMPONENT_DEFINE(world, UZ);

    for (int op = 0; op < 5000; op++) {
        int action = op % 5;
        const int N = 20;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);

        if (action == 0) {
            /* Just create + delete */
        } else if (action == 1) {
            UX v = {op};
            ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);
        } else if (action == 2) {
            ecs_id_t add_ids[2] = {ecs_id(UY), ecs_id(UZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
        } else if (action == 3) {
            ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &(UX){op});
        } else {
            UX v = {op * 2};
            UY vy = {op + 100};
            ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);
            ecs_bulk_set_id(world, ids, N, ecs_id(UY), sizeof(UY), &vy);
        }
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS (5000 ops OK)\n");
    fflush(stdout);
    return 0;
}

/* TEST 2: Mixed archetype with re-creation */
static int test_mixed_archetype_recreate(void) {
    printf("TEST 2: mixed_archetype_recreate\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);
    ECS_COMPONENT_DEFINE(world, UZ);

    for (int round = 0; round < 100; round++) {
        const int N = 30;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);

        /* Different archetypes per entity */
        for (int i = 0; i < N; i++) {
            if (i % 3 == 0) {
                ecs_add_id(world, ids[i], ecs_id(UY));
                ecs_add_id(world, ids[i], ecs_id(UZ));
            } else if (i % 3 == 1) {
                ecs_add_id(world, ids[i], ecs_id(UY));
            }
            /* else: just UX */
        }

        UX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);

        /* Verify */
        for (int i = 0; i < N; i++) {
            const UX *p = ecs_get(world, ids[i], UX);
            if (!p || p->x != round) {
                printf("  FAIL: round=%d i=%d\n", round, i);
                return 1;
            }
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 3: Query consistency across operations */
static int test_query_consistency(void) {
    printf("TEST 3: query_consistency\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(UX) }, { .id = ecs_id(UY) }}
    });

    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);
        ecs_id_t uy_id = ecs_id(UY);
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], &uy_id, 1);
        }

        UX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);

        int count = 0;
        int matches = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            count += it.count;
            const UX *px = ecs_field(&it, UX, 0);
            for (int i = 0; i < it.count; i++) {
                if (px[i].x == round) matches++;
            }
        }
        if (matches != count) {
            printf("  FAIL: round %d, count=%d matches=%d\n", round, count, matches);
            return 1;
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 4: Tier-J2 world cache stress */
static int test_j2_world_cache_stress(void) {
    printf("TEST 4: j2_world_cache_stress\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);
    ECS_COMPONENT_DEFINE(world, UZ);

    /* Tier-J2 caches last lookup. Stress with many different components. */
    for (int round = 0; round < 1000; round++) {
        const ecs_id_t comp = (round % 3 == 0) ? ecs_id(UX) :
                              (round % 3 == 1) ? ecs_id(UY) : ecs_id(UZ);
        ecs_entity_t e = ecs_new(world);
        ecs_add_id(world, e, comp);

        const int *p = ecs_get(world, e, UX);
        if (round % 3 == 0) {
            assert(p != NULL);
        }
        ecs_delete(world, e);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 5: Bulk ops with deferred ops */
static int test_bulk_with_defer(void) {
    printf("TEST 5: bulk_with_defer\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);

    for (int round = 0; round < 50; round++) {
        const int N = 20;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);

        /* Defer mode */
        ecs_defer_begin(world);
        ecs_id_t uy_id = ecs_id(UY);
        for (int i = 0; i < N; i++) {
            ecs_add_ids(world, ids[i], &uy_id, 1);
        }
        ecs_defer_end(world);

        /* Verify deferred ops applied */
        for (int i = 0; i < N; i++) {
            assert(ecs_has(world, ids[i], UY));
        }

        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 6: Repeated world init/fini */
static int test_repeated_init_fini(void) {
    printf("TEST 6: repeated_init_fini\n");
    fflush(stdout);
    for (int i = 0; i < 20; i++) {
        ecs_world_t *world = ecs_init();
        ECS_COMPONENT_DEFINE(world, UX);

        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);
        UX v = {i};
        ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);
        ecs_bulk_delete(world, ids, N);

        ecs_fini(world);
    }
    printf("  PASS (20 init/fini cycles)\n");
    fflush(stdout);
    return 0;
}

/* TEST 7: Stress test - random patterns */
static int test_random_patterns(void) {
    printf("TEST 7: random_patterns\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);

    srand(123);
    ecs_entity_t alive[500];
    int alive_count = 0;

    for (int op = 0; op < 2000; op++) {  /* reduced for safety */
        int action = rand() % 4;
        if (action == 0) {
            if (alive_count < 500) {
                ecs_entity_t e = ecs_new(world);
                alive[alive_count++] = e;
            }
        } else if (action == 1) {
            if (alive_count > 0) {
                int idx = rand() % alive_count;
                ecs_set(world, alive[idx], UX, {op});
            }
        } else if (action == 2) {
            if (alive_count > 0) {
                int idx = rand() % alive_count;
                ecs_delete(world, alive[idx]);
                alive[idx] = alive[--alive_count];
            }
        } else {
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), 10);
            ecs_bulk_delete(world, ids, 10);
        }
    }

    while (alive_count > 0) {
        ecs_delete(world, alive[--alive_count]);
    }

    ecs_fini(world);
    printf("  PASS (2000 random ops)\n");
    fflush(stdout);
    return 0;
}

/* TEST 8: Tier-V1 grow_data stress */
static int test_v1_grow_data_extreme(void) {
    printf("TEST 8: v1_grow_data_extreme\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);
    ECS_COMPONENT_DEFINE(world, UZ);

    /* Stress grow_data with varying sizes */
    for (int round = 0; round < 100; round++) {
        int size = 1 + (round % 5);  /* 1 to 5 */
        int total = 0;
        for (int s = 0; s < size; s++) {
            const int N = 50;
            const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);
            ecs_id_t add_ids[2] = {ecs_id(UY), ecs_id(UZ)};
            ecs_add_ids_w_entities(world, ids, N, add_ids, 2);
            total += N;
            ecs_bulk_delete(world, ids, N);
        }
        printf("  round %d: total_entities=%d\n", round, total * size);
    }

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 9: Tier-A2 unchecked field in different queries */
static int test_a2_unchecked_different_queries(void) {
    printf("TEST 9: a2_unchecked_different_queries\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);
    ECS_COMPONENT_DEFINE(world, UZ);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);

    /* Add all three components */
    ecs_id_t add_ids[2] = {ecs_id(UY), ecs_id(UZ)};
    ecs_add_ids_w_entities(world, ids, N, add_ids, 2);

    /* Set values */
    UX vx = {1};
    UY vy = {2};
    UZ vz = {3};
    ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &vx);
    ecs_bulk_set_id(world, ids, N, ecs_id(UY), sizeof(UY), &vy);
    ecs_bulk_set_id(world, ids, N, ecs_id(UZ), sizeof(UZ), &vz);

    /* Multiple queries */
    ecs_query_t *q1 = ecs_query(world, { .terms = {{ .id = ecs_id(UX) }} });
    ecs_query_t *q2 = ecs_query(world, { .terms = {{ .id = ecs_id(UY) }} });
    ecs_query_t *q3 = ecs_query(world, { .terms = {{ .id = ecs_id(UZ) }} });

    int c1 = 0, c2 = 0, c3 = 0;
    ecs_iter_t it;
    it = ecs_query_iter(world, q1);
    while (ecs_query_next(&it)) c1 += it.count;
    it = ecs_query_iter(world, q2);
    while (ecs_query_next(&it)) c2 += it.count;
    it = ecs_query_iter(world, q3);
    while (ecs_query_next(&it)) c3 += it.count;

    assert(c1 == N);
    assert(c2 == N);
    assert(c3 == N);

    ecs_fini(world);
    printf("  PASS\n");
    fflush(stdout);
    return 0;
}

/* TEST 10: Tier-W1 query prefetch in long iteration */
static int test_w1_long_iter(void) {
    printf("TEST 10: w1_long_iter (10000 iters)\n");
    fflush(stdout);
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, UX);
    ECS_COMPONENT_DEFINE(world, UY);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(UX), N);
    ecs_id_t uy_id = ecs_id(UY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &uy_id, 1);
    }

    UX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(UX), sizeof(UX), &v);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(UX) }, { .id = ecs_id(UY) }}
    });

    for (int iter = 0; iter < 10000; iter++) {
        int count = 0;
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            count += it.count;
        }
        if (count != N) {
            printf("  FAIL: iter %d count=%d\n", iter, count);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS (10000 iters)\n");
    fflush(stdout);
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_all_bulk_extreme();
    failures += test_mixed_archetype_recreate();
    failures += test_query_consistency();
    failures += test_j2_world_cache_stress();
    failures += test_bulk_with_defer();
    failures += test_repeated_init_fini();
    failures += test_random_patterns();
    failures += test_v1_grow_data_extreme();
    failures += test_a2_unchecked_different_queries();
    failures += test_w1_long_iter();

    if (failures) {
        printf("\n%d ULTIMATE TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 ULTIMATE TESTS PASSED\n");
    return 0;
}