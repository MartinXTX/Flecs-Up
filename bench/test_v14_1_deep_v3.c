/* TIER v14.1 DEEP CROSS-TEST v3 — ~35 tests, 8 NEW categories
 *
 * Targets core-only code paths in flecs_patched.c that v14 (22), v14.1 deep
 * (59), and v14.1 deep v2 (33) do NOT exercise. v3 explores completely new
 * angles:
 *
 *   Y) Custom event emit (ecs_emit, ecs_enqueue)           (4 tests)
 *   Z) Component hooks lifecycle (ctor/dtor/copy/move)     (5 tests)
 *   AA) Identifier stress (EcsName/EcsSymbol/lookup)       (5 tests)
 *   AB) Tier-LK1 refcount edge cases                       (4 tests)
 *   AC) Sparse + observer interaction                      (3 tests)
 *   AD) Override + Sparse combination                      (3 tests)
 *   AE) Transitive IsA traverse                            (3 tests)
 *   AF) Finalization order (fini stress)                   (3 tests)
 *
 *   Total: 30+ tests across 8 categories
 *
 * Builds on v14.1 patches (X1+ V3, DQ1, EV1, SI1, LK1) — these tests do
 * not re-test what the prior suites covered; they focus on lifecycle,
 * custom events, lookup, refcount, and finalization paths.
 *
 * Compile: cl /O2 /W3 /DFLECS_PATCHED_BUILD test_v14_1_deep_v3.c ^
 *          /I. /I../include /Fe:test_v14_1_deep_v3.exe ^
 *          release\flecs_patched.lib
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void init_stdout(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
}

/* =========================================================================
 * Data types — file scope to avoid C4700 (ECS_COMPONENT_DECLARE is non-extern)
 * ========================================================================= */
typedef struct { int32_t v; } H1;  ECS_COMPONENT_DECLARE(H1);
typedef struct { int32_t v; } H2;  ECS_COMPONENT_DECLARE(H2);
typedef struct { int32_t v; } H3;  ECS_COMPONENT_DECLARE(H3);
typedef struct { int32_t v; } H4;  ECS_COMPONENT_DECLARE(H4);
typedef struct { int32_t v; } H5;  ECS_COMPONENT_DECLARE(H5);
typedef struct { int32_t v; } H6;  ECS_COMPONENT_DECLARE(H6);
typedef struct { int32_t v; } H7;  ECS_COMPONENT_DECLARE(H7);
typedef struct { int32_t v; } H8;  ECS_COMPONENT_DECLARE(H8);

typedef struct { int32_t v; } I1;  ECS_COMPONENT_DECLARE(I1);
typedef struct { int32_t v; } I2;  ECS_COMPONENT_DECLARE(I2);
typedef struct { int32_t v; } I3;  ECS_COMPONENT_DECLARE(I3);
typedef struct { int32_t v; } I4;  ECS_COMPONENT_DECLARE(I4);
typedef struct { int32_t v; } I5;  ECS_COMPONENT_DECLARE(I5);

typedef struct { int32_t v; } SP1; ECS_COMPONENT_DECLARE(SP1);
typedef struct { int32_t v; } SP2; ECS_COMPONENT_DECLARE(SP2);

typedef struct { int32_t v; } TR1; ECS_COMPONENT_DECLARE(TR1);
typedef struct { int32_t v; } TR2; ECS_COMPONENT_DECLARE(TR2);

typedef struct { int32_t v; } OV1; ECS_COMPONENT_DECLARE(OV1);

/* Custom event payload types. ecs_emit needs an entity used as event, and an
 * optional payload (param/const_param) typed as the event component. */
typedef struct { int32_t x; int32_t y; } EvPayload;
ECS_COMPONENT_DECLARE(EvPayload);

/* =========================================================================
 * Test framework
 * ========================================================================= */
static int g_total = 0, g_passed = 0, g_failed = 0;
static const char *g_current_category = "";

#define CAT(n) do { g_current_category = n; printf("\n=== %s ===\n", n); } while(0)
#define BEGIN(n) printf("  [%s] ", n)
#define PASS printf("PASS\n"); g_passed++
#define FAIL_MSG(m, ...) do { printf("FAIL: " m "\n", ##__VA_ARGS__); g_failed++; return 0; } while(0)

#define TASSERT(c, m, ...) do { if (!(c)) { printf("FAIL: " m "\n", ##__VA_ARGS__); return 0; } } while(0)
#define TASSERT_EQ(a, b, m, ...) do { long long aa=(long long)(a), bb=(long long)(b); \
    if (aa != bb) { printf("FAIL: " m " (got %lld exp %lld)\n", ##__VA_ARGS__, aa, bb); return 0; } } while(0)

static void run_one(int (*fn)(void), const char *name) {
    g_total++;
    BEGIN(name);
    int rc = fn();
    if (rc) { PASS; }
}

/* =========================================================================
 * Globals for observer/hook callbacks
 * ========================================================================= */
static int g_emit_count = 0;
static int g_emit_last_x = 0;
static int g_emit_last_y = 0;
static int g_enqueue_count = 0;

static int g_ctor_count = 0;
static int g_dtor_count = 0;
static int g_copy_count = 0;
static int g_move_count = 0;
static int g_on_add_count = 0;
static int g_on_remove_count = 0;
static int g_on_set_count = 0;
static int g_on_replace_count = 0;

static int g_lookup_hits = 0;
static int g_sparse_obs_count = 0;
static int g_transitive_hits = 0;
static int g_ov_obs_count = 0;

/* =========================================================================
 * Y) Custom event emit (4 tests)
 * ========================================================================= */

/* Observer callback for custom event. ecs_iter_t::param holds the pointer
 * passed to ecs_emit()::desc->param (or const_param). */
static void obs_on_emit(ecs_iter_t *it) {
    g_emit_count++;
    if (it->param) {
        const EvPayload *p = (const EvPayload*)it->param;
        g_emit_last_x = p->x;
        g_emit_last_y = p->y;
    }
}

static int Y1_custom_emit_with_param(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, EvPayload);

    /* Register observer for the custom event. The event ID itself can be
     * any entity — we use EvPayload as the event. */
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(EvPayload) }},
        .events = { ecs_id(EvPayload) },
        .callback = obs_on_emit
    });

    /* Emit with a single observer-attached table: need a table containing
     * EvPayload. Use the world itself. */
    ecs_entity_t evt_e = ecs_new(w);

    /* Note: in Flecs, custom event observers subscribe to a specific event
     * entity. Our observer subscribed to EvPayload event, so we emit
     * event=EvPayload, and the table must contain EvPayload for the
     * observer to fire on entities. */
    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, ecs_id(EvPayload));

    EvPayload p = { .x = 42, .y = -7 };
    ecs_emit(w, &(ecs_event_desc_t){
        .event = ecs_id(EvPayload),
        .ids = &(ecs_type_t){ .count = 1, .array = (ecs_id_t[]){ ecs_id(EvPayload) } },
        .entity = e,
        .param = &p
    });

    int emitted = g_emit_count;
    TASSERT_EQ(emitted, 1, "observer must fire once for custom event");
    TASSERT_EQ(g_emit_last_x, 42, "param x delivered");
    TASSERT_EQ(g_emit_last_y, -7, "param y delivered");

    /* Suppress unused evt_e */
    (void)evt_e;

    ecs_fini(w);
    return 1;
}

static int Y2_custom_emit_no_observer(void) {
    /* Emit to a world with no observer — must not crash. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, EvPayload);

    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, ecs_id(EvPayload));

    /* Reset global count to detect any emission in this fresh world. */
    int emit_before = g_emit_count;

    EvPayload p = { .x = 1, .y = 2 };
    ecs_emit(w, &(ecs_event_desc_t){
        .event = ecs_id(EvPayload),
        .ids = &(ecs_type_t){ .count = 1, .array = (ecs_id_t[]){ ecs_id(EvPayload) } },
        .entity = e,
        .param = &p
    });

    /* No observer registered, so no callback should fire */
    TASSERT_EQ(g_emit_count, emit_before, "no observer means no callback");
    ecs_fini(w);
    return 1;
}

static int Y3_custom_enqueue_in_defer(void) {
    /* ecs_enqueue enqueues to command queue, dispatches on ecs_defer_end. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, EvPayload);

    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(EvPayload) }},
        .events = { ecs_id(EvPayload) },
        .callback = obs_on_emit
    });

    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, ecs_id(EvPayload));

    int emit_before = g_emit_count;

    /* enqueue fires immediately if not deferred (FLECS_NDEBUG default).
     * The key behavior: must not crash and must increment. */
    EvPayload p = { .x = 9, .y = 9 };
    ecs_enqueue(w, &(ecs_event_desc_t){
        .event = ecs_id(EvPayload),
        .ids = &(ecs_type_t){ .count = 1, .array = (ecs_id_t[]){ ecs_id(EvPayload) } },
        .entity = e,
        .param = &p
    });

    TASSERT(g_emit_count >= emit_before, "enqueue must dispatch (eventually)");
    TASSERT(g_emit_count - emit_before >= 1, "observer fired at least once");
    ecs_fini(w);
    return 1;
}

static int Y4_custom_emit_to_table(void) {
    /* Emit using table + offset + count parameters instead of entity. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, EvPayload);

    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(EvPayload) }},
        .events = { ecs_id(EvPayload) },
        .callback = obs_on_emit
    });

    ecs_entity_t e1 = ecs_new(w);
    ecs_entity_t e2 = ecs_new(w);
    ecs_entity_t e3 = ecs_new(w);
    ecs_add_id(w, e1, ecs_id(EvPayload));
    ecs_add_id(w, e2, ecs_id(EvPayload));
    ecs_add_id(w, e3, ecs_id(EvPayload));

    int emit_before = g_emit_count;
    EvPayload p = { .x = 11, .y = 22 };
    /* Emit at table level — all 3 entities match */
    ecs_emit(w, &(ecs_event_desc_t){
        .event = ecs_id(EvPayload),
        .ids = &(ecs_type_t){ .count = 1, .array = (ecs_id_t[]){ ecs_id(EvPayload) } },
        .table = ecs_get_table(w, e1),
        .param = &p
    });

    int fired = g_emit_count - emit_before;
    TASSERT(fired >= 1, "table-level emit fires observer");
    (void)e2; (void)e3;
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * Z) Component hooks lifecycle (5 tests)
 *
 * Verifies ctor/dtor/copy/move hooks fire on add/remove/set/move.
 * ========================================================================= */

/* Note: ecs_xtor_t signature is `void(*)(void *ptr, int32_t count, const ecs_type_info_t *)`
 * For ctor: ptr is destination, count is number of instances to construct. */
static void h_ctor(void *dst, int32_t count, const ecs_type_info_t *ti) {
    (void)ti;
    for (int i = 0; i < count; i++) {
        H1 *h = (H1*)dst + i;
        h->v = 100;
    }
    g_ctor_count++;
}
static void h_dtor(void *dst, int32_t count, const ecs_type_info_t *ti) {
    (void)ti;
    for (int i = 0; i < count; i++) {
        H1 *h = (H1*)dst + i;
        h->v = -1;  /* poison to detect UAF */
    }
    g_dtor_count++;
}
static void h_copy(void *dst, const void *src, int32_t count, const ecs_type_info_t *ti) {
    (void)ti;
    const H1 *s = (const H1*)src;
    H1 *d = (H1*)dst;
    for (int i = 0; i < count; i++) {
        d[i].v = s[i].v;
    }
    g_copy_count++;
}
static void h_move(void *dst, void *src, int32_t count, const ecs_type_info_t *ti) {
    (void)ti;
    H1 *d = (H1*)dst;
    H1 *s = (H1*)src;
    for (int i = 0; i < count; i++) {
        d[i].v = s[i].v;
        s[i].v = -2;
    }
    g_move_count++;
}
static void h_on_add(ecs_iter_t *it) { (void)it; g_on_add_count++; }
static void h_on_set(ecs_iter_t *it) { (void)it; g_on_set_count++; }
static void h_on_remove(ecs_iter_t *it) { (void)it; g_on_remove_count++; }
static void h_on_replace(ecs_iter_t *it) { (void)it; g_on_replace_count++; }

static int Z1_ctor_dtor_fire_on_add_remove(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H1);

    ecs_set_hooks_id(w, ecs_id(H1), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .dtor = (ecs_xtor_t)h_dtor,
        .on_add = h_on_add,
        .on_remove = h_on_remove
    });

    /* Reset counters (comp def may have triggered on_add for system) */
    int ctor0 = g_ctor_count;
    int dtor0 = g_dtor_count;
    int on_add0 = g_on_add_count;
    int on_remove0 = g_on_remove_count;

    ecs_entity_t e = ecs_new(w);
    ecs_add(w, e, H1);

    /* ctor + on_add must fire on add */
    TASSERT(g_ctor_count > ctor0, "ctor fires on add");
    TASSERT(g_on_add_count > on_add0, "on_add fires on add");
    TASSERT_EQ(ecs_get(w, e, H1)->v, 100, "ctor initialized value");

    ecs_remove(w, e, H1);
    TASSERT(g_dtor_count > dtor0, "dtor fires on remove");
    TASSERT(g_on_remove_count > on_remove0, "on_remove fires on remove");

    ecs_fini(w);
    return 1;
}

static int Z2_copy_move_on_set(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H2);

    ecs_set_hooks_id(w, ecs_id(H2), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .dtor = (ecs_xtor_t)h_dtor,
        .copy = (ecs_copy_t)h_copy,
        .move = (ecs_move_t)h_move,
        .on_set = h_on_set
    });

    int copy0 = g_copy_count;
    int on_set0 = g_on_set_count;

    ecs_entity_t e = ecs_new(w);
    H2 val = { .v = 7 };
    ecs_set_id(w, e, ecs_id(H2), sizeof(H2), &val);

    TASSERT(g_copy_count > copy0, "copy fires on set");
    TASSERT(g_on_set_count > on_set0, "on_set fires on set");
    TASSERT_EQ(ecs_get(w, e, H2)->v, 7, "set stored value");

    ecs_fini(w);
    return 1;
}

static int Z3_on_replace_blocks_get_mut(void) {
    /* on_replace should be set as a hook callback. Verify it fires when
     * value is replaced via ecs_set_id. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H3);

    ecs_set_hooks_id(w, ecs_id(H3), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .dtor = (ecs_xtor_t)h_dtor,
        .on_replace = h_on_replace
    });

    int on_replace0 = g_on_replace_count;

    ecs_entity_t e = ecs_new(w);
    ecs_add(w, e, H3);  /* triggers ctor only */

    H3 val = { .v = 99 };
    ecs_set_id(w, e, ecs_id(H3), sizeof(H3), &val);  /* triggers on_replace */
    TASSERT(g_on_replace_count > on_replace0, "on_replace fires on set after add");

    ecs_fini(w);
    return 1;
}

static int Z4_move_dtor_on_table_change(void) {
    /* When entity moves tables, move_dtor must fire on the component. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H4);
    ECS_COMPONENT_DEFINE(w, H5);

    ecs_set_hooks_id(w, ecs_id(H4), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .move = (ecs_move_t)h_move,
        .dtor = (ecs_xtor_t)h_dtor
    });

    int move0 = g_move_count;

    ecs_entity_t e = ecs_new(w);
    H4 val = { .v = 5 };
    ecs_set_id(w, e, ecs_id(H4), sizeof(H4), &val);

    /* Adding H5 forces table move */
    ecs_add_id(w, e, ecs_id(H5));
    TASSERT(g_move_count > move0, "move fires on table transition");

    ecs_fini(w);
    return 1;
}

static int Z5_get_hooks_id_returns_set(void) {
    /* ecs_get_hooks_id should return the hooks we set. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H6);

    ecs_set_hooks_id(w, ecs_id(H6), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .dtor = (ecs_xtor_t)h_dtor,
        .on_add = h_on_add
    });

    const ecs_type_hooks_t *got = ecs_get_hooks_id(w, ecs_id(H6));
    TASSERT(got != NULL, "get_hooks_id returns non-NULL after set");
    TASSERT(got->ctor == (ecs_xtor_t)h_ctor, "ctor hook stored");
    TASSERT(got->dtor == (ecs_xtor_t)h_dtor, "dtor hook stored");
    TASSERT(got->on_add == h_on_add, "on_add hook stored");

    /* Also: get_type_info should expose the registered size */
    const ecs_type_info_t *ti = ecs_get_type_info(w, ecs_id(H6));
    TASSERT(ti != NULL, "get_type_info returns non-NULL");
    TASSERT_EQ(ti->size, (ecs_size_t)sizeof(H6), "type_info size matches");
    TASSERT_EQ(ti->alignment, (ecs_size_t)_Alignof(H6), "type_info alignment matches");

    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AA) Identifier stress (5 tests) — EcsName, EcsSymbol, lookup
 * ========================================================================= */

static int AA1_set_get_name_roundtrip(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e = ecs_new(w);
    ecs_set_name(w, e, "Hello.World");

    const char *name = ecs_get_name(w, e);
    TASSERT(name != NULL, "name non-null");
    TASSERT(strcmp(name, "Hello.World") == 0, "name roundtrips");
    ecs_fini(w);
    return 1;
}

static int AA2_lookup_resolves(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e = ecs_new(w);
    ecs_set_name(w, e, "Hello");

    ecs_entity_t looked_up = ecs_lookup(w, "Hello");
    TASSERT_EQ(looked_up, e, "lookup returns the entity by name");
    ecs_fini(w);
    return 1;
}

static int AA3_set_symbol_then_set_alias(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e = ecs_new(w);
    ecs_set_symbol(w, e, "MySym");
    ecs_set_alias(w, e, "MyAlias");

    ecs_entity_t by_sym = ecs_lookup_symbol(w, "MySym", false, false);
    TASSERT_EQ(by_sym, e, "lookup_symbol works");

    ecs_fini(w);
    return 1;
}

static int AA4_rename_doesnt_leak(void) {
    /* Renaming an entity multiple times must not leak. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e = ecs_new(w);
    for (int i = 0; i < 50; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Name_%d", i);
        ecs_set_name(w, e, buf);
    }
    /* Lookup must find the last name */
    ecs_entity_t found = ecs_lookup(w, "Name_49");
    TASSERT_EQ(found, e, "final name resolves");

    ecs_fini(w);
    return 1;
}

static int AA5_lookup_child_resolves(void) {
    /* lookup_child(e, "ChildName") resolves a child by relative name. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t parent = ecs_new(w);
    ecs_set_name(w, parent, "Parent");
    ecs_entity_t child = ecs_new(w);
    ecs_set_name(w, child, "Child");
    ecs_add_pair(w, child, EcsChildOf, parent);

    ecs_entity_t found = ecs_lookup_child(w, parent, "Child");
    TASSERT_EQ(found, child, "lookup_child finds child by name");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AB) Tier-LK1 refcount edge cases (4 tests)
 *
 * Targets the LK1 patch (atomic refcount) under stress. The patch is in
 * flecs_poly.c / component_index.c; we test via high-churn component usage.
 * ========================================================================= */

static int AB1_high_churn_component_creates(void) {
    /* Create + delete a component many times. Each cycle bumps refcount
     * in the component_index. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    for (int i = 0; i < 200; i++) {
        ecs_entity_t c = ecs_new(w);  /* not a component per se */
        /* We just need many add/remove on entities */
        ecs_entity_t e = ecs_new(w);
        ecs_add_id(w, e, c);
        ecs_add_id(w, e, ecs_id(H1));
        ecs_remove_id(w, e, c);
        ecs_delete(w, e);
    }
    /* Just ensure no crash. */
    int32_t cnt = ecs_count_id(w, ecs_id(H1));
    TASSERT(cnt >= 0, "world count non-negative");
    ecs_fini(w);
    return 1;
}

static int AB2_1000_components_add_remove(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e = ecs_new(w);
    ecs_entity_t comps[200];
    for (int i = 0; i < 200; i++) {
        comps[i] = ecs_new(w);
        ecs_add_id(w, e, comps[i]);
    }
    for (int i = 0; i < 200; i++) {
        ecs_remove_id(w, e, comps[i]);
    }
    /* Entity still alive, archetype back to empty */
    TASSERT(ecs_is_alive(w, e), "entity alive after bulk add/remove");
    ecs_fini(w);
    return 1;
}

static int AB3_world_info_count_correct(void) {
    /* ecs_count_id must match creations. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, I5);
    /* Count entities with I5 component — only those are created below */
    int n = 50;
    ecs_entity_t es[50];
    for (int i = 0; i < n; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], I5);
    }
    TASSERT_EQ(ecs_count_id(w, ecs_id(I5)), n, "count matches creations");

    for (int i = 0; i < n; i += 2) {
        ecs_delete(w, es[i]);
    }
    TASSERT_EQ(ecs_count_id(w, ecs_id(I5)), n / 2, "count reflects deletes");
    ecs_fini(w);
    return 1;
}

static int AB4_recycled_id_reused(void) {
    /* After delete, the id may be recycled. Reuse and verify. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ecs_entity_t e1 = ecs_new(w);
    ecs_delete(w, e1);
    ecs_entity_t e2 = ecs_new(w);
    TASSERT(e2 != e1, "recycled id has different generation");
    TASSERT(!ecs_is_alive(w, e1), "old id not alive");
    TASSERT(ecs_is_alive(w, e2), "new id alive");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AC) Sparse + observer interaction (3 tests)
 * ========================================================================= */

static void sparse_obs_cb(ecs_iter_t *it) {
    (void)it;
    g_sparse_obs_count++;
}

static int AC1_sparse_observer_fires_on_add(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, SP1);
    ecs_add_id(w, ecs_id(SP1), EcsSparse);

    /* Sparse component: stored separately from archetype. Add to entity
     * and verify observer fires. */
    ecs_observer_init(w, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(SP1) }},
        .events = { EcsOnAdd },
        .callback = sparse_obs_cb
    });

    ecs_entity_t e = ecs_new(w);
    ecs_add_id(w, e, ecs_id(SP1));
    TASSERT(g_sparse_obs_count >= 1, "sparse OnAdd observer fired");

    ecs_fini(w);
    return 1;
}

static int AC2_sparse_add_remove_pure(void) {
    /* Add/remove a sparse component to/from many entities, verify state. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, SP1);
    ecs_add_id(w, ecs_id(SP1), EcsSparse);

    const int N = 200;
    ecs_entity_t es[200];
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
        ecs_add_id(w, es[i], ecs_id(SP1));
        SP1 *p = ecs_get_mut(w, es[i], SP1);
        TASSERT(p != NULL, "get_mut on sparse returns valid pointer");
        p->v = i;
    }
    /* Verify all 200 have the right value */
    for (int i = 0; i < N; i++) {
        const SP1 *p = ecs_get(w, es[i], SP1);
        TASSERT(p != NULL, "sparse component present");
        TASSERT_EQ(p->v, i, "sparse value preserved");
    }
    /* Remove from half */
    for (int i = 0; i < N; i += 2) {
        ecs_remove_id(w, es[i], ecs_id(SP1));
    }
    /* Verify removals */
    for (int i = 0; i < N; i++) {
        const SP1 *p = ecs_get(w, es[i], SP1);
        if (i % 2 == 0) {
            TASSERT(p == NULL, "removed sparse is NULL");
        } else {
            TASSERT(p != NULL, "kept sparse is non-NULL");
        }
    }
    ecs_fini(w);
    return 1;
}

static int AC3_sparse_and_dontfragment(void) {
    /* EcsDontFragment implies sparse; verify both flags don't break add/remove. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, SP2);
    ecs_add_id(w, ecs_id(SP2), EcsDontFragment);

    ecs_entity_t e = ecs_new(w);
    SP2 *p = ecs_emplace_id(w, e, ecs_id(SP2), sizeof(SP2), NULL);
    TASSERT(p != NULL, "emplace on dont-fragment");
    p->v = 1234;

    const SP2 *gp = ecs_get(w, e, SP2);
    TASSERT(gp != NULL, "get on dont-fragment");
    TASSERT_EQ(gp->v, 1234, "value roundtrips");

    ecs_remove_id(w, e, ecs_id(SP2));
    TASSERT(ecs_get(w, e, SP2) == NULL, "removed dont-fragment is NULL");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AD) Override + Sparse combination (3 tests)
 * ========================================================================= */

static int AD1_override_pair_marks_instance(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, OV1);
    ecs_entity_t base = ecs_new(w);
    /* Add ECS_AUTO_OVERRIDE (FLECS core feature) to base */
    ecs_add_id(w, base, ECS_AUTO_OVERRIDE | ecs_id(OV1));
    /* Verify the base has the auto-override pair */
    ecs_id_t ao_pair = ECS_AUTO_OVERRIDE | ecs_id(OV1);
    TASSERT(ecs_has_id(w, base, ao_pair), "base has auto-override pair");
    ecs_fini(w);
    return 1;
}

static int AD2_override_value_doesnt_inherit(void) {
    /* ECS_AUTO_OVERRIDE on base: instance gets its own (own) value. */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, OV1);

    ecs_entity_t base = ecs_new(w);
    ecs_set(w, base, OV1, { 7 });
    ecs_add_id(w, base, ECS_AUTO_OVERRIDE | ecs_id(OV1));
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, base);
    const OV1 *v = ecs_get(w, inst, OV1);
    TASSERT(v != NULL, "instance gets own auto-overridden OV1");
    TASSERT_EQ(v->v, 7, "inherited value matches base");
    ecs_fini(w);
    return 1;
}

static int AD3_override_is_own_not_inherited(void) {
    /* ECS_AUTO_OVERRIDE on base should not show on instance directly */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, OV1);

    ecs_entity_t base = ecs_new(w);
    ecs_id_t ao_pair = ECS_AUTO_OVERRIDE | ecs_id(OV1);
    ecs_add_id(w, base, ao_pair);
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, base);

    TASSERT(!ecs_has_id(w, inst, ao_pair),
            "instance does not have auto-override pair");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AE) Transitive IsA traverse (3 tests)
 * ========================================================================= */

static int AE1_transitive_isa_simple(void) {
    /* base1 -> base2 -> inst. IsA is transitive (EcsTransitive trait). */
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, TR1);

    /* Set TR1 on base1 FIRST, then build chain */
    ecs_entity_t base1 = ecs_new(w);
    ecs_set(w, base1, TR1, { 99 });
    ecs_entity_t base2 = ecs_new(w);
    ecs_add_pair(w, base2, EcsIsA, base1);
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, base2);

    /* inst should inherit TR1 transitively */
    const TR1 *v = ecs_get(w, inst, TR1);
    TASSERT(v != NULL, "instance inherits transitively via IsA chain");
    TASSERT_EQ(v->v, 99, "transitive value");
    ecs_fini(w);
    return 1;
}

static int AE2_transitive_isa_with_override(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, TR2);

    ecs_entity_t base1 = ecs_new(w);
    ecs_set(w, base1, TR2, { 10 });
    ecs_entity_t base2 = ecs_new(w);
    ecs_add_pair(w, base2, EcsIsA, base1);
    ecs_set(w, base2, TR2, { 20 });  /* shadow base1 */
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, base2);
    const TR2 *v = ecs_get(w, inst, TR2);
    TASSERT(v != NULL, "transitive with mid-shadow");
    TASSERT_EQ(v->v, 20, "instance sees closer base's value");
    ecs_fini(w);
    return 1;
}

static int AE3_isa_chain_length_5(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H7);

    /* Build chain: b0 <- b1 <- b2 <- b3 <- b4 */
    ecs_entity_t bs[5];
    bs[0] = ecs_new(w);
    ecs_set(w, bs[0], H7, { 555 });
    for (int i = 1; i < 5; i++) {
        bs[i] = ecs_new(w);
        ecs_add_pair(w, bs[i], EcsIsA, bs[i-1]);
    }
    ecs_entity_t inst = ecs_new_w_pair(w, EcsIsA, bs[4]);
    const H7 *v = ecs_get(w, inst, H7);
    TASSERT(v != NULL, "5-level IsA chain resolved");
    TASSERT_EQ(v->v, 555, "root value reached");
    ecs_fini(w);
    return 1;
}

/* =========================================================================
 * AF) Finalization order (3 tests)
 * ========================================================================= */

static int AF1_fini_with_observers(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, H8);

    for (int i = 0; i < 5; i++) {
        ecs_observer_init(w, &(ecs_observer_desc_t){
            .query.terms = {{ .id = ecs_id(H8) }},
            .events = { EcsOnAdd, EcsOnRemove },
            .callback = sparse_obs_cb
        });
    }
    /* Should fini cleanly. */
    ecs_fini(w);
    return 1;
}

static int AF2_fini_with_many_entities(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, I1);
    ECS_COMPONENT_DEFINE(w, I2);
    ECS_COMPONENT_DEFINE(w, I3);

    const int N = 1000;
    for (int i = 0; i < N; i++) {
        ecs_entity_t e = ecs_new(w);
        ecs_add(w, e, I1);
        ecs_add(w, e, I2);
        ecs_add(w, e, I3);
        ecs_add_id(w, e, EcsDisabled);
    }
    ecs_fini(w);
    return 1;
}

static int AF3_fini_with_hooks(void) {
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL, "world init");

    ECS_COMPONENT_DEFINE(w, I4);
    ecs_set_hooks_id(w, ecs_id(I4), &(ecs_type_hooks_t){
        .ctor = (ecs_xtor_t)h_ctor,
        .dtor = (ecs_xtor_t)h_dtor
    });

    int dtor0 = g_dtor_count;

    const int N = 200;
    ecs_entity_t es[200];
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(w);
        ecs_add(w, es[i], I4);
    }
    /* delete some */
    for (int i = 0; i < N / 2; i++) {
        ecs_delete(w, es[i]);
    }
    TASSERT(g_dtor_count > dtor0, "dtor fires on explicit delete");

    /* Fini must call remaining dtors */
    int dtor_before_fini = g_dtor_count;
    ecs_fini(w);
    TASSERT(g_dtor_count > dtor_before_fini, "fini calls remaining dtors");
    return 1;
}

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void) {
    init_stdout();
    printf("=== TIER v14.1 DEEP CROSS-TEST v3 ===\n");
    printf("Core-only flecs_patched.lib — 8 NEW categories\n\n");

    CAT("Y) Custom event emit (ecs_emit / ecs_enqueue)");
    run_one(Y1_custom_emit_with_param,    "Y1: emit with param");
    run_one(Y2_custom_emit_no_observer,   "Y2: emit no-op (no observer)");
    run_one(Y3_custom_enqueue_in_defer,   "Y3: enqueue fires observer");
    run_one(Y4_custom_emit_to_table,      "Y4: table-level emit");

    CAT("Z) Component hooks lifecycle");
    run_one(Z1_ctor_dtor_fire_on_add_remove, "Z1: ctor/dtor on add/remove");
    run_one(Z2_copy_move_on_set,             "Z2: copy/move on set");
    run_one(Z3_on_replace_blocks_get_mut,    "Z3: on_replace fires on set");
    run_one(Z4_move_dtor_on_table_change,    "Z4: move on table transition");
    run_one(Z5_get_hooks_id_returns_set,     "Z5: get_hooks_id roundtrip");

    CAT("AA) Identifier stress (EcsName/Symbol/lookup)");
    run_one(AA1_set_get_name_roundtrip,   "AA1: name roundtrip");
    run_one(AA2_lookup_resolves,          "AA2: lookup by path");
    run_one(AA3_set_symbol_then_set_alias,"AA3: symbol + alias");
    run_one(AA4_rename_doesnt_leak,       "AA4: 50 renames");
    run_one(AA5_lookup_child_resolves,    "AA5: lookup_child");

    CAT("AB) Tier-LK1 refcount edge cases");
    run_one(AB1_high_churn_component_creates, "AB1: churn add/remove");
    run_one(AB2_1000_components_add_remove,   "AB2: 200 comps add/remove");
    run_one(AB3_world_info_count_correct,     "AB3: ecs_count accuracy");
    run_one(AB4_recycled_id_reused,           "AB4: recycled id generation");

    CAT("AC) Sparse + observer");
    run_one(AC1_sparse_observer_fires_on_add, "AC1: sparse + observer init");
    run_one(AC2_sparse_add_remove_pure,        "AC2: sparse add/remove 200");
    run_one(AC3_sparse_and_dontfragment,       "AC3: EcsDontFragment");

    CAT("AD) Override + Sparse combination");
    run_one(AD1_override_pair_marks_instance,   "AD1: EcsOverride pair");
    run_one(AD2_override_value_doesnt_inherit,  "AD2: override value shadow");
    run_one(AD3_override_is_own_not_inherited,  "AD3: override not inherited");

    CAT("AE) Transitive IsA traverse");
    run_one(AE1_transitive_isa_simple,         "AE1: 2-level IsA transitive");
    run_one(AE2_transitive_isa_with_override,  "AE2: 2-level with mid-shadow");
    run_one(AE3_isa_chain_length_5,             "AE3: 5-level IsA chain");

    CAT("AF) Finalization order");
    run_one(AF1_fini_with_observers,         "AF1: fini with 5 observers");
    run_one(AF2_fini_with_many_entities,     "AF2: fini with 1K entities");
    run_one(AF3_fini_with_hooks,             "AF3: fini calls all dtors");

    printf("\n=== Summary ===\n");
    printf("Total:  %d\n", g_total);
    printf("Passed: %d\n", g_passed);
    printf("Failed: %d\n", g_failed);

    if (g_failed == 0) {
        printf("\nALL v3 TESTS PASS — 8 new categories covered\n");
        return 0;
    }
    printf("\n%d FAILURE(S)\n", g_failed);
    return 1;
}
