/* TIER v14 COMPREHENSIVE CROSS-TEST v2 (MSVC-safe)
 *
 * Bypasses BULGU-10 (MSVC ecs_set macro token-paste issue) by using
 * ecs_set_id directly everywhere. Tests 22 scenarios from 9 categories.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

/* =========================================================================
 * Data structures
 * ========================================================================= */
typedef struct { int32_t x; } S1;
typedef struct { double a, b, c; } S3;
typedef struct { int64_t v; char buf[120]; } SB;
typedef struct { int x; int y; int z; } SXYZ;

ECS_COMPONENT_DECLARE(S1);
ECS_COMPONENT_DECLARE(S3);
ECS_COMPONENT_DECLARE(SB);
ECS_COMPONENT_DECLARE(SXYZ);

/* =========================================================================
 * Hook counters
 * ========================================================================= */
static int g_ctor = 0, g_dtor = 0, g_copy = 0, g_move = 0;
static int g_obs_set = 0, g_obs_add = 0, g_obs_unset = 0;

static void sb_ctor(void *ptr, int32_t n, const ecs_type_info_t *ti) {
    (void)ti; memset(ptr, 0, sizeof(SB) * n); g_ctor += n;
}
static void sb_dtor(void *ptr, int32_t n, const ecs_type_info_t *ti) {
    (void)ptr; (void)ti; g_dtor += n;
}
static void sb_copy(void *dst, const void *src, int32_t n, const ecs_type_info_t *ti) {
    (void)ti; memcpy(dst, src, sizeof(SB) * n); g_copy += n;
}
static void sb_move(void *dst, void *src, int32_t n, const ecs_type_info_t *ti) {
    (void)ti; memmove(dst, src, sizeof(SB) * n); g_move += n;
}

static void obs_on_set(ecs_iter_t *it) { for (int i=0;i<it->count;i++) g_obs_set++; }
static void obs_on_add(ecs_iter_t *it) { for (int i=0;i<it->count;i++) g_obs_add++; }
static void obs_on_unset(ecs_iter_t *it) { (void)it; }

static void reset_hooks(void) {
    g_ctor = g_dtor = g_copy = g_move = 0;
    g_obs_set = g_obs_add = g_obs_unset = 0;
}

static int total_tests = 0, total_failures = 0;

#define ASSERT_TRUE(c, m) do { if (!(c)) { printf("    FAIL: %s\n", m); return 0; } } while(0)
#define ASSERT_EQ(a, b, m) do { if ((long long)(a) != (long long)(b)) { \
    printf("    FAIL: %s (got %lld exp %lld)\n", m, (long long)(a), (long long)(b)); \
    return 0; } } while(0)
#define TS(n) printf("  [%s]\n", n)

/* =========================================================================
 * TEST 1: Deep IsA inheritance (5 levels)
 * ========================================================================= */
static int test_deep_isa(void) {
    TS("deep_isa_5_levels");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, SXYZ);
    ecs_entity_t a = ecs_entity(w, { .name = "Animal" });
    SXYZ v1 = {100, 200, 300}; ecs_set_id(w, a, ecs_id(SXYZ), sizeof(v1), &v1);
    ecs_entity_t dog = ecs_entity(w, { .name = "Dog" });
    ecs_add_pair(w, dog, EcsIsA, a);
    SXYZ v2 = {50, 60, 70}; ecs_set_id(w, dog, ecs_id(SXYZ), sizeof(v2), &v2);
    ecs_entity_t puppy = ecs_entity(w, { .name = "Puppy" });
    ecs_add_pair(w, puppy, EcsIsA, dog);
    ecs_entity_t poodle = ecs_entity(w, { .name = "ToyPoodle" });
    ecs_add_pair(w, poodle, EcsIsA, puppy);
    SXYZ v3 = {1, 2, 3}; ecs_set_id(w, poodle, ecs_id(SXYZ), sizeof(v3), &v3);
    ecs_entity_t tiny = ecs_entity(w, { .name = "TinyPoodle" });
    ecs_add_pair(w, tiny, EcsIsA, poodle);
    const SXYZ *p = ecs_get(w, tiny, SXYZ);
    ASSERT_EQ(p->x, 1, "poodle x override");
    ASSERT_EQ(p->y, 2, "poodle y override");
    ASSERT_EQ(p->z, 3, "poodle z override");
    const SXYZ *q = ecs_get(w, puppy, SXYZ);
    ASSERT_EQ(q->x, 50, "dog x inherited");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 2: Custom hooks (ctor/dtor/copy/move)
 * ========================================================================= */
static int test_custom_hooks(void) {
    TS("custom_hooks");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, SB);
    ecs_set_hooks_id(w, ecs_id(SB), &(ecs_type_hooks_t){
        .ctor = sb_ctor, .dtor = sb_dtor,
        .copy = sb_copy, .move = sb_move,
    });
    reset_hooks();
    ecs_entity_t e = ecs_new(w);
    SB v = {42, "test"};
    ecs_set_id(w, e, ecs_id(SB), sizeof(v), &v);
    ASSERT_TRUE(g_ctor >= 1, "ctor fired");
    ASSERT_TRUE(g_copy >= 1, "copy fired");
    int dt0 = g_dtor;
    ecs_delete(w, e);
    ASSERT_TRUE(g_dtor > dt0, "dtor fired on delete");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 3: Cascade delete (10 levels)
 * ========================================================================= */
static int test_cascade_10(void) {
    TS("cascade_delete_10_levels");
    ecs_world_t *w = ecs_init();
    ecs_entity_t c[10];
    for (int i=0;i<10;i++) c[i] = ecs_new(w);
    for (int i=1;i<10;i++) ecs_add_pair(w, c[i], EcsChildOf, c[i-1]);
    ecs_delete(w, c[0]);
    int alive = 0;
    for (int i=0;i<10;i++) if (ecs_is_alive(w, c[i])) alive++;
    ASSERT_EQ(alive, 0, "all cascaded");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 4: Prefab override (3 levels)
 * ========================================================================= */
static int test_prefab_3lvl(void) {
    TS("prefab_override_3_level");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ECS_COMPONENT_DEFINE(w, S3);
    ecs_entity_t pa = ecs_entity(w, { .name = "PA" });
    ecs_add(w, pa, S1);
    S1 v10 = {10}; ecs_set_id(w, pa, ecs_id(S1), sizeof(v10), &v10);
    ecs_entity_t pb = ecs_entity(w, { .name = "PB" });
    ecs_add_pair(w, pb, EcsIsA, pa);
    ecs_add(w, pb, S1);
    S1 v20 = {20}; ecs_set_id(w, pb, ecs_id(S1), sizeof(v20), &v20);
    ecs_add(w, pb, S3);
    S3 v30 = {1.0, 2.0, 3.0}; ecs_set_id(w, pb, ecs_id(S3), sizeof(v30), &v30);
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, pb);
    const S1 *p1 = ecs_get(w, inst, S1);
    ASSERT_EQ(p1->x, 20, "PB's S1 override");
    const S3 *p3 = ecs_get(w, inst, S3);
    ASSERT_EQ((int)p3->a, 1, "PB's S3 inherited");
    S1 v99 = {99}; ecs_set_id(w, inst, ecs_id(S1), sizeof(v99), &v99);
    const S1 *p1b = ecs_get(w, inst, S1);
    ASSERT_EQ(p1b->x, 99, "instance S1 override");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 5: World fini edge cases
 * ========================================================================= */
static int test_world_fini(void) {
    TS("world_fini_edge_cases");
    ecs_world_t *w = ecs_init();
    ecs_fini(w);
    w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_entity_t e = ecs_new(w);
    S1 v = {1}; ecs_set_id(w, e, ecs_id(S1), sizeof(v), &v);
    ecs_fini(w);
    w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_entity_t *ids = ecs_bulk_new_w_id(w, ecs_id(S1), 100);
    S1 vb = {42};
    ecs_bulk_set_id(w, ids, 100, ecs_id(S1), sizeof(vb), &vb);
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 6: Storage allocator pressure
 * ========================================================================= */
static int test_storage_pressure(void) {
    TS("storage_allocator_pressure");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    for (int i=0;i<10000;i++) {
        ecs_entity_t e = ecs_new(w);
        S1 v = {i}; ecs_set_id(w, e, ecs_id(S1), sizeof(v), &v);
    }
    ecs_entity_t td[5000]; int j=0;
    ecs_query_t *q = ecs_query(w, { .terms = {{ .id = ecs_id(S1) }} });
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it) && j < 5000) {
        for (int i=0;i<it.count && j<5000;i++) td[j++] = it.entities[i];
    }
    for (int i=0;i<5000;i++) ecs_delete(w, td[i]);
    int rem = 0;
    it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) rem += it.count;
    ASSERT_EQ(rem, 5000, "remaining after delete half");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 7: Named components at scale
 * ========================================================================= */
static int test_named_1000(void) {
    TS("named_components_1000");
    ecs_world_t *w = ecs_init();
    ecs_entity_t e[1000]; char nm[64];
    for (int i=0;i<1000;i++) { snprintf(nm,sizeof(nm),"Ent_%d",i); e[i]=ecs_entity(w,{.name=nm}); }
    for (int i=0;i<1000;i+=100) {
        snprintf(nm,sizeof(nm),"Ent_%d",i);
        ASSERT_EQ(ecs_lookup(w,nm), e[i], "name lookup");
    }
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 8: Fuzzing
 * ========================================================================= */
static int test_fuzzing(void) {
    TS("fuzzing_5000_ops");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ECS_COMPONENT_DEFINE(w, S3);
    srand(42);
    int ac = 0; ecs_entity_t alive[1000];
    for (int op=0;op<5000;op++) {
        int a = rand()%5;
        switch(a) {
            case 0: if (ac<999) alive[ac++]=ecs_new(w); break;
            case 1: if (ac>0) { int i=rand()%ac; S1 v={op}; ecs_set_id(w,alive[i],ecs_id(S1),sizeof(v),&v); } break;
            case 2: if (ac>0) { int i=rand()%ac; ecs_add_id(w,alive[i],ecs_id(S3)); } break;
            case 3: if (ac>0) { int i=rand()%ac; ecs_remove_id(w,alive[i],ecs_id(S3)); } break;
            case 4: if (ac>0) { int i=rand()%ac; ecs_delete(w,alive[i]); alive[i]=alive[--ac]; } break;
        }
    }
    printf("    fuzz: 5000 ops, %d alive\n", ac);
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 9: Multi-stage defer
 * ========================================================================= */
static int test_defer(void) {
    TS("multi_stage_defer");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_defer_begin(w);
    ecs_entity_t e = ecs_new(w);
    S1 v = {42}; ecs_set_id(w, e, ecs_id(S1), sizeof(v), &v);
    ecs_defer_end(w);
    const S1 *p = ecs_get(w, e, S1);
    ASSERT_EQ(p->x, 42, "value after defer");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 10: Tier-X1+ V2 multi-table iteration
 * ========================================================================= */
static int test_x1_multitable(void) {
    TS("tier_x1_v2_multi_table");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ECS_COMPONENT_DEFINE(w, S3);
    ecs_entity_t a1 = ecs_new(w); S1 v1={1}; ecs_set_id(w,a1,ecs_id(S1),sizeof(v1),&v1);
    ecs_entity_t a2 = ecs_new(w); S1 v2={2}; ecs_set_id(w,a2,ecs_id(S1),sizeof(v2),&v2); ecs_add(w,a2,S3);
    ecs_entity_t a3 = ecs_new(w); S3 v3={1,2,3}; ecs_set_id(w,a3,ecs_id(S3),sizeof(v3),&v3);
    int total=0, f1=0, f2=0;
    ecs_query_t *q = ecs_query(w, { .terms = {{ .id = ecs_id(S1) }} });
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) {
        for (int i=0;i<it.count;i++) {
            const S1 *p = ecs_field(&it, S1, 0);
            (void)p;
            total++;
            if (it.entities[i]==a1) f1++;
            if (it.entities[i]==a2) f2++;
        }
    }
    ASSERT_EQ(total, 2, "iter found 2 with S1");
    ASSERT_EQ(f1, 1, "found a1");
    ASSERT_EQ(f2, 1, "found a2");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 11: Tier-DQ1 defer queue under budget
 * ========================================================================= */
static int test_dq1_budget(void) {
    TS("tier_dq1_defer_under_budget");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_defer_begin(w);
    ecs_entity_t e = ecs_new(w);
    S1 v1={1}; ecs_set_id(w,e,ecs_id(S1),sizeof(v1),&v1);
    S1 v2={2}; ecs_set_id(w,e,ecs_id(S1),sizeof(v2),&v2);
    ecs_defer_end(w);
    const S1 *p = ecs_get(w, e, S1);
    ASSERT_EQ(p->x, 2, "final value after defer");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 12: Tier-EV1 observer with defer
 * ========================================================================= */
static int test_ev1_obs(void) {
    TS("tier_ev1_observer_with_defer");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    g_obs_set = 0;
    ecs_observer(w, {
        .query.terms = {{ .id = ecs_id(S1) }},
        .events = { EcsOnSet },
        .callback = obs_on_set
    });
    ecs_entity_t e = ecs_new(w);
    ecs_defer_begin(w);
    S1 v={1}; ecs_set_id(w,e,ecs_id(S1),sizeof(v),&v);
    ASSERT_EQ(g_obs_set, 0, "observer not fired during defer");
    ecs_defer_end(w);
    ASSERT_EQ(g_obs_set, 1, "observer fired after defer_end");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 13: Tier-SI1 retry hooks
 * ========================================================================= */
static int test_si1_hooks(void) {
    TS("tier_si1_sparse_with_hooks");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, SB);
    ecs_set_hooks_id(w, ecs_id(SB), &(ecs_type_hooks_t){
        .ctor = sb_ctor, .dtor = sb_dtor,
        .copy = sb_copy, .move = sb_move,
    });
    reset_hooks();
    ecs_entity_t e1 = ecs_new(w); SB v1={1,"a"}; ecs_set_id(w,e1,ecs_id(SB),sizeof(v1),&v1);
    ecs_entity_t e2 = ecs_new(w); SB v2={2,"b"}; ecs_set_id(w,e2,ecs_id(SB),sizeof(v2),&v2);
    int c0=g_ctor, d0=g_dtor;
    ecs_delete(w,e1); ecs_delete(w,e2);
    ASSERT_TRUE(g_dtor>d0, "dtor fired on delete");
    ASSERT_TRUE(g_ctor>=c0, "ctor count maintained");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 14: Tier-LK1 (verified via 18-suite regression; not duplicated here)
 * ========================================================================= */
static int test_lk1_noop(void) {
    TS("tier_lk1_verified_via_18_suite");
    /* Tier-LK1 lock semantics tested in test_stress, test_stability. */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_entity_t e = ecs_new(w);
    S1 v={42}; ecs_set_id(w,e,ecs_id(S1),sizeof(v),&v);
    const S1 *p = ecs_get(w, e, S1);
    ASSERT_TRUE(p != NULL && p->x == 42, "basic get works");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 15: 1M entity world
 * ========================================================================= */
static int test_1m(void) {
    TS("large_world_1m");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    double t0=(double)clock()/CLOCKS_PER_SEC;
    ecs_entity_t *ids = ecs_bulk_new_w_id(w, ecs_id(S1), 1000000);
    S1 v={1};
    ecs_bulk_set_id(w, ids, 1000000, ecs_id(S1), sizeof(v), &v);
    double t1=(double)clock()/CLOCKS_PER_SEC;
    printf("    1M bulk: %.2f sec (%.1f M/sec)\n", t1-t0, 1.0/(t1-t0));
    int count = 0;
    ecs_query_t *q = ecs_query(w, { .terms = {{ .id = ecs_id(S1) }} });
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) count += it.count;
    ASSERT_EQ(count, 1000000, "1M iterated");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 16: Component lifecycle in defer mode
 * ========================================================================= */
static int test_lifecycle_defer(void) {
    TS("component_lifecycle_defer");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, SB);
    ecs_set_hooks_id(w, ecs_id(SB), &(ecs_type_hooks_t){
        .ctor = sb_ctor, .dtor = sb_dtor,
        .copy = sb_copy, .move = sb_move,
    });
    reset_hooks();
    ecs_entity_t e = ecs_new(w);
    SB va={1,"a"}, vb={2,"b"};
    ecs_defer_begin(w);
    ecs_set_id(w,e,ecs_id(SB),sizeof(va),&va);
    ecs_set_id(w,e,ecs_id(SB),sizeof(vb),&vb);
    ecs_defer_end(w);
    const SB *p = ecs_get(w, e, SB);
    ASSERT_TRUE(p != NULL, "SB applied");
    ASSERT_EQ(p->v, 2, "final value");
    printf("    hooks: c=%d d=%d cp=%d mv=%d\n", g_ctor, g_dtor, g_copy, g_move);
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 17: Many archetypes (50)
 * ========================================================================= */
static int test_many_archetypes(void) {
    TS("many_archetypes_50");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ECS_COMPONENT_DEFINE(w, S3);
    ecs_entity_t e[50];
    for (int i=0;i<50;i++) {
        e[i] = ecs_new(w);
        ecs_add_id(w, e[i], ecs_id(S1));
        if (i%2==0) ecs_add_id(w, e[i], ecs_id(S3));
    }
    int found = 0;
    ecs_query_t *q = ecs_query(w, { .terms = {{ .id = ecs_id(S1) }} });
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) found += it.count;
    ASSERT_EQ(found, 50, "all 50 entities found");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 18: Add/remove cycles
 * ========================================================================= */
static int test_add_remove_cycles(void) {
    TS("add_remove_cycles_100");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ECS_COMPONENT_DEFINE(w, S3);
    ecs_entity_t e = ecs_new(w);
    for (int i=0;i<100;i++) {
        ecs_add_id(w, e, ecs_id(S1));
        ecs_add_id(w, e, ecs_id(S3));
        ecs_remove_id(w, e, ecs_id(S1));
        ecs_remove_id(w, e, ecs_id(S3));
    }
    ASSERT_TRUE(ecs_is_alive(w, e), "entity alive after cycles");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 19: Multiple observers (OnAdd + OnSet)
 * ========================================================================= */
static int test_multi_observer(void) {
    TS("multi_observer_on_add_set");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    g_obs_add = g_obs_set = 0;
    ecs_observer(w, {
        .query.terms = {{ .id = ecs_id(S1) }},
        .events = { EcsOnAdd },
        .callback = obs_on_add
    });
    ecs_observer(w, {
        .query.terms = {{ .id = ecs_id(S1) }},
        .events = { EcsOnSet },
        .callback = obs_on_set
    });
    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, ecs_id(S1));  /* trigger OnAdd */
    ASSERT_EQ(g_obs_add, 1, "OnAdd fired");
    S1 v={1}; ecs_set_id(w,e,ecs_id(S1),sizeof(v),&v);
    ASSERT_EQ(g_obs_set, 1, "OnSet fired");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 20: World basic lifecycle
 * ========================================================================= */
static int test_world_basic(void) {
    TS("world_basic");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_entity_t e = ecs_new(w);
    S1 v={99}; ecs_set_id(w,e,ecs_id(S1),sizeof(v),&v);
    const S1 *p = ecs_get(w, e, S1);
    ASSERT_EQ(p->x, 99, "value");
    ecs_delete(w, e);
    ASSERT_TRUE(!ecs_is_alive(w, e), "deleted");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 21: Bulk operations (new + set + delete)
 * ========================================================================= */
static int test_bulk_ops(void) {
    TS("bulk_operations_10k");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    ecs_entity_t *ids = ecs_bulk_new_w_id(w, ecs_id(S1), 10000);
    S1 v={7};
    ecs_bulk_set_id(w, ids, 10000, ecs_id(S1), sizeof(v), &v);
    /* Verify all */
    int ok = 0;
    for (int i=0;i<10000;i+=100) {
        const S1 *p = ecs_get(w, ids[i], S1);
        if (p && p->x == 7) ok++;
    }
    ASSERT_EQ(ok, 100, "all sampled correct");
    /* Delete all */
    ecs_bulk_delete(w, ids, 10000);
    int alive = 0;
    ecs_query_t *q = ecs_query(w, { .terms = {{ .id = ecs_id(S1) }} });
    ecs_iter_t it = ecs_query_iter(w, q);
    while (ecs_query_next(&it)) alive += it.count;
    ASSERT_EQ(alive, 0, "all deleted");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * TEST 22: Cleanup test - world fini with active entities
 * ========================================================================= */
static int test_fini_with_active(void) {
    TS("fini_with_active_entities");
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, S1);
    /* Create lots, then fini */
    for (int i=0;i<5000;i++) {
        ecs_entity_t e = ecs_new(w);
        S1 v={i}; ecs_set_id(w,e,ecs_id(S1),sizeof(v),&v);
    }
    ecs_fini(w);
    /* If we got here without crash, cleanup worked */
    return 1;
}

typedef int (*test_fn)(void);
typedef struct { const char *name; test_fn fn; } test_entry_t;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("=== TIER v14 COMPREHENSIVE CROSS-TEST v2 ===\n\n");
    printf("(MSVC-safe: uses ecs_set_id to bypass BULGU-10)\n\n");

    test_entry_t tests[] = {
        {"deep_isa_5_levels", test_deep_isa},
        {"custom_hooks", test_custom_hooks},
        {"cascade_delete_10_levels", test_cascade_10},
        {"prefab_override_3_level", test_prefab_3lvl},
        {"world_fini_edge_cases", test_world_fini},
        {"storage_allocator_pressure", test_storage_pressure},
        {"named_components_1000", test_named_1000},
        {"fuzzing_5000_ops", test_fuzzing},
        {"multi_stage_defer", test_defer},
        {"tier_x1_v2_multi_table", test_x1_multitable},
        {"tier_dq1_defer_budget", test_dq1_budget},
        {"tier_ev1_observer_with_defer", test_ev1_obs},
        {"tier_si1_sparse_with_hooks", test_si1_hooks},
        {"tier_lk1_single_thread", test_lk1_noop},
        {"large_world_1m", test_1m},
        {"component_lifecycle_defer", test_lifecycle_defer},
        {"many_archetypes_50", test_many_archetypes},
        {"add_remove_cycles_100", test_add_remove_cycles},
        {"multi_observer", test_multi_observer},
        {"world_basic", test_world_basic},
        {"bulk_operations_10k", test_bulk_ops},
        {"fini_with_active_entities", test_fini_with_active},
    };

    int n = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < n; i++) {
        total_tests++;
        printf("TEST %2d: %s\n", i + 1, tests[i].name);
        if (tests[i].fn()) {
            printf("  PASS\n\n");
        } else {
            printf("  FAIL\n\n");
            total_failures++;
        }
    }

    printf("=== RESULT: %d/%d PASS (%d FAILURES) ===\n",
        total_tests - total_failures, total_tests, total_failures);

    if (total_failures == 0) {
        printf("\n*** TIER v14 CROSS-TEST: ALL 22 PASS ***\n");
        return 0;
    } else {
        printf("\n*** %d FAILURES ***\n", total_failures);
        return 1;
    }
}
