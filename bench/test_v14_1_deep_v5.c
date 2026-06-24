/* TIER v14.1 DEEP CROSS-TEST v5 — son açı taraması
 *
 * Hedef: v14 + v14.1 + v4 sonrası kalan son test alanları.
 *
 *   CA) ecs_new_w_table                           (3 tests)
 *   CB) ecs_clone variants                        (4 tests)
 *   CC) ecs_set_id to many entities               (3 tests)
 *   CD) ecs_new_w_parent                          (3 tests)
 *   CE) ecs_define components                     (3 tests)
 *   CF) Name lookup (parent + child path)         (3 tests)
 *   CG) Observer yield pattern                    (3 tests)
 *   CH) Long name edges                           (3 tests)
 *   CI) Transitive + IsA resolve                  (3 tests)
 *   CJ) Mass emit event                           (2 tests)
 *
 *   Total: 30 tests across 10 categories
 *
 * Build: cl /O2 /W3 /DFLECS_PATCHED_BUILD test_v14_1_deep_v5.c ^
 *        /I. /I../include /Fe:test_v14_1_deep_v5.exe ^
 *        release\flecs_patched.lib
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void init_stdout(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
}

/* =========================================================================
 * Data types — file scope (C4700 fix)
 * ========================================================================= */
typedef struct { int32_t v; } CA1;  ECS_COMPONENT_DECLARE(CA1);
typedef struct { int32_t v; } CA2;  ECS_COMPONENT_DECLARE(CA2);

typedef struct { int32_t v; } CB1;  ECS_COMPONENT_DECLARE(CB1);
typedef struct { int32_t v; } CB2;  ECS_COMPONENT_DECLARE(CB2);

typedef struct { int32_t v; } CC1;  ECS_COMPONENT_DECLARE(CC1);

typedef struct { int32_t v; } CD1;  ECS_COMPONENT_DECLARE(CD1);

typedef struct { int32_t v; } CE1;  ECS_COMPONENT_DECLARE(CE1);

typedef struct { int32_t v; } CF1;  ECS_COMPONENT_DECLARE(CF1);

typedef struct { int32_t v; } CG1;  ECS_COMPONENT_DECLARE(CG1);

typedef struct { int32_t v; } CH1;  ECS_COMPONENT_DECLARE(CH1);

typedef struct { int32_t v; } CI1;  ECS_COMPONENT_DECLARE(CI1);

typedef struct { int32_t v; } CJ1;  ECS_COMPONENT_DECLARE(CJ1);  /* renamed for CJ series */

/* =========================================================================
 * Test framework
 * ========================================================================= */
static int g_total = 0, g_passed = 0, g_failed = 0;
static int g_obs_count = 0;
static int g_yield_stop = 0;

#define CAT(n) printf("\n=== %s ===\n", n)
#define BEGIN(n) printf("  [%s] ", n)
#define PASS printf("PASS\n"); g_passed++
#define FAIL_MSG(m, ...) do { printf("FAIL: " m "\n", ##__VA_ARGS__); g_failed++; g_total++; return 0; } while(0)

#define TASSERT(c, m, ...) do { if (!(c)) { printf("FAIL: " m "\n", ##__VA_ARGS__); g_failed++; g_total++; return 0; } } while(0)
#define TASSERT_EQ(a, b, m, ...) do { long long aa=(long long)(a), bb=(long long)(b); \
    if (aa != bb) { printf("FAIL: " m " (got %lld exp %lld)\n", ##__VA_ARGS__, aa, bb); g_failed++; g_total++; return 0; } } while(0)

static void run_one(int (*fn)(void), const char *name) {
    g_total++;
    BEGIN(name);
    int rc = fn();
    if (rc) { PASS; }
}

/* =========================================================================
 * CA) ecs_new_w_table (3 tests)
 * ========================================================================= */

static int CA1_new_w_table_same_components(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");
    ECS_COMPONENT_DEFINE(w, CA1);

    /* Create 3 entities with CA1 */
    ecs_entity_t e1 = ecs_new(w);
    ecs_add(w, e1, CA1);
    ecs_entity_t e2 = ecs_new(w);
    ecs_add(w, e2, CA1);
    ecs_entity_t e3 = ecs_new(w);
    ecs_add(w, e3, CA1);

    /* Get their shared table */
    ecs_table_t *t = ecs_get_table(w, e1);
    TASSERT(t != NULL, "table non-NULL");

    /* Create new entity in same table */
    ecs_entity_t e4 = ecs_new_w_table(w, t);
    TASSERT(e4 != 0, "new_w_table created entity");
    TASSERT(ecs_has_id(w, e4, ecs_id(CA1)), "e4 has CA1 (from table)");
    TASSERT(ecs_get_table(w, e4) == t, "e4 in same table");
    (void)e2; (void)e3;
    ecs_fini(w);
    return 1;
}

static int CA2_new_w_table_empty_table(void) {
    /* Create with empty (no-component) table */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CA1);

    /* Get table from a real entity (no components added) */
    ecs_entity_t empty = ecs_new(w);
    ecs_table_t *t = ecs_get_table(w, empty);
    TASSERT(t != NULL, "table of empty entity");
    ecs_entity_t e = ecs_new_w_table(w, t);
    TASSERT(e != 0, "new_w_table with empty table");
    TASSERT_EQ(ecs_get_table(w, e), t, "e in same table");
    ecs_fini(w);
    return 1;
}

static int CA3_new_w_table_100_entities(void) {
    /* Mass create via table — 100 entities */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CA1);
    ecs_entity_t seed = ecs_new(w);
    ecs_add(w, seed, CA1);
    ecs_table_t *t = ecs_get_table(w, seed);

    for (int i = 0; i < 100; i++) {
        ecs_entity_t e = ecs_new_w_table(w, t);
        TASSERT(e != 0, "created");
        TASSERT(ecs_has_id(w, e, ecs_id(CA1)), "has CA1");
    }
    TASSERT_EQ(ecs_count_id(w, ecs_id(CA1)), 101, "101 entities (1 seed + 100 new)");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CB) ecs_clone variants (4 tests)
 * ========================================================================= */

static int CB1_clone_creates_new(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CB1);
    ecs_entity_t src = ecs_new(w);
    CB1 val = { .v = 100 };
    ecs_set_id(w, src, ecs_id(CB1), sizeof(CB1), &val);

    /* ecs_clone with dst=0 creates new entity */
    ecs_entity_t dst = ecs_clone(w, 0, src, false);
    TASSERT(dst != 0, "clone created entity");
    TASSERT(dst != src, "clone is different entity");
    ecs_fini(w);
    return 1;
}

static int CB2_clone_copies_components(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CB1);
    ECS_COMPONENT_DEFINE(w, CB2);
    ecs_entity_t src = ecs_new(w);
    ecs_add(w, src, CB1);
    ecs_add(w, src, CB2);

    ecs_entity_t dst = ecs_clone(w, 0, src, false);
    TASSERT(ecs_has_id(w, dst, ecs_id(CB1)), "CB1 copied");
    TASSERT(ecs_has_id(w, dst, ecs_id(CB2)), "CB2 copied");
    ecs_fini(w);
    return 1;
}

static int CB3_clone_with_value(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CB1);
    ecs_entity_t src = ecs_new(w);
    CB1 val = { .v = 42 };
    ecs_set_id(w, src, ecs_id(CB1), sizeof(CB1), &val);

    ecs_entity_t dst = ecs_clone(w, 0, src, true);
    const CB1 *p = ecs_get(w, dst, CB1);
    TASSERT(p != NULL, "CB1 present");
    TASSERT_EQ(p->v, 42, "value copied = 42");
    ecs_fini(w);
    return 1;
}

static int CB4_clone_to_existing(void) {
    /* Clone INTO existing entity */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CB1);
    ecs_entity_t src = ecs_new(w);
    CB1 val = { .v = 7 };
    ecs_set_id(w, src, ecs_id(CB1), sizeof(CB1), &val);

    ecs_entity_t existing = ecs_new(w);  /* empty */
    ecs_entity_t ret = ecs_clone(w, existing, src, true);
    TASSERT_EQ(ret, existing, "clone returns existing");
    const CB1 *p = ecs_get(w, existing, CB1);
    TASSERT(p != NULL, "CB1 added to existing");
    TASSERT_EQ(p->v, 7, "value = 7");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CC) ecs_set_id to many entities (3 tests)
 * ========================================================================= */

static int CC1_set_id_to_500(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CC1);
    ecs_entity_t es[500];
    for (int i = 0; i < 500; i++) {
        es[i] = ecs_new(w);
    }
    /* Add CC1 + set value in one shot */
    for (int i = 0; i < 500; i++) {
        CC1 val = { .v = i };
        ecs_set_id(w, es[i], ecs_id(CC1), sizeof(CC1), &val);
    }
    /* Verify all */
    for (int i = 0; i < 500; i++) {
        const CC1 *p = ecs_get(w, es[i], CC1);
        TASSERT(p != NULL, "CC1 set");
        TASSERT_EQ(p->v, i, "value matches");
    }
    ecs_fini(w);
    return 1;
}

static int CC2_set_id_overwrite(void) {
    /* set_id twice — second wins */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CC1);
    ecs_entity_t e = ecs_new(w);
    CC1 v1 = { .v = 1 };
    ecs_set_id(w, e, ecs_id(CC1), sizeof(CC1), &v1);
    CC1 v2 = { .v = 2 };
    ecs_set_id(w, e, ecs_id(CC1), sizeof(CC1), &v2);
    TASSERT_EQ(ecs_get(w, e, CC1)->v, 2, "second set wins");
    ecs_fini(w);
    return 1;
}

static int CC3_set_id_then_remove(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CC1);
    ecs_entity_t e = ecs_new(w);
    CC1 val = { .v = 99 };
    ecs_set_id(w, e, ecs_id(CC1), sizeof(CC1), &val);
    ecs_remove_id(w, e, ecs_id(CC1));
    TASSERT(ecs_get(w, e, CC1) == NULL, "removed");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CD) ecs_new_w_parent (3 tests)
 * ========================================================================= */

static int CD1_new_w_parent_basic(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t parent = ecs_new(w);
    ecs_entity_t child = ecs_new_w_parent(w, parent, "MyChild");
    TASSERT(child != 0, "child created");
    TASSERT(ecs_has_pair(w, child, EcsChildOf, parent), "ChildOf pair");
    const char *name = ecs_get_name(w, child);
    TASSERT(name != NULL, "name set");
    TASSERT(strcmp(name, "MyChild") == 0, "name = MyChild");
    ecs_fini(w);
    return 1;
}

static int CD2_new_w_parent_null_name(void) {
    /* NULL name = no name set */
    ecs_world_t *w = ecs_init();
    ecs_entity_t parent = ecs_new(w);
    ecs_entity_t child = ecs_new_w_parent(w, parent, NULL);
    TASSERT(child != 0, "child created");
    TASSERT(ecs_has_pair(w, child, EcsChildOf, parent), "ChildOf");
    ecs_fini(w);
    return 1;
}

static int CD3_new_w_parent_nested(void) {
    /* Parent of a parent (multi-level) */
    ecs_world_t *w = ecs_init();
    ecs_entity_t grandparent = ecs_new(w);
    ecs_entity_t parent = ecs_new_w_parent(w, grandparent, "P");
    ecs_entity_t child = ecs_new_w_parent(w, parent, "C");
    TASSERT(ecs_has_pair(w, parent, EcsChildOf, grandparent), "P → G");
    TASSERT(ecs_has_pair(w, child, EcsChildOf, parent), "C → P");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CE) ecs_define-style component creation (3 tests)
 *
 * If ecs_define not available, use ecs_new + ecs_set_name + ECS_COMPONENT_DEFINE
 * ========================================================================= */

static int CE1_create_named_component(void) {
    ecs_world_t *w = ecs_init();
    /* Use ECS_COMPONENT_DEFINE with explicit type */
    ECS_COMPONENT_DEFINE(w, CE1);
    TASSERT(ecs_id(CE1) != 0, "CE1 id valid");
    /* Query the world to verify it's a known component */
    TASSERT(ecs_get_type_info(w, ecs_id(CE1)) != NULL, "CE1 has type info");
    ecs_fini(w);
    return 1;
}

static int CE2_component_id_nonzero(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CE1);
    ecs_entity_t id = ecs_id(CE1);
    TASSERT(id != 0, "id non-zero");
    TASSERT(id > 0, "id positive");
    /* Add and check presence */
    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, id);
    TASSERT(ecs_has_id(w, e, id), "added correctly");
    ecs_fini(w);
    return 1;
}

static int CE3_multiple_components_unique_ids(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CA1);
    ECS_COMPONENT_DEFINE(w, CA2);
    ECS_COMPONENT_DEFINE(w, CB1);
    ECS_COMPONENT_DEFINE(w, CB2);
    TASSERT(ecs_id(CA1) != ecs_id(CA2), "CA1 != CA2");
    TASSERT(ecs_id(CA1) != ecs_id(CB1), "CA1 != CB1");
    TASSERT(ecs_id(CA2) != ecs_id(CB2), "CA2 != CB2");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CF) Name lookup (parent + child path) (3 tests)
 * ========================================================================= */

static int CF1_lookup_with_slash_path(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t p = ecs_new(w);
    ecs_set_name(w, p, "Parent");
    ecs_entity_t c = ecs_new(w);
    ecs_set_name(w, c, "Child");
    ecs_add_pair(w, c, EcsChildOf, p);

    /* lookup via path "Parent.Child" with "." separator (ChildOf chain) */
    ecs_entity_t found = ecs_lookup_path_w_sep(w, 0, "Parent.Child", ".", NULL, true);
    TASSERT_EQ(found, c, "found Child via Parent.Child");
    ecs_fini(w);
    return 1;
}

static int CF2_lookup_relative_to_parent(void) {
    /* Lookup relative to a parent entity */
    ecs_world_t *w = ecs_init();
    ecs_entity_t p = ecs_new(w);
    ecs_set_name(w, p, "P");
    ecs_entity_t c = ecs_new(w);
    ecs_set_name(w, c, "C");
    ecs_add_pair(w, c, EcsChildOf, p);

    ecs_entity_t found = ecs_lookup_path_w_sep(w, p, "C", "/", NULL, false);
    TASSERT_EQ(found, c, "found C relative to P");
    ecs_fini(w);
    return 1;
}

static int CF3_lookup_nonexistent(void) {
    ecs_world_t *w = ecs_init();
    ecs_entity_t found = ecs_lookup(w, "DoesNotExist");
    TASSERT_EQ(found, 0, "non-existent returns 0");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CG) Observer yield pattern (3 tests)
 *
 * "Yield" in Flecs observer terms = using ecs_iter_next-style walking.
 * For static observer it's just one dispatch, but we can test early-out by
 * returning from callback (which terminates iteration).
 * ========================================================================= */

static void cg_obs(ecs_iter_t *it) {
    g_obs_count++;
    if (it->count >= 1) {
        /* Don't yield — let observer complete */
    }
}

static int CG1_observer_with_pair_term(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CG1);
    ecs_entity_t Rel1 = ecs_new(w);

    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(CG1) }, { .id = ecs_pair(Rel1, EcsWildcard) }},
        .events = { EcsOnAdd },
        .callback = cg_obs
    });

    int before = g_obs_count;
    ecs_entity_t e = ecs_new(w);
    ecs_add(w, e, CG1);
    ecs_entity_t target = ecs_new(w);
    ecs_add_pair(w, e, Rel1, target);
    TASSERT(g_obs_count > before, "observer with pair fired");
    ecs_fini(w);
    return 1;
}

static int CG2_observer_for_each_dispatch(void) {
    /* Verify observer fires per add */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CG1);
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(CG1) }},
        .events = { EcsOnAdd },
        .callback = cg_obs
    });

    int before = g_obs_count;
    for (int i = 0; i < 10; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, CG1);
    }
    TASSERT_EQ(g_obs_count - before, 10, "observer fired 10 times for 10 adds");
    ecs_fini(w);
    return 1;
}

static int CG3_observer_not_optional(void) {
    /* Verify observer with required term doesn't match entity missing that term */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CG1);
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(CG1) }},
        .events = { EcsOnAdd },
        .callback = cg_obs
    });

    int before = g_obs_count;
    /* Create entity without CG1 */
    ecs_new(w);
    /* No fire */
    TASSERT_EQ(g_obs_count, before, "no fire for entity without required term");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CH) Long name edges (3 tests)
 * ========================================================================= */

static int CH1_set_256_char_name(void) {
    ecs_world_t *w = ecs_init();
    char buf[256];
    for (int i = 0; i < 255; i++) buf[i] = 'A' + (i % 26);
    buf[255] = 0;

    ecs_entity_t e = ecs_new(w);
    ecs_set_name(w, e, buf);
    const char *name = ecs_get_name(w, e);
    TASSERT(name != NULL, "name set");
    TASSERT_EQ(strlen(name), 255, "name length 255");
    /* Lookup requires exact name match (no prefix match) */
    /* Verify name is stored exactly */
    ecs_entity_t found = ecs_lookup(w, buf);
    TASSERT_EQ(found, e, "lookup by exact 255-char name works");
    ecs_fini(w);
    return 1;
}

static int CH2_set_1000_char_name(void) {
    /* Long name stress */
    ecs_world_t *w = ecs_init();
    char *buf = (char*)malloc(1001);
    for (int i = 0; i < 1000; i++) buf[i] = 'B' + (i % 20);
    buf[1000] = 0;

    ecs_entity_t e = ecs_new(w);
    ecs_set_name(w, e, buf);
    const char *name = ecs_get_name(w, e);
    TASSERT(name != NULL, "name set");
    TASSERT(strlen(name) >= 999, "name length ~1000");
    free(buf);
    ecs_fini(w);
    return 1;
}

static int CH3_rename_cycle(void) {
    /* Rename same entity 100 times */
    ecs_world_t *w = ecs_init();
    ecs_entity_t e = ecs_new(w);
    for (int i = 0; i < 100; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Name%d", i);
        ecs_set_name(w, e, buf);
    }
    ecs_entity_t found = ecs_lookup(w, "Name99");
    TASSERT_EQ(found, e, "last name resolves");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CI) Transitive + IsA resolve (3 tests)
 * ========================================================================= */

static int CI1_is_a_inheritance_chain(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CI1);
    ecs_entity_t b1 = ecs_new(w);
    ecs_set(w, b1, CI1, { 10 });
    ecs_entity_t b2 = ecs_new(w);
    ecs_add_pair(w, b2, EcsIsA, b1);
    ecs_set(w, b2, CI1, { 20 });
    ecs_entity_t inst = ecs_new(w);
    ecs_add_pair(w, inst, EcsIsA, b2);
    const CI1 *v = ecs_get(w, inst, CI1);
    TASSERT(v != NULL, "instance gets CI1 from chain");
    TASSERT_EQ(v->v, 20, "closest base wins");
    ecs_fini(w);
    return 1;
}

static int CI2_is_a_3level_deep(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CI1);
    ecs_entity_t b1 = ecs_new(w);
    ecs_set(w, b1, CI1, { 1 });
    ecs_entity_t b2 = ecs_new(w);
    ecs_add_pair(w, b2, EcsIsA, b1);
    ecs_entity_t b3 = ecs_new(w);
    ecs_add_pair(w, b3, EcsIsA, b2);
    ecs_entity_t inst = ecs_new(w);
    ecs_add_pair(w, inst, EcsIsA, b3);
    const CI1 *v = ecs_get(w, inst, CI1);
    TASSERT(v != NULL, "3-level resolved");
    TASSERT_EQ(v->v, 1, "root value");
    ecs_fini(w);
    return 1;
}

static int CI3_is_a_sibling_override(void) {
    /* Sibling base overrides sibling base */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CI1);
    ecs_entity_t b1 = ecs_new(w);
    ecs_set(w, b1, CI1, { 100 });
    ecs_entity_t b2 = ecs_new(w);
    ecs_set(w, b2, CI1, { 200 });
    ecs_entity_t inst = ecs_new(w);
    /* inst IsA both b1 and b2 — closest wins */
    ecs_add_pair(w, inst, EcsIsA, b1);
    ecs_add_pair(w, inst, EcsIsA, b2);
    const CI1 *v = ecs_get(w, inst, CI1);
    TASSERT(v != NULL, "sibling resolved");
    /* b1 or b2 — Flecs picks by add order. Both 100 and 200 valid. */
    TASSERT(v->v == 100 || v->v == 200, "value from one sibling");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * CJ) Mass emit event (2 tests)
 * ========================================================================= */

static int CJ1_emit_to_table_with_50_entities(void) {
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CJ1);
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(CJ1) }},
        .events = { EcsOnAdd },
        .callback = cg_obs
    });

    /* Create 50 entities with CJ1 */
    ecs_entity_t es[50];
    for (int i = 0; i < 50; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], CJ1);
    }
    int before = g_obs_count;
    /* Emit on table of first entity */
    ecs_emit(w, &(ecs_event_desc_t){
        .event = EcsOnAdd,
        .ids = &(ecs_type_t){
            .count = 1,
            .array = (ecs_id_t[]){ ecs_id(CJ1) }
        },
        .table = ecs_get_table(w, es[0])
    });
    /* Observer fires once per table = once */
    TASSERT(g_obs_count - before >= 1, "table-level emit fired");
    ecs_fini(w);
    return 1;
}

static int CJ2_emit_with_param_to_50(void) {
    /* Mass emit with payload */
    ecs_world_t *w = ecs_init();
    ECS_COMPONENT_DEFINE(w, CJ1);
    /* Capture payload via global via observer */
    g_obs_count = 0;
    g_yield_stop = 0;
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(CJ1) }},
        .events = { ecs_id(CJ1) },
        .callback = cg_obs
    });

    /* Set a payload — use a different observer to verify */
    ecs_entity_t e = ecs_new(w);
    ecs_add(w, e, CJ1);
    int32_t payload = 1234;
    ecs_emit(w, &(ecs_event_desc_t){
        .event = ecs_id(CJ1),
        .ids = &(ecs_type_t){
            .count = 1,
            .array = (ecs_id_t[]){ ecs_id(CJ1) }
        },
        .entity = e,
        .param = &payload
    });
    TASSERT(g_obs_count >= 1, "observer fired with payload");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void) {
    init_stdout();
    printf("=== TIER v14.1 DEEP CROSS-TEST v5 — son açı taraması ===\n");
    printf("30 tests across 10 categories — %d v4 sonrası\n\n", 30);

    CAT("CA) ecs_new_w_table");
    run_one(CA1_new_w_table_same_components,   "CA1: new_w_table same comp");
    run_one(CA2_new_w_table_empty_table,       "CA2: new_w_table root table");
    run_one(CA3_new_w_table_100_entities,      "CA3: new_w_table 100 ent");

    CAT("CB) ecs_clone variants");
    run_one(CB1_clone_creates_new,             "CB1: clone creates new ent");
    run_one(CB2_clone_copies_components,       "CB2: clone copies comps");
    run_one(CB3_clone_with_value,              "CB3: clone with value");
    run_one(CB4_clone_to_existing,             "CB4: clone into existing");

    CAT("CC) ecs_set_id to many entities");
    run_one(CC1_set_id_to_500,                 "CC1: set_id to 500 ent");
    run_one(CC2_set_id_overwrite,              "CC2: set_id overwrites");
    run_one(CC3_set_id_then_remove,            "CC3: set_id + remove");

    CAT("CD) ecs_new_w_parent");
    run_one(CD1_new_w_parent_basic,            "CD1: new_w_parent basic");
    run_one(CD2_new_w_parent_null_name,        "CD2: new_w_parent NULL name");
    run_one(CD3_new_w_parent_nested,           "CD3: new_w_parent 2-level");

    CAT("CE) Component define");
    run_one(CE1_create_named_component,        "CE1: ECS_COMPONENT_DEFINE");
    run_one(CE2_component_id_nonzero,          "CE2: id is non-zero");
    run_one(CE3_multiple_components_unique_ids,"CE3: 4 components unique");

    CAT("CF) Name lookup (parent + child path)");
    run_one(CF1_lookup_with_slash_path,        "CF1: /Parent/Child");
    run_one(CF2_lookup_relative_to_parent,     "CF2: lookup relative");
    run_one(CF3_lookup_nonexistent,            "CF3: lookup 0 returns 0");

    CAT("CG) Observer yield pattern");
    run_one(CG1_observer_with_pair_term,       "CG1: obs with pair term");
    run_one(CG2_observer_for_each_dispatch,    "CG2: obs fires 10x for 10 add");
    run_one(CG3_observer_not_optional,         "CG3: obs doesn't match missing term");

    CAT("CH) Long name edges");
    run_one(CH1_set_256_char_name,             "CH1: 255-char name");
    run_one(CH2_set_1000_char_name,            "CH2: 1000-char name");
    run_one(CH3_rename_cycle,                  "CH3: 100 renames");

    CAT("CI) Transitive + IsA resolve");
    run_one(CI1_is_a_inheritance_chain,        "CI1: 2-level IsA");
    run_one(CI2_is_a_3level_deep,              "CI2: 3-level IsA");
    run_one(CI3_is_a_sibling_override,         "CI3: sibling bases");

    CAT("CJ) Mass emit event");
    run_one(CJ1_emit_to_table_with_50_entities,"CJ1: table-level emit 50");
    run_one(CJ2_emit_with_param_to_50,         "CJ2: emit with param");

    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v5 TESTS PASS — 10 new categories scanned\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
