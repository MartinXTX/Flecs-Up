/* Tier-v17 P2-3 dispatcher benchmark
 *
 * Measures observer fanout cost: many observers subscribed to the same
 * event/component, then emit the event and time the dispatch.
 *
 * Compares Tier-v16 (live-map iteration) vs Tier-v17 (snapshot iteration).
 *
 * Build:
 *   cl /O2 /W3 /DFLECS_PATCHED_BUILD /I . /I ..\include bench_dispatch.c \
 *      <flecs_vXX.obj> /Fe:bench_dispatch.exe
 */

#define FLECS_NO_OS_API
#ifdef FLECS_PATCHED_BUILD
#include "flecs_patched.h"
#else
#include "../distr/flecs_no_addons.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
static double now_sec(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER t;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}
#else
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

typedef struct { int value; } Tag;

static int counter = 0;
static void obs_callback(ecs_iter_t *it) {
    counter += it->count;
}

static double run_fanout(int n_observers, int n_entities, int n_iters) {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, Tag);

    /* Register N observers on Tag */
    for (int i = 0; i < n_observers; i++) {
        ecs_observer_init(world, &(ecs_observer_desc_t){
            .query.terms = {{ .id = ecs_id(Tag), .src.id = EcsSelf }},
            .events = { EcsOnAdd },
            .callback = obs_callback
        });
    }

    /* Create entities (one emit each, but they trigger all N observers) */
    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
        ecs_entity_t e = ecs_new(world);
        ecs_add_id(world, e, ecs_id(Tag));
    }
    double t1 = now_sec();

    /* Compute emits/sec */
    double elapsed = t1 - t0;
    double emits_per_sec = (double)n_iters / elapsed;
    double observer_invokes = emits_per_sec * (double)n_observers;
    ecs_fini(world);

    /* Suppress unused */
    (void)n_entities; (void)counter;

    return observer_invokes;
}

int main(int argc, char **argv) {
    int n_iters = 10000;
    if (argc > 1) n_iters = atoi(argv[1]);

    printf("=== Tier-v17 P2-3 dispatcher fanout benchmark ===\n");
    printf("N_iters = %d\n", n_iters);
    printf("\n");

    /* Warmup */
    run_fanout(8, 0, 100);

    /* Sweep observer count to find the breakpoint between small/large path */
    int observer_counts[] = { 1, 4, 16, 64, 128, 256, 1024 };
    int n_obs_sizes = sizeof(observer_counts) / sizeof(observer_counts[0]);

    printf("%-12s | %-15s\n", "n_observers", "obs_invokes/sec");
    printf("-------------|----------------\n");

    for (int i = 0; i < n_obs_sizes; i++) {
        int n_obs = observer_counts[i];
        double inv_per_sec = run_fanout(n_obs, 0, n_iters);
        printf("%-12d | %-15.0f\n", n_obs, inv_per_sec);
    }

    printf("\n=== done ===\n");
    return 0;
}