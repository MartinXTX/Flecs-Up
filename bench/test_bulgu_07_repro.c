/* BULGU-07: Cascade hang with 100+ unique parents
 *
 * Standalone reproducer for upstream Flecs issue.
 * This test demonstrates the cascade delete hang that occurs when
 * deleting entities with many unique parents.
 *
 * Usage:
 *   cl /O2 /DFLECS_PATCHED_BUILD /I . /I ..\include test_bulgu_07_repro.c ^
 *      /Fe:test_bulgu_07_repro.exe ..\release\flecs_patched.lib
 *   test_bulgu_07_repro.exe
 *
 * Expected: PASS on Flecs Patched v14.1 (Tier-DQ1/EV1 mitigations)
 * Expected: HANG on upstream master (revert patches first to test)
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_sec(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#define TASSERT(c) do { if (!(c)) { printf("  FAIL: %s:%d assertion failed\n", __FILE__, __LINE__); return 1; } } while(0)

/* Test 1: 100 unique parents, each with 1 child, cascade delete */
static int test_cascade_100_unique_parents(void) {
    printf("Test 1: 100 unique parents, cascade delete\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    const int N = 100;
    ecs_entity_t parents[100];
    for (int i = 0; i < N; i++) {
        parents[i] = ecs_new(w);
    }
    for (int i = 0; i < N; i++) {
        ecs_new_w_pair(w, EcsChildOf, parents[i]);
    }

    double t0 = now_sec();
    for (int i = 0; i < N; i++) {
        ecs_delete(w, parents[i]);
    }
    double dt = now_sec() - t0;
    printf("  PASS: %.3f sec for 100 cascade deletes\n", dt);
    ecs_fini(w);
    return 0;
}

/* Test 2: 500 unique parents with 5 children each */
static int test_cascade_500_parents_5_children(void) {
    printf("Test 2: 500 unique parents, 5 children each\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    const int N_PARENTS = 500;
    const int N_CHILDREN = 5;
    ecs_entity_t *parents = malloc(N_PARENTS * sizeof(ecs_entity_t));
    for (int i = 0; i < N_PARENTS; i++) {
        parents[i] = ecs_new(w);
        for (int c = 0; c < N_CHILDREN; c++) {
            ecs_new_w_pair(w, EcsChildOf, parents[i]);
        }
    }
    double t0 = now_sec();
    for (int i = 0; i < N_PARENTS; i++) {
        ecs_delete(w, parents[i]);
    }
    double dt = now_sec() - t0;
    printf("  PASS: %.3f sec for 500*5 cascade deletes\n", dt);
    ecs_fini(w);
    free(parents);
    return 0;
}

/* Test 3: 1000 unique parents — stress */
static int test_cascade_1000_parents(void) {
    printf("Test 3: 1000 unique parents stress\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    const int N = 1000;
    double t0 = now_sec();
    for (int i = 0; i < N; i++) {
        ecs_entity_t p = ecs_new(w);
        ecs_new_w_pair(w, EcsChildOf, p);
        ecs_delete(w, p);
    }
    double dt = now_sec() - t0;
    printf("  PASS: %.3f sec for 1000 create+child+delete cycles\n", dt);
    ecs_fini(w);
    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("=== BULGU-07 Cascade Hang Reproducer ===\n\n");

    int failures = 0;
    failures += test_cascade_100_unique_parents();
    failures += test_cascade_500_parents_5_children();
    failures += test_cascade_1000_parents();

    if (failures) {
        printf("\n%d TEST(S) FAILED\n", failures);
        return 1;
    }
    printf("\nALL TESTS PASS — BULGU-07 mitigations working\n");
    return 0;
}
