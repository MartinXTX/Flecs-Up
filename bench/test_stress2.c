/* DEEP TIER VERIFICATION — 100k scale, multi-world, batched APIs. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct { int x; } SX;
typedef struct { int y; } SY;
typedef struct { int z; } SZ;
typedef struct { float a, b, c, d; } SBig;

ECS_COMPONENT_DECLARE(SX);
ECS_COMPONENT_DECLARE(SY);
ECS_COMPONENT_DECLARE(SZ);
ECS_COMPONENT_DECLARE(SBig);

/* TEST 1: 100k entity bulk operations */
static int test_100k_bulk(void) {
    printf("TEST 1: 100k_bulk_stable\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SX);

    const int N = 100000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SX), N);

    SX v = {42};
    ecs_bulk_set_id(world, ids, N, ecs_id(SX), sizeof(SX), &v);

    /* Sample verification */
    int errors = 0;
    for (int i = 0; i < N; i += 10000) {
        const SX *p = ecs_get(world, ids[i], SX);
        if (!p || p->x != 42) errors++;
    }
    if (errors > 0) {
        printf("  FAIL: %d errors\n", errors);
        return 1;
    }

    /* Bulk delete all */
    ecs_bulk_delete(world, ids, N);

    /* Verify all dead */
    int alive = 0;
    for (int i = 0; i < N; i += 10000) {
        if (ecs_is_alive(world, ids[i])) alive++;
    }
    if (alive > 0) {
        printf("  FAIL: %d still alive\n", alive);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 2: Multi-world cache safety */
static int test_multi_world_cache(void) {
    printf("TEST 2: multi_world_cache_safety\n");
    ecs_world_t *w1 = ecs_init();
    ecs_world_t *w2 = ecs_init();
    ECS_COMPONENT_DEFINE(w1, SX);
    ECS_COMPONENT_DEFINE(w2, SX);

    /* Create entity in w1 */
    ecs_entity_t e1 = ecs_new(w1);
    ecs_set(w1, e1, SX, {100});

    /* Create entity in w2 */
    ecs_entity_t e2 = ecs_new(w2);
    ecs_set(w2, e2, SX, {200});

    /* Verify no cross-contamination */
    const SX *p1 = ecs_get(w1, e1, SX);
    const SX *p2 = ecs_get(w2, e2, SX);
    if (p1->x != 100 || p2->x != 200) {
        printf("  FAIL: cross-contamination detected\n");
        return 1;
    }

    /* Bulk operations in both worlds */
    const int N = 1000;
    const ecs_entity_t *ids1 = ecs_bulk_new_w_id(w1, ecs_id(SX), N);
    const ecs_entity_t *ids2 = ecs_bulk_new_w_id(w2, ecs_id(SX), N);

    SX v1 = {111};
    SX v2 = {222};
    ecs_bulk_set_id(w1, ids1, N, ecs_id(SX), sizeof(SX), &v1);
    ecs_bulk_set_id(w2, ids2, N, ecs_id(SX), sizeof(SX), &v2);

    for (int i = 0; i < N; i += 100) {
        const SX *pa = ecs_get(w1, ids1[i], SX);
        const SX *pb = ecs_get(w2, ids2[i], SX);
        if (pa->x != 111 || pb->x != 222) {
            printf("  FAIL: bulk cache pollution\n");
            return 1;
        }
    }

    ecs_fini(w1);
    ecs_fini(w2);
    printf("  PASS\n");
    return 0;
}

/* TEST 3: Tier-I1 vs single ecs_add equivalence */
static int test_i1_vs_single_add(void) {
    printf("TEST 3: i1_vs_single_add_equivalence\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SX);
    ECS_COMPONENT_DEFINE(world, SY);
    ECS_COMPONENT_DEFINE(world, SZ);

    /* Method 1: individual ecs_add */
    ecs_entity_t e_single[3];
    for (int i = 0; i < 3; i++) {
        e_single[i] = ecs_new(world);
        ecs_add(world, e_single[i], SX);
        ecs_add(world, e_single[i], SY);
        ecs_add(world, e_single[i], SZ);
    }

    /* Method 2: ecs_add_ids batched */
    ecs_entity_t e_batch[3];
    for (int i = 0; i < 3; i++) e_batch[i] = ecs_new(world);
    ecs_id_t ids[3] = {ecs_id(SX), ecs_id(SY), ecs_id(SZ)};
    for (int i = 0; i < 3; i++) {
        ecs_add_ids(world, e_batch[i], ids, 3);
    }

    /* Verify equivalence */
    for (int i = 0; i < 3; i++) {
        assert(ecs_has(world, e_single[i], SX));
        assert(ecs_has(world, e_single[i], SY));
        assert(ecs_has(world, e_single[i], SZ));
        assert(ecs_has(world, e_batch[i], SX));
        assert(ecs_has(world, e_batch[i], SY));
        assert(ecs_has(world, e_batch[i], SZ));
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 4: Mixed tier-CC1 archetype safety with component swap */
static int test_archetype_swap(void) {
    printf("TEST 4: archetype_swap_with_cc1\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SX);
    ECS_COMPONENT_DEFINE(world, SY);

    /* Create entities with SX only */
    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SX), N);

    /* Bulk set SY for first 50 — adds SY, creates new archetype */
    ecs_id_t sy_ids[1] = {ecs_id(SY)};
    for (int i = 0; i < 50; i++) {
        ecs_add_ids(world, ids[i], sy_ids, 1);
    }

    /* Now bulk_set_id SX on all 100 — entities in 2 archetypes */
    SX v = {77};
    ecs_bulk_set_id(world, ids, N, ecs_id(SX), sizeof(SX), &v);

    /* Verify all have SX = 77 */
    int errors = 0;
    for (int i = 0; i < N; i++) {
        const SX *p = ecs_get(world, ids[i], SX);
        if (!p || p->x != 77) {
            printf("  FAIL: ids[%d]=%lld p->x=%d (expected 77)\n",
                i, (long long)ids[i], p ? p->x : -1);
            errors++;
        }
    }
    if (errors > 0) {
        printf("  %d errors\n", errors);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 5: Bulk operations with entity index recycle stress */
static int test_recycle_pressure(void) {
    printf("TEST 5: recycle_pressure\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SX);

    /* Repeatedly create+delete to stress ID recycling */
    for (int round = 0; round < 100; round++) {
        const int N = 50;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SX), N);
        SX v = {round};
        ecs_bulk_set_id(world, ids, N, ecs_id(SX), sizeof(SX), &v);
        for (int i = 0; i < N; i++) {
            const SX *p = ecs_get(world, ids[i], SX);
            if (!p || p->x != round) {
                printf("  FAIL: round %d, ids[%d]=%lld p->x=%d (expected %d)\n",
                    round, i, (long long)ids[i], p ? p->x : -1, round);
                return 1;
            }
        }
        ecs_bulk_delete(world, ids, N);
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 6: Big component bulk set */
static int test_big_component_bulk(void) {
    printf("TEST 6: big_component_bulk\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SBig);

    const int N = 10000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SBig), N);

    SBig shared = {1.5f, 2.5f, 3.5f, 4.5f};
    ecs_bulk_set_id(world, ids, N, ecs_id(SBig), sizeof(SBig), &shared);

    int errors = 0;
    for (int i = 0; i < N; i += 100) {
        const SBig *p = ecs_get(world, ids[i], SBig);
        if (!p || p->a != 1.5f || p->b != 2.5f || p->c != 3.5f || p->d != 4.5f) {
            errors++;
        }
    }
    if (errors > 0) {
        printf("  FAIL: %d errors\n", errors);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 7: Combined bulk operations sequence */
static int test_combined_sequence(void) {
    printf("TEST 7: combined_bulk_sequence\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, SX);
    ECS_COMPONENT_DEFINE(world, SY);
    ECS_COMPONENT_DEFINE(world, SZ);

    /* Create 100 entities with SX */
    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(SX), N);

    /* Set SX values */
    SX vx = {10};
    ecs_bulk_set_id(world, ids, N, ecs_id(SX), sizeof(SX), &vx);

    /* Add SY to all */
    ecs_id_t sy_id = ecs_id(SY);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &sy_id, 1);
    }

    /* Set SY values (now all in SX+SY archetype) */
    SY vy = {20};
    ecs_bulk_set_id(world, ids, N, ecs_id(SY), sizeof(SY), &vy);

    /* Add SZ to all (now all in SX+SY+SZ archetype) */
    ecs_id_t sz_id = ecs_id(SZ);
    for (int i = 0; i < N; i++) {
        ecs_add_ids(world, ids[i], &sz_id, 1);
    }

    /* Set SZ values */
    SZ vz = {30};
    ecs_bulk_set_id(world, ids, N, ecs_id(SZ), sizeof(SZ), &vz);

    /* Verify all values */
    for (int i = 0; i < N; i++) {
        const SX *px = ecs_get(world, ids[i], SX);
        const SY *py = ecs_get(world, ids[i], SY);
        const SZ *pz = ecs_get(world, ids[i], SZ);
        if (!px || px->x != 10 || !py || py->y != 20 || !pz || pz->z != 30) {
            printf("  FAIL: ids[%d] wrong values\n", i);
            return 1;
        }
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_100k_bulk();
    failures += test_multi_world_cache();
    failures += test_i1_vs_single_add();
    failures += test_archetype_swap();
    failures += test_recycle_pressure();
    failures += test_big_component_bulk();
    failures += test_combined_sequence();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 7 DEEP VERIFICATION TESTS PASSED\n");
    return 0;
}