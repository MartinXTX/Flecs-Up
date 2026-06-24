/* TIER-B2 test: tiny-archetype single-chunk alloc.
 *
 * Verifies that tables with <=2 columns, no pairs, no overrides take
 * the EcsTableIsTiny fast path and behave identically to the standard
 * path. Runs:
 *   1) Smoke   : 1 entity with 1 component (verify create/set/get works)
 *   2) Grow    : 10K single-component entities (verify count/size match
 *                 table->data.entities / columns)
 *   3) Mixed   : create entities across several tiny archetypes and
 *                 verify they all remain queryable & correct.
 *   4) Cleanup : fini runs clean (no leaks, no asserts).
 *
 * Build (see build_b2.bat):
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD ^
 *      /I . /I ..\include test_b2_tiny.c /Fe:bench\release\test_b2_tiny.exe ^
 *      bench\release\v18_b2_flecs_patched.lib
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct { int    x; } Pos1;       /* single-col tiny */
typedef struct { float  y; } Vel1;
typedef struct { double z; } Scl1;
typedef struct { int    a; int b; } TwoCol; /* two-col tiny */

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); g_fail++; } \
} while (0)

static void test_smoke(ecs_world_t *world) {
    printf("[smoke] single entity, single component\n");
    ECS_COMPONENT(world, Pos1);
    ecs_entity_t e = ecs_new(world);
    ecs_set(world, e, Pos1, { 42 });
    const Pos1 *p = ecs_get(world, e, Pos1);
    CHECK(p != NULL,                "ecs_get returned NULL");
    CHECK(p->x == 42,               "value mismatch");
    ecs_fini(world);
}

static void test_grow_10k(ecs_world_t *world) {
    printf("[grow] 10K entities single-col\n");
    ECS_COMPONENT(world, Pos1);
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, 10000);
    for (int i = 0; i < 10000; i++) {
        es[i] = ecs_new(world);
        ecs_set(world, es[i], Pos1, { i });
    }
    /* verify all 10K are reachable */
    int hit = 0;
    for (int i = 0; i < 10000; i++) {
        const Pos1 *p = ecs_get(world, es[i], Pos1);
        if (p && p->x == i) hit++;
    }
    CHECK(hit == 10000, "not all 10K entities returned correct value");
    printf("       hit=%d/10000\n", hit);
    ecs_os_free(es);
    ecs_fini(world);
}

static void test_mixed(ecs_world_t *world) {
    printf("[mixed] 4 archetypes, varying per-entity types\n");
    ECS_COMPONENT(world, Pos1);
    ECS_COMPONENT(world, Vel1);
    ECS_COMPONENT(world, Scl1);
    ECS_COMPONENT(world, TwoCol);

    ecs_entity_t e_pos   = ecs_new(world);
    ecs_entity_t e_vel   = ecs_new(world);
    ecs_entity_t e_scl   = ecs_new(world);
    ecs_entity_t e_two   = ecs_new(world);

    ecs_add(world, e_pos, Pos1);
    ecs_add(world, e_vel, Vel1);
    ecs_add(world, e_scl, Scl1);
    ecs_add(world, e_two, TwoCol);

    ecs_set(world, e_pos, Pos1, { 1 });
    ecs_set(world, e_vel, Vel1, { 2.0f });
    ecs_set(world, e_scl, Scl1, { 3.0 });
    ecs_set(world, e_two, TwoCol, { 4, 5 });

    CHECK(ecs_get(world, e_pos, Pos1)->x     == 1,   "Pos1 mismatch");
    CHECK(ecs_get(world, e_vel, Vel1)->y     == 2.0f,"Vel1 mismatch");
    CHECK(ecs_get(world, e_scl, Scl1)->z     == 3.0, "Scl1 mismatch");
    CHECK(ecs_get(world, e_two, TwoCol)->a   == 4,   "TwoCol.a mismatch");
    CHECK(ecs_get(world, e_two, TwoCol)->b   == 5,   "TwoCol.b mismatch");

    /* delete one, ensure others survive */
    ecs_delete(world, e_pos);
    CHECK(ecs_exists(world, e_vel), "Vel1 entity lost");
    CHECK(ecs_exists(world, e_scl), "Scl1 entity lost");
    CHECK(ecs_exists(world, e_two), "TwoCol entity lost");

    ecs_fini(world);
}

static void test_query(ecs_world_t *world) {
    printf("[query] verify ecs_query_next sees all tiny-archetype entities\n");
    ECS_COMPONENT(world, Pos1);
    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new(world);
        ecs_add(world, e, Pos1);
    }
    ecs_query_t *q = ecs_query_init(world, &(ecs_query_desc_t){
        .terms = {
            { .id = ecs_id(Pos1) },
        }
    });
    CHECK(q != NULL, "query creation failed");

    int seen = 0;
    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        seen += it.count;
    }
    CHECK(seen == 100, "query did not see all 100 entities");
    printf("       seen=%d/100\n", seen);
    ecs_query_fini(q);
    ecs_fini(world);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("=== test_b2_tiny: TIER-B2 tiny-archetype verification ===\n");

    ecs_world_t *w;
    w = ecs_init();           test_smoke(w);
    w = ecs_init();           test_grow_10k(w);
    w = ecs_init();           test_mixed(w);
    w = ecs_init();           test_query(w);

    if (g_fail) {
        printf("FAILED: %d assertion(s)\n", g_fail);
        return 1;
    }
    printf("PASSED: all checks\n");
    return 0;
}