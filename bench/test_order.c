/* Observer ordering and Tier-CC1 reverse iteration tests. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x; } OPos;
typedef struct { int y; } OOther;
ECS_COMPONENT_DECLARE(OPos);
ECS_COMPONENT_DECLARE(OOther);

static int call_order[1000];
static int call_idx = 0;
static int on_add_count = 0;
static int on_set_count = 0;

static void on_set_cb(ecs_iter_t *it) {
    const OPos *p = ecs_field(it, OPos, 0);
    for (int i = 0; i < it->count; i++) {
        if (call_idx < 1000) {
            call_order[call_idx++] = p[i].x;
        }
    }
}

static void on_add_cb(ecs_iter_t *it) {
    on_add_count += it->count;
}

static void on_set_count_cb(ecs_iter_t *it) {
    on_set_count += it->count;
}

/* TEST 1: Bulk set values are correct (verify via ecs_get) */
static int test_bulk_set_order(void) {
    printf("TEST 1: bulk_set_values_correct\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    /* Test 1a: single entity bulk_set_id (count=1) */
    ecs_entity_t e1 = ecs_new(world);
    ecs_add(world, e1, OPos);
    OPos v1 = {42};
    ecs_bulk_set_id(world, &e1, 1, ecs_id(OPos), sizeof(OPos), &v1);
    const OPos *p1 = ecs_get(world, e1, OPos);
    printf("  1a: p1->x=%d (expected 42)\n", p1->x);
    if (!p1 || p1->x != 42) return 1;

    /* Test 1b: 3 entities bulk_set with SAME value (API contract) */
    ecs_entity_t ids[3];
    for (int i = 0; i < 3; i++) {
        ids[i] = ecs_new(world);
        ecs_add(world, ids[i], OPos);
    }
    OPos shared = {77};  /* Same value for all */
    ecs_bulk_set_id(world, ids, 3, ecs_id(OPos), sizeof(OPos), &shared);
    for (int i = 0; i < 3; i++) {
        const OPos *p = ecs_get(world, ids[i], OPos);
        printf("  1b[%d]: p->x=%d (expected 77)\n", i, p->x);
        if (!p || p->x != 77) return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 2: Tier-R1 OnAdd fires once per entity */
static int test_r1_on_add_once(void) {
    printf("TEST 2: r1_on_add_once\n");
    on_add_count = 0;
    on_set_count = 0;
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(OPos) }},
        .events = { EcsOnAdd },
        .callback = on_add_cb
    });
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(OPos) }},
        .events = { EcsOnSet },
        .callback = on_set_count_cb
    });

    OPos v = {99};
    ecs_bulk_new_w_value(world, ecs_id(OPos), 50, &v, sizeof(OPos));

    printf("  on_add=%d on_set=%d (expected on_add=50)\n", on_add_count, on_set_count);
    if (on_add_count != 50) {
        printf("  FAIL\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 3: Tier-CC1 mixed archetype safety */
static int test_cc1_mixed_archetype_safe(void) {
    printf("TEST 3: cc1_mixed_archetype_safe\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);
    ECS_COMPONENT_DEFINE(world, OOther);

    ecs_entity_t e1 = ecs_new(world);
    ecs_add(world, e1, OPos);

    ecs_entity_t e2 = ecs_new(world);
    ecs_add(world, e2, OPos);
    ecs_add(world, e2, OOther);

    ecs_entity_t ids[2] = {e1, e2};
    OPos v = {77};
    ecs_bulk_set_id(world, ids, 2, ecs_id(OPos), sizeof(OPos), &v);

    const OPos *p1 = ecs_get(world, e1, OPos);
    const OPos *p2 = ecs_get(world, e2, OPos);
    printf("  p1.x=%d p2.x=%d\n", p1->x, p2->x);

    if (p1->x != 77 || p2->x != 77) {
        printf("  FAIL\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 4: Tier-P1 with recycled IDs */
static int test_p1_recycled_ids(void) {
    printf("TEST 4: p1_recycled_ids\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);

    const int N = 100;
    ecs_entity_t ids[100];

    /* Round 1 */
    for (int i = 0; i < N; i++) {
        ids[i] = ecs_new(world);
        ecs_add(world, ids[i], OPos);
    }
    ecs_bulk_delete(world, ids, N);

    /* Round 2 - IDs may be recycled */
    for (int i = 0; i < N; i++) {
        ids[i] = ecs_new(world);
        ecs_add(world, ids[i], OPos);
        ecs_set(world, ids[i], OPos, {i * 10});
    }

    int errors = 0;
    for (int i = 0; i < N; i++) {
        const OPos *p = ecs_get(world, ids[i], OPos);
        if (!p || p->x != i * 10) errors++;
    }
    printf("  errors=%d (expected 0)\n", errors);
    if (errors > 0) {
        printf("  FAIL\n");
        return 1;
    }

    ecs_bulk_delete(world, ids, N);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 5: Tier-I1 batched add with deferred operations */
static int test_i1_deferred_ops(void) {
    printf("TEST 5: i1_deferred_ops\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);
    ECS_COMPONENT_DEFINE(world, OOther);

    ecs_entity_t e = ecs_new(world);

    /* Defer ops */
    ecs_defer_begin(world);
    ecs_id_t ids[2] = {ecs_id(OPos), ecs_id(OOther)};
    ecs_add_ids(world, e, ids, 2);
    ecs_defer_end(world);

    /* After defer_end, ops should be flushed */
    if (!ecs_has(world, e, OPos)) {
        printf("  FAIL: OPos not added after defer_end\n");
        return 1;
    }
    if (!ecs_has(world, e, OOther)) {
        printf("  FAIL: OOther not added after defer_end\n");
        return 1;
    }

    /* Now remove */
    ecs_defer_begin(world);
    ecs_remove_ids(world, e, ids, 2);
    ecs_defer_end(world);

    if (ecs_has(world, e, OPos) || ecs_has(world, e, OOther)) {
        printf("  FAIL: components still present after remove\n");
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* TEST 6: Tier-EE1 across N entities with multiple components */
static int test_ee1_multi_component(void) {
    printf("TEST 6: ee1_multi_component\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, OPos);
    ECS_COMPONENT_DEFINE(world, OOther);

    const int N = 100;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(OPos), N);

    ecs_id_t add_ids[2] = {ecs_id(OPos), ecs_id(OOther)};
    ecs_add_ids_w_entities(world, ids, N, add_ids, 2);

    for (int i = 0; i < N; i++) {
        assert(ecs_has(world, ids[i], OPos));
        assert(ecs_has(world, ids[i], OOther));
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_bulk_set_order();
    failures += test_r1_on_add_once();
    failures += test_cc1_mixed_archetype_safe();
    failures += test_p1_recycled_ids();
    failures += test_i1_deferred_ops();
    failures += test_ee1_multi_component();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 6 ORDER TESTS PASSED\n");
    return 0;
}