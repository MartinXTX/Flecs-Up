/* Tier-A1 sparse-set hybrid TEST
 *
 * Verifies that the Tier-A1.3 LAZY auto-mark + EcsDontFragment dispatch
 * path actually works as advertised:
 *   1. Data-only components (no hooks, no observers) get auto-marked
 *      EcsDontFragment on register.
 *   2. Setting such a component does NOT change the entity's archetype
 *      (sparse path instead of archetype-column).
 *   3. ecs_get_id reads directly from cr->sparse (fast path).
 *   4. ecs_has_id works (sparse has() check).
 *   5. ecs_remove_id clears the sparse entry; subsequent add re-creates.
 *   6. 100k entity stress completes without crash.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct { int x; } Pos;
typedef struct { float y; } Vel;
typedef struct { double z; } Acc;
typedef struct { char w; } W;

static int n_tests = 0;
static int n_pass = 0;
#define CHECK(cond, msg) do { \
    n_tests++; \
    if (cond) { n_pass++; } \
    else { fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); } \
} while (0)

static void test_auto_mark(void) {
    printf("[1] Auto-mark: data-only component -> EcsDontFragment ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    ECS_COMPONENT(world, Pos);
    CHECK(ecs_has_id(world, ecs_id(Pos), EcsDontFragment),
        "Pos should be auto-marked EcsDontFragment");

    ecs_fini(world);
    printf("OK\n");
}

static void test_no_archetype_churn(void) {
    printf("[2] No archetype churn: ecs_set_id does not move entity ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);
    ECS_COMPONENT(world, Pos);
    ECS_TAG(world, TagA);

    ecs_entity_t e = ecs_new(world);
    ecs_add_id(world, e, TagA);
    ecs_table_t *t0 = ecs_get_table(world, e);

    /* Set Pos many times -- entity must stay in t0 */
    for (int i = 0; i < 100; i++) {
        ecs_set_ptr(world, e, Pos, &(Pos){.x = i});
    }

    ecs_table_t *t1 = ecs_get_table(world, e);
    CHECK(t0 == t1,
        "Entity table must NOT change when setting EcsDontFragment component");

    ecs_fini(world);
    printf("OK\n");
}

static void test_get_id_sparse_path(void) {
    printf("[3] ecs_get_id reads sparse path ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);
    ECS_COMPONENT(world, Pos);

    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, Pos, {42});
    const Pos *p = ecs_get(world, e, Pos);
    CHECK(p != NULL && p->x == 42, "ecs_get must return Pos with x=42");

    /* ecs_has_id */
    CHECK(ecs_has_id(world, e, ecs_id(Pos)), "ecs_has_id(Pos) must be true");

    ecs_fini(world);
    printf("OK\n");
}

static void test_remove_re_add(void) {
    printf("[4] remove + re-add: data persists correctly ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);
    ECS_COMPONENT(world, Pos);

    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, Pos, {7});
    CHECK(ecs_get(world, e, Pos)->x == 7, "first set x=7");

    ecs_remove_id(world, e, ecs_id(Pos));
    CHECK(!ecs_has_id(world, e, ecs_id(Pos)), "after remove, ecs_has_id = false");

    ecs_set(world, e, Pos, {13});
    CHECK(ecs_get(world, e, Pos)->x == 13, "re-add: x=13");

    ecs_fini(world);
    printf("OK\n");
}

static void test_many_components(void) {
    printf("[5] 1 entity + 4 EcsDontFragment components ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    /* Register 4 data-only components */
    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);
    ECS_COMPONENT(world, Acc);
    ECS_COMPONENT(world, W);

    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, Pos, {1});
    ecs_set(world, e, Vel, {2.0f});
    ecs_set(world, e, Acc, {3.0});
    ecs_set(world, e, W, {'a'});

    CHECK(ecs_get(world, e, Pos)->x == 1, "Pos=1");
    CHECK(ecs_get(world, e, Vel)->y == 2.0f, "Vel=2.0");
    CHECK(ecs_get(world, e, Acc)->z == 3.0, "Acc=3.0");
    CHECK(ecs_get(world, e, W)->w == 'a', "W='a'");

    ecs_fini(world);
    printf("OK\n");
}

static void test_stress_100k(void) {
    printf("[6] Stress: 100k entities + bulk set/remove ... ");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    ECS_COMPONENT(world, Pos);

    const int N = 100000;
    ecs_entity_t *ids = malloc(sizeof(ecs_entity_t) * N);
    for (int i = 0; i < N; i++) {
        ids[i] = ecs_new(world);
        ecs_set(world, ids[i], Pos, {i});
    }

    /* Verify all reads */
    int ok = 1;
    for (int i = 0; i < N; i++) {
        if (ecs_get(world, ids[i], Pos)->x != i) {
            ok = 0;
            break;
        }
    }
    CHECK(ok, "All 100k Pos values must be readable");

    /* Bulk remove */
    for (int i = 0; i < N; i++) {
        ecs_remove_id(world, ids[i], ecs_id(Pos));
    }
    ok = 1;
    for (int i = 0; i < N; i++) {
        if (ecs_has_id(world, ids[i], ecs_id(Pos))) {
            ok = 0;
            break;
        }
    }
    CHECK(ok, "All 100k Pos must be removed");

    free(ids);
    ecs_fini(world);
    printf("OK\n");
}

int main(void) {
    printf("=== Tier-A1 sparse-set hybrid TEST ===\n\n");

    test_auto_mark();
    test_no_archetype_churn();
    test_get_id_sparse_path();
    test_remove_re_add();
    test_many_components();
    test_stress_100k();

    printf("\n=== Result: %d/%d PASS ===\n", n_pass, n_tests);
    return (n_pass == n_tests) ? 0 : 1;
}
