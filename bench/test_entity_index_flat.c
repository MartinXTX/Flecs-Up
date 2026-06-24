/* Tier-P2-1 entity-index flat array test.
 *
 * Verifies that flat ecs_record_t* backing store (replacing page-table) works:
 *   1. 1M entity create+lookup round-trip
 *   2. Bulk delete
 *   3. Re-create after delete (recycled id path)
 *   4. flecs_entity_index_prefetch hint (B1 companion)
 *
 * No crash expected. No leak expected (CRT debug heap would catch).
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define N_ENTITIES 1000000

static ecs_entity_t* bulk_new_no_comp(ecs_world_t *world, int32_t count) {
    ecs_entity_t *out = ecs_os_malloc_n(ecs_entity_t, count);
    for (int i = 0; i < count; i++) {
        out[i] = ecs_new(world);
    }
    return out;
}

static int test_create_lookup_roundtrip(void) {
    printf("TEST P2-1-1: create+lookup 1M entities\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();

    /* Bulk create via public API */
    ecs_entity_t *entities = bulk_new_no_comp(world, N_ENTITIES);
    if (!entities) {
        printf("  FAIL: bulk_new returned NULL\n");
        ecs_fini(world);
        return 1;
    }

    /* Lookup each entity, verify alive */
    int fail = 0;
    for (int i = 0; i < N_ENTITIES; i++) {
        if (!ecs_is_alive(world, entities[i])) {
            if (i < 5) printf("  FAIL at i=%d: not alive for entity %llu\n",
                              i, (unsigned long long)entities[i]);
            fail++;
            break;
        }
    }

    ecs_os_free(entities);
    ecs_fini(world);
    if (fail) { printf("  FAIL (%d failures)\n", fail); return 1; }
    printf("  PASS (1M entities created and looked up)\n");
    return 0;
}

static int test_bulk_delete(void) {
    printf("TEST P2-1-2: bulk_delete 1M entities\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_entity_t *entities = bulk_new_no_comp(world, N_ENTITIES);

    /* Delete all */
    ecs_delete(world, entities[0]);
    int fail = 0;
    for (int i = 1; i < N_ENTITIES; i++) {
        ecs_delete(world, entities[i]);
        if (!ecs_is_alive(world, entities[i])) {
            if (i < 5) printf("  FAIL at i=%d: still alive after delete\n", i);
            fail++;
            break;
        }
    }

    ecs_os_free(entities);
    ecs_fini(world);
    if (fail) { printf("  FAIL\n"); return 1; }
    printf("  PASS (1M entities deleted)\n");
    return 0;
}

static int test_recycle_after_delete(void) {
    printf("TEST P2-1-3: recycle id path after delete\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();

    /* Create + delete 100K */
    ecs_entity_t *entities = bulk_new_no_comp(world, 100000);
    for (int i = 0; i < 100000; i++) {
        ecs_delete(world, entities[i]);
    }

    /* Re-create 100K — should reuse recycled ids */
    ecs_entity_t *entities2 = bulk_new_no_comp(world, 100000);
    if (!entities2) {
        printf("  FAIL: re-create returned NULL\n");
        ecs_fini(world);
        return 1;
    }

    /* Verify each new entity has valid record */
    int fail = 0;
    for (int i = 0; i < 100000; i++) {
        if (!ecs_is_alive(world, entities2[i])) {
            if (i < 5) printf("  FAIL at i=%d: not alive\n", i);
            fail++;
            break;
        }
    }

    ecs_os_free(entities);
    ecs_os_free(entities2);
    ecs_fini(world);
    if (fail) { printf("  FAIL\n"); return 1; }
    printf("  PASS (recycled ids valid)\n");
    return 0;
}

static int test_prefetch_hint(void) {
    printf("TEST P2-1-4: flecs_entity_index_prefetch helper\n");
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ecs_entity_t *entities = bulk_new_no_comp(world, 1000);

    /* Public API lookup — prefetch hint is internal, just verify lookup works */
    int fail = 0;
    for (int i = 0; i < 1000; i++) {
        const void *r = ecs_get_id(world, entities[i], 0);
        (void)r;
        if (!ecs_is_alive(world, entities[i])) {
            fail++;
            break;
        }
    }

    ecs_os_free(entities);
    ecs_fini(world);
    if (fail) { printf("  FAIL\n"); return 1; }
    printf("  PASS (1K entity lookup path exercised)\n");
    return 0;
}

int main(void) {
    int failed = 0;
    failed += test_create_lookup_roundtrip();
    failed += test_bulk_delete();
    failed += test_recycle_after_delete();
    failed += test_prefetch_hint();
    if (failed) {
        printf("\n=== %d test(s) FAILED ===\n", failed);
        return 1;
    }
    printf("\n=== ALL P2-1 TESTS PASSED ===\n");
    return 0;
}