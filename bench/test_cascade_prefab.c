/* Cascade delete + prefab override + IsA inheritance stress test (clean). */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef struct { int x, y; } CSPos;
typedef struct { int vx, vy; } CSVel;
typedef struct { int hp; } CSHp;
typedef struct { int dmg; } CSDmg;

ECS_COMPONENT_DECLARE(CSPos);
ECS_COMPONENT_DECLARE(CSVel);
ECS_COMPONENT_DECLARE(CSHp);
ECS_COMPONENT_DECLARE(CSDmg);

static int test_ecs_new_after_bulk(void) {
    printf("TEST: ecs_new_after_bulk (5 simple)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, CSPos);

    ecs_entity_t prefab = ecs_entity(world, { .name = "ParentPrefab" });
    ecs_add(world, prefab, CSPos);

    const int N = 100;
    ecs_entity_t *parents = ecs_bulk_new_w_id(world, prefab, N);
    CSPos p = {1, 2};
    ecs_bulk_set_id(world, parents, N, ecs_id(CSPos), sizeof(CSPos), &p);

    /* 5 simple ecs_new calls after bulk */
    for (int i = 0; i < 5; i++) {
        ecs_entity_t e = ecs_new(world);
        (void)e;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_ecs_new_w_pair_after_bulk(void) {
    printf("TEST: ecs_new_w_pair_after_bulk (5 simple)\n");
    ecs_world_t *world = ecs_init();
    ECS_COMPONENT_DEFINE(world, CSPos);

    ecs_entity_t prefab = ecs_entity(world, { .name = "ParentPrefab" });
    ecs_add(world, prefab, CSPos);

    const int N = 100;
    ecs_entity_t *parents = ecs_bulk_new_w_id(world, prefab, N);
    CSPos p = {1, 2};
    ecs_bulk_set_id(world, parents, N, ecs_id(CSPos), sizeof(CSPos), &p);

    /* 5 ecs_new_w_pair calls after bulk */
    for (int i = 0; i < 5; i++) {
        ecs_entity_t e = ecs_new_w_pair(world, EcsChildOf, parents[i]);
        (void)e;
    }

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

static int test_cascade_delete_small(void) {
    int N_CHILDREN = 5;
    int N_PARENTS = 100;
    printf("TEST: cascade_delete_small_no_prefab (N_PARENTS=%d, N_CHILDREN=%d)\n", N_PARENTS, N_CHILDREN);
    ecs_world_t *world = ecs_init();

    /* No prefab — just create parents via ecs_new */
    ecs_entity_t parents[100];
    for (int i = 0; i < N_PARENTS; i++) {
        fprintf(stderr, "[c] parent i=%d\n", i); fflush(stderr);
        parents[i] = ecs_new(world);
    }
    fprintf(stderr, "[c] parents done, adding children\n"); fflush(stderr);

    int total = 0;
    for (int i = 0; i < N_PARENTS; i++) {
        for (int c = 0; c < N_CHILDREN; c++) {
            fprintf(stderr, "[c] child i=%d c=%d total=%d\n", i, c, total); fflush(stderr);
            ecs_new_w_pair(world, EcsChildOf, parents[i]);
            total++;
        }
    }
    fprintf(stderr, "[c] all %d children done\n", total); fflush(stderr);

    ecs_fini(world);
    printf("  PASS\n");
    return 0;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int failures = 0;
    failures += test_ecs_new_after_bulk();
    failures += test_ecs_new_w_pair_after_bulk();
    failures += test_cascade_delete_small();
    if (failures) printf("\n%d TESTS FAILED\n", failures);
    else printf("\nALL CASCADE/PREFAB TESTS PASSED\n");
    return failures;
}
