/* Tier-A1 sparse storage test:
 * Compare archetype_churn performance with/without EcsDontFragment flag.
 * If Tier-A1 works: sparse storage skips archetype migration entirely.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static double now_sec(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}
#endif

/* Data-only components (no observers, no hooks) */
typedef struct { float v; } T01;
typedef struct { float v; } T02;
typedef struct { float v; } T03;
typedef struct { float v; } T04;
typedef struct { float v; } T05;
typedef struct { float v; } T06;
typedef struct { float v; } T07;
typedef struct { float v; } T08;
typedef struct { float v; } T09;
typedef struct { float v; } T10;

int main(int argc, char **argv) {
    int n_iters = argc > 1 ? atoi(argv[1]) : 1000;
    int use_dont_fragment = argc > 2 ? atoi(argv[2]) : 0;
    printf("=== TIER-A1 sparse test: n=%d, dont_fragment=%d ===\n",
        n_iters, use_dont_fragment);
    fflush(stdout);

    ecs_world_t *world = ecs_init();

    /* Declare components — same as ECS_COMPONENT but with optional
     * EcsDontFragment flag. */
    ecs_entity_t comps[10];
    if (use_dont_fragment) {
        ecs_entity_t ids[] = {
            ecs_entity(world, { .name = "T01", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T02", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T03", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T04", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T05", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T06", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T07", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T08", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T09", .add = ecs_ids( EcsDontFragment ) }),
            ecs_entity(world, { .name = "T10", .add = ecs_ids( EcsDontFragment ) }),
        };
        memcpy(comps, ids, sizeof(ids));
    } else {
        comps[0] = ecs_entity(world, { .name = "T01" });
        comps[1] = ecs_entity(world, { .name = "T02" });
        comps[2] = ecs_entity(world, { .name = "T03" });
        comps[3] = ecs_entity(world, { .name = "T04" });
        comps[4] = ecs_entity(world, { .name = "T05" });
        comps[5] = ecs_entity(world, { .name = "T06" });
        comps[6] = ecs_entity(world, { .name = "T07" });
        comps[7] = ecs_entity(world, { .name = "T08" });
        comps[8] = ecs_entity(world, { .name = "T09" });
        comps[9] = ecs_entity(world, { .name = "T10" });
    }

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) es[i] = ecs_new(world);

    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
        ecs_entity_t e = es[it % N];
        for (int k = 0; k < 10; k++) ecs_add_id(world, e, comps[k]);
        for (int k = 0; k < 10; k++) ecs_remove_id(world, e, comps[k]);
    }
    double dt = now_sec() - t0;
    double ops = (double)n_iters * 20.0;
    printf("churn: %d iters x 20 ops | %.3f sec | %.2f M ops/sec | %.0f ns/op\n",
        n_iters, dt, ops / dt / 1e6, dt * 1e9 / ops);

    ecs_os_free(es);
    ecs_fini(world);
    return 0;
}
