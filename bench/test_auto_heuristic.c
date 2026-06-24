/* Tier-A1.3: Auto-heuristic EcsDontFragment verification (deferred v16).
 * The auto-heuristic was attempted in Tier v15 with patches to
 * flecs_ensure and flecs_component_init_sparse, but a deeper architectural
 * conflict was discovered (the existing EcsDontFragment init path
 * conflicts with the auto-heuristic). The flag API remains for future
 * Tier v16 work. This test verifies the flag API works without breaking
 * ecs_set/get.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef struct { int x; } AHData;
typedef struct { int y; } AHData2;

ECS_COMPONENT_DECLARE(AHData);
ECS_COMPONENT_DECLARE(AHData2);

static int g_observer_count = 0;

static void ah_observer(ecs_iter_t *it) {
    g_observer_count += it->count;
}

static int test_disabled_by_default(void) {
    printf("TEST: disabled_by_default\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, AHData);

    ecs_entity_t e = ecs_new(world);
    AHData v = {42};
    ecs_set(world, e, AHData, {42});
    const AHData *p = ecs_get(world, e, AHData);
    if (!p || p->x != 42) {
        printf("  FAIL\n");
        ecs_fini(world);
        return 1;
    }
    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_flag_is_noop(void) {
    printf("TEST: flag_is_noop (Tier-A1.3 deferred to v16)\n");
    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    /* Setting the flag must NOT cause ecs_set to hang. With auto-heuristic
     * code commented out (deferred v16), the flag is a no-op. Verify
     * ecs_set/get still work correctly. */
    ECS_COMPONENT_DEFINE(world, AHData2);

    ecs_entity_t e = ecs_new(world);
    AHData2 v = {99};
    ecs_set(world, e, AHData2, {99});

    const AHData2 *p = ecs_get(world, e, AHData2);
    if (!p || p->y != 99) {
        printf("  FAIL: get returned wrong value\n");
        ecs_fini(world);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS (flag no-op, ecs_set/get work correctly)\n");
    return 0;
}

static int test_observer_still_works(void) {
    printf("TEST: observer_still_works\n");
    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);

    ECS_COMPONENT_DEFINE(world, AHData);

    g_observer_count = 0;
    ecs_observer(world, {
        .query.terms = {{ .id = ecs_id(AHData) }},
        .events = { EcsOnAdd },
        .callback = ah_observer
    });

    ecs_entity_t e = ecs_new(world);
    ecs_add(world, e, AHData);

    if (g_observer_count != 1) {
        printf("  FAIL: observer count=%d expected 1\n", g_observer_count);
        ecs_fini(world);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_disable_flag(void) {
    printf("TEST: disable_flag_roundtrip\n");
    ecs_world_t *world = ecs_init();
    ecs_world_auto_dont_fragment(world, true);
    ecs_world_auto_dont_fragment(world, false);

    ECS_COMPONENT_DEFINE(world, AHData2);
    ecs_entity_t e = ecs_new(world);
    AHData2 v = {7};
    ecs_set(world, e, AHData2, {7});
    const AHData2 *p = ecs_get(world, e, AHData2);
    if (!p || p->y != 7) {
        printf("  FAIL\n");
        ecs_fini(world);
        return 1;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int failures = 0;
    failures += test_disabled_by_default();
    failures += test_flag_is_noop();
    failures += test_observer_still_works();
    failures += test_disable_flag();
    if (failures) printf("\n%d TESTS FAILED\n", failures);
    else printf("\nALL AUTO-HEURISTIC TESTS PASSED (Tier-A1.3 deferred v16)\n");
    return failures;
}
