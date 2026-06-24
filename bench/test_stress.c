/* Comprehensive stress test for Tier patches — includes edge cases. */
#include "flecs_patched.h"
#include <stdio.h>
#include <assert.h>

typedef struct { int x, y, z; } Pos;
typedef struct { int x, y, z; } Vel;
typedef struct { float mass; } Mass;
typedef struct { int hp; } Health;
typedef struct { int dmg; } Attack;
typedef struct { float data[64]; } BigStruct;

ECS_COMPONENT_DECLARE(Pos);
ECS_COMPONENT_DECLARE(Vel);
ECS_COMPONENT_DECLARE(Mass);
ECS_COMPONENT_DECLARE(Health);
ECS_COMPONENT_DECLARE(Attack);
ECS_COMPONENT_DECLARE(BigStruct);

/* Test 1: Large bulk operations */
static int test_large_bulk(void) {
    printf("TEST 1: large_bulk (100k entities)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);
    ECS_COMPONENT_DEFINE(world, Vel);

    Pos p = {1, 2, 3};
    Vel v = {4, 5, 6};

    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(Pos), 100000, &p, sizeof(Pos));
    assert(ids != NULL);

    ecs_id_t comp_ids[1] = { ecs_id(Vel) };
    ecs_add_ids_w_entities(world, ids, 100000, comp_ids, 1);
    ecs_bulk_set_id(world, ids, 100000, ecs_id(Vel), sizeof(Vel), &v);

    for (int i = 0; i < 100000; i += 10000) {
        const Pos *pp = ecs_get(world, ids[i], Pos);
        const Vel *vv = ecs_get(world, ids[i], Vel);
        if (!pp || pp->x != 1 || pp->y != 2 || pp->z != 3) { printf("  FAIL Pos at %d\n", i); return 1; }
        if (!vv || vv->x != 4 || vv->y != 5 || vv->z != 6) { printf("  FAIL Vel at %d\n", i); return 1; }
    }

    ecs_bulk_delete(world, ids, 100000);
    int alive = 0;
    for (int i = 0; i < 100000; i += 10000) {
        if (ecs_is_alive(world, ids[i])) alive++;
    }
    if (alive != 0) { printf("  FAIL: %d alive after delete\n", alive); return 1; }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 2: Mixed lifecycle (100 rounds) */
static int test_mixed_lifecycle(void) {
    printf("TEST 2: mixed_lifecycle (100 rounds)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);
    ECS_COMPONENT_DEFINE(world, Vel);
    ECS_COMPONENT_DEFINE(world, Mass);
    ECS_COMPONENT_DEFINE(world, Health);
    ECS_COMPONENT_DEFINE(world, Attack);

    for (int round = 0; round < 100; round++) {
        const int N = 100;
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(Pos), N);
        assert(ids != NULL);

        ecs_id_t add_ids[4] = { ecs_id(Vel), ecs_id(Mass), ecs_id(Health), ecs_id(Attack) };
        ecs_add_ids_w_entities(world, ids, N, add_ids, 4);

        for (int i = 0; i < N; i++) {
            assert(ecs_has(world, ids[i], Pos));
            assert(ecs_has(world, ids[i], Vel));
            assert(ecs_has(world, ids[i], Mass));
            assert(ecs_has(world, ids[i], Health));
            assert(ecs_has(world, ids[i], Attack));
        }

        ecs_id_t rm_ids[2] = { ecs_id(Mass), ecs_id(Health) };
        for (int i = 0; i < N; i++) {
            ecs_remove_ids(world, ids[i], rm_ids, 2);
        }

        for (int i = 0; i < N; i++) {
            assert(!ecs_has(world, ids[i], Mass));
            assert(!ecs_has(world, ids[i], Health));
            assert(ecs_has(world, ids[i], Pos));
            assert(ecs_has(world, ids[i], Vel));
        }

        ecs_bulk_delete(world, ids, N);
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 3: Query after bulk */
static int test_query_after_bulk(void) {
    printf("TEST 3: query_after_bulk\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);
    ECS_COMPONENT_DEFINE(world, Vel);

    Pos p = {1, 2, 3};
    Vel v = {4, 5, 6};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(Pos), 500, &p, sizeof(Pos));
    ecs_id_t comp_ids[1] = { ecs_id(Vel) };
    ecs_add_ids_w_entities(world, ids, 500, comp_ids, 1);
    ecs_bulk_set_id(world, ids, 500, ecs_id(Vel), sizeof(Vel), &v);

    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(Pos) }, { .id = ecs_id(Vel) }}
    });

    int total_count = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        total_count += it.count;
    }
    assert(total_count == 500);

    ecs_iter_t it2 = ecs_query_iter(world, q);
    while (ecs_query_next(&it2)) {
        const Pos *p = ecs_field(&it2, Pos, 0);
        for (int i = 0; i < it2.count; i++) {
            assert(p[i].x == 1 && p[i].y == 2 && p[i].z == 3);
        }
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 4: Observer dispatch after bulk */
static int obs_count = 0;
static void on_add_cb(ecs_iter_t *it) { obs_count += it->count; }
static void on_remove_cb(ecs_iter_t *it) { obs_count -= it->count; }

static int test_observer_after_bulk(void) {
    printf("TEST 4: observer_after_bulk\n");
    obs_count = 0;
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(Pos) }},
        .events = { EcsOnAdd },
        .callback = on_add_cb
    });
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(Pos) }},
        .events = { EcsOnRemove },
        .callback = on_remove_cb
    });

    Pos p = {0};
    const ecs_entity_t *ids = ecs_bulk_new_w_value(world, ecs_id(Pos), 100, &p, sizeof(Pos));
    assert(obs_count == 100);

    ecs_bulk_delete(world, ids, 100);
    assert(obs_count == 0);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 5: Multi-world isolation */
static int test_multi_world(void) {
    printf("TEST 5: multi_world\n");
    ecs_world_t *w1 = ecs_init();
    ecs_world_t *w2 = ecs_init();
    ECS_COMPONENT_DEFINE(w1, Pos);
    ECS_COMPONENT_DEFINE(w2, Pos);

    Pos p = {10, 20, 30};
    const ecs_entity_t *ids1 = ecs_bulk_new_w_value(w1, ecs_id(Pos), 50, &p, sizeof(Pos));
    const ecs_entity_t *ids2 = ecs_bulk_new_w_value(w2, ecs_id(Pos), 50, &p, sizeof(Pos));

    for (int i = 0; i < 50; i++) {
        assert(ecs_get(w1, ids1[i], Pos)->x == 10);
        assert(ecs_get(w2, ids2[i], Pos)->x == 10);
    }

    ecs_bulk_delete(w1, ids1, 50);
    for (int i = 0; i < 50; i++) {
        assert(!ecs_is_alive(w1, ids1[i]));
        assert(ecs_is_alive(w2, ids2[i]));
    }

    ecs_fini(w1);
    ecs_fini(w2);
    printf("  PASS\n");
    return 0;
}

/* Test 6: ID recycle stress */
static int test_id_recycle_stress(void) {
    printf("TEST 6: id_recycle_stress (50 rounds)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    for (int round = 0; round < 50; round++) {
        ecs_entity_t es[200];
        for (int i = 0; i < 200; i++) {
            es[i] = ecs_new(world);
            ecs_set(world, es[i], Pos, {i});
        }
        ecs_bulk_delete(world, es, 100);

        ecs_entity_t fresh[100];
        for (int i = 0; i < 100; i++) {
            fresh[i] = ecs_new(world);
            ecs_set(world, fresh[i], Pos, {i + 1000});
        }
        for (int i = 0; i < 100; i++) {
            const Pos *p = ecs_get(world, fresh[i], Pos);
            assert(p != NULL);
            assert(p->x == i + 1000);
        }
        ecs_bulk_delete(world, es + 100, 100);
        ecs_bulk_delete(world, fresh, 100);
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 7: Large component (256 bytes) */
static int test_large_component(void) {
    printf("TEST 7: large_component (256 bytes)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, BigStruct);

    const int N = 1000;
    const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(BigStruct), N);

    BigStruct shared = {{0}};
    for (int i = 0; i < 64; i++) shared.data[i] = (float)i;

    ecs_bulk_set_id(world, ids, N, ecs_id(BigStruct), sizeof(BigStruct), &shared);

    for (int i = 0; i < N; i += 100) {
        const BigStruct *p = ecs_get(world, ids[i], BigStruct);
        for (int j = 0; j < 64; j++) {
            assert(p->data[j] == (float)j);
        }
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 8: Recursive bulk operations */
static int test_recursive_bulk(void) {
    printf("TEST 8: recursive_bulk\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    for (int round = 0; round < 10; round++) {
        ecs_entity_t es[100];
        for (int i = 0; i < 100; i++) es[i] = ecs_new(world);

        ecs_bulk_delete(world, es, 50);
        for (int i = 0; i < 50; i++) assert(!ecs_is_alive(world, es[i]));
        for (int i = 50; i < 100; i++) assert(ecs_is_alive(world, es[i]));

        ecs_bulk_delete(world, es + 50, 50);
        for (int i = 50; i < 100; i++) assert(!ecs_is_alive(world, es[i]));
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 9: Empty world */
static int test_empty_world(void) {
    printf("TEST 9: empty_world\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    ecs_entity_t e = ecs_new(world);
    Pos p = {42};
    ecs_set(world, e, Pos, {42});
    assert(ecs_get(world, e, Pos)->x == 42);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

/* Test 10: Tier-FF1 cache invalidation under pressure */
static int test_ff1_invalidation(void) {
    printf("TEST 10: ff1_cache_invalidation\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, Pos);

    /* Create, delete, recreate pattern that triggers ID reuse */
    for (int i = 0; i < 1000; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_set(world, e, Pos, {i});
        assert(ecs_get(world, e, Pos)->x == i);
        ecs_delete(world, e);
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_large_bulk();
    failures += test_mixed_lifecycle();
    failures += test_query_after_bulk();
    failures += test_observer_after_bulk();
    failures += test_multi_world();
    failures += test_id_recycle_stress();
    failures += test_large_component();
    failures += test_recursive_bulk();
    failures += test_empty_world();
    failures += test_ff1_invalidation();

    if (failures) {
        printf("\n%d TESTS FAILED\n", failures);
        return 1;
    }
    printf("\nALL 10 STRESS+EDGE TESTS PASSED\n");
    return 0;
}