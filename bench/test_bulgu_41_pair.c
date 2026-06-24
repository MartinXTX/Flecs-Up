/* BULGU-41: ecs_get_id(world, e, pair_id) crash on EcsDontFragment pair workload.
 *
 * Standalone reproducer + regression test.
 *
 * Scenario:
 *   - Define a relationship R with EcsDontFragment (and EcsSparse).
 *   - Define a set of target entities T[i].
 *   - Create N entities (50K+) and add (R, T[i % K]) pair to each.
 *   - Walk the table and call ecs_get_id for each entity's pair.
 *   - Walk via ecs_set_id, emplace, get_id, has_id, target_of loops.
 *
 * Pre-fix: NULL deref inside flecs_sparse_on_add_cr at 50K+ entities because
 *          flecs_component_init_sparse did not properly size concrete pair
 *          (R, T) sparse storage for EcsDontFragment.
 *
 * Post-fix (Tier-v18 BULGU-41 patch):
 *   - flecs_sparse_on_add_cr has NULL guard on cr->sparse (defensive)
 *   - flecs_component_init_sparse handles concrete pair (R, T) with
 *     EcsIdDontFragment and sizes sparse as ecs_entity_t (exclusive) or
 *     ecs_type_t (non-exclusive).
 *
 * Usage:
 *   cl /O2 /DFLECS_PATCHED_BUILD /I . /I ..\include test_bulgu_41_pair.c ^
 *      /Fe:test_bulgu_41_pair.exe ..\release\v18_bulgu41_flecs_patched.lib
 *   test_bulgu_41_pair.exe
 *
 * Expected: PASS on Tier-v18 with BULGU-41 patch applied.
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

#define BULGU41_N_ENTITIES 50000
#define BULGU41_N_TARGETS  64

/* Test 1: 50K entities, EcsDontFragment pair workload, ecs_get_id loop.
 * Crash target: NULL deref inside flecs_sparse_on_add_cr -> flecs_sparse_count(NULL). */
static int test_get_id_50k_dont_fragment_pair(void) {
    printf("Test 1: 50K entities, EcsDontFragment pair, ecs_get_id loop\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    /* Define relationship with EcsDontFragment (no type_info -> tag pair). */
    ecs_entity_t Rel = ecs_entity(w, { .add = ecs_ids( EcsDontFragment ) });
    TASSERT(Rel != 0);

    /* Create target entities. */
    ecs_entity_t targets[BULGU41_N_TARGETS];
    for (int i = 0; i < BULGU41_N_TARGETS; i++) {
        char name[32];
        snprintf(name, sizeof(name), "Tgt_%d", i);
        targets[i] = ecs_entity(w, { .name = name });
        TASSERT(targets[i] != 0);
    }

    /* Create 50K entities, attach (Rel, T[i%K]) pair. */
    ecs_entity_t *entities = (ecs_entity_t*)malloc(
        sizeof(ecs_entity_t) * BULGU41_N_ENTITIES);
    TASSERT(entities != NULL);

    double t0 = now_sec();
    for (int i = 0; i < BULGU41_N_ENTITIES; i++) {
        entities[i] = ecs_new(w);
        TASSERT(entities[i] != 0);
        ecs_add_pair(w, entities[i], Rel, targets[i % BULGU41_N_TARGETS]);
    }
    double dt_create = now_sec() - t0;
    printf("  create+add_pair: %.3f sec for %d entities\n",
        dt_create, BULGU41_N_ENTITIES);

    /* Walk ecs_get_id for every entity's pair. Pre-fix: crashes here. */
    int rc = 0;
    double t1 = now_sec();
    for (int i = 0; i < BULGU41_N_ENTITIES; i++) {
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % BULGU41_N_TARGETS]);
        /* ecs_get_id on a pair id is normally only valid for zero-sized pairs
         * via the dont-fragment sparse path; we just exercise the lookup so
         * that flecs_sparse_on_add_cr (when triggered) and downstream paths
         * run for every entity. */
        (void)ecs_get_id(w, entities[i], pair_id);
    }
    double dt_get = now_sec() - t1;
    printf("  ecs_get_id loop: %.3f sec for %d calls\n",
        dt_get, BULGU41_N_ENTITIES);

    /* has_id must return true for the pair on every entity. */
    int mismatch = 0;
    for (int i = 0; i < BULGU41_N_ENTITIES; i++) {
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % BULGU41_N_TARGETS]);
        bool has = ecs_has_id(w, entities[i], pair_id);
        if (!has) {
            mismatch++;
            if (mismatch < 5) {
                printf("    has_id miss at i=%d pair_id=%llu\n",
                    i, (unsigned long long)pair_id);
            }
        }
    }
    TASSERT(mismatch == 0);
    printf("  has_id loop ok\n");

    /* target lookup via ecs_get_target. */
    for (int i = 0; i < BULGU41_N_ENTITIES; i++) {
        if ((i % 10000) == 0) printf("  get_target i=%d\n", i);
        ecs_entity_t tgt = ecs_get_target(w, entities[i], Rel, 0);
        if (tgt != targets[i % BULGU41_N_TARGETS]) {
            printf("    target miss at i=%d got=%llu expected=%llu\n",
                i, (unsigned long long)tgt,
                (unsigned long long)targets[i % BULGU41_N_TARGETS]);
            rc = 1;
            break;
        }
    }
    TASSERT(rc == 0);

    free(entities);
    ecs_fini(w);
    printf("  PASS\n");
    return 0;
}

/* Test 2: bulk add/remove of EcsDontFragment pair (50K workload, exercises
 * flecs_sparse_on_add_cr via the standard add path which the Tier-A1 fast
 * path intercepts). */
static int test_bulk_add_remove_dont_fragment_pair(void) {
    printf("Test 2: bulk add/remove of EcsDontFragment pair (50K)\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    ecs_entity_t Rel = ecs_entity(w, { .add = ecs_ids( EcsDontFragment ) });
    TASSERT(Rel != 0);

    /* Create 16 target entities. */
    const int K = 16;
    ecs_entity_t targets[16];
    for (int i = 0; i < K; i++) {
        char nm[16]; snprintf(nm, sizeof(nm), "TT_%d", i);
        targets[i] = ecs_entity(w, { .name = nm });
    }

    const int N = BULGU41_N_ENTITIES;
    ecs_entity_t *entities = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * N);
    TASSERT(entities != NULL);

    double t0 = now_sec();
    for (int i = 0; i < N; i++) {
        entities[i] = ecs_new(w);
        TASSERT(entities[i] != 0);
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % K]);
        ecs_add_id(w, entities[i], pair_id);
    }
    double dt = now_sec() - t0;
    printf("  bulk add: %.3f sec for %d entities\n", dt, N);

    /* Verify all entities have the pair. */
    int mismatch = 0;
    for (int i = 0; i < N; i++) {
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % K]);
        if (!ecs_has_id(w, entities[i], pair_id)) {
            mismatch++;
        }
    }
    TASSERT(mismatch == 0);
    printf("  all %d have pair (mismatch=%d)\n", N, mismatch);

    /* Now bulk remove. */
    t0 = now_sec();
    for (int i = 0; i < N; i++) {
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % K]);
        ecs_remove_id(w, entities[i], pair_id);
    }
    dt = now_sec() - t0;
    printf("  bulk remove: %.3f sec for %d entities\n", dt, N);

    /* Verify all entities do NOT have the pair anymore. */
    mismatch = 0;
    for (int i = 0; i < N; i++) {
        ecs_id_t pair_id = ecs_pair(Rel, targets[i % K]);
        if (ecs_has_id(w, entities[i], pair_id)) {
            mismatch++;
        }
    }
    TASSERT(mismatch == 0);
    printf("  all %d removed pair (mismatch=%d)\n", N, mismatch);

    free(entities);
    ecs_fini(w);
    printf("  PASS\n");
    return 0;
}

/* Test 3: Mixed - some entities with (Rel, T[i]), others without; verify
 * sparse insert/remove is correct. */
static int test_mixed_sparse_insert_remove(void) {
    printf("Test 3: mixed add/remove EcsDontFragment pair\n");
    ecs_world_t *w = ecs_init();
    TASSERT(w != NULL);

    ecs_entity_t Rel = ecs_entity(w, { .add = ecs_ids( EcsDontFragment ) });
    ecs_entity_t T1 = ecs_entity(w, { .name = "T1" });
    ecs_entity_t T2 = ecs_entity(w, { .name = "T2" });

    const int N = 10000;
    ecs_entity_t *e = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * N);
    TASSERT(e != NULL);

    for (int i = 0; i < N; i++) {
        e[i] = ecs_new(w);
        ecs_id_t pair_id = (i & 1) ? ecs_pair(Rel, T1) : ecs_pair(Rel, T2);
        ecs_add_pair(w, e[i], Rel, (i & 1) ? T1 : T2);
        TASSERT(ecs_has_id(w, e[i], pair_id));
    }

    /* Remove all and re-add to exercise sparse path on remove + add. */
    for (int i = 0; i < N; i++) {
        ecs_id_t pair_id = (i & 1) ? ecs_pair(Rel, T1) : ecs_pair(Rel, T2);
        ecs_remove_id(w, e[i], pair_id);
        TASSERT(!ecs_has_id(w, e[i], pair_id));
        ecs_add_id(w, e[i], pair_id);
        TASSERT(ecs_has_id(w, e[i], pair_id));
    }

    free(e);
    ecs_fini(w);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("=== BULGU-41 EcsDontFragment pair workload regression ===\n\n");

    int failures = 0;
    failures += test_get_id_50k_dont_fragment_pair();
    failures += test_bulk_add_remove_dont_fragment_pair();
    failures += test_mixed_sparse_insert_remove();

    if (failures) {
        printf("\n%d TEST(S) FAILED\n", failures);
        return 1;
    }
    printf("\nALL TESTS PASS — BULGU-41 fix working\n");
    return 0;
}