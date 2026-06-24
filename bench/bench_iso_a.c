/* İzole test: sadece senaryo A — iter_throughput */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdlib.h>
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
#endif

typedef struct { float x, y, z; } Pos;
typedef struct { float x, y, z; } Vel;

int main(int argc, char **argv) {
    int n_iters = argc > 1 ? atoi(argv[1]) : 5;
    printf("=== iso [A] n=%d ===\n", n_iters);
    fflush(stdout);

    printf("[step 1] before ecs_init\n"); fflush(stdout);
    ecs_world_t *world = ecs_init();
    printf("[step 2] after ecs_init\n"); fflush(stdout);

    printf("[step 3] before Pos COMPONENT\n"); fflush(stdout);
    ECS_COMPONENT(world, Pos);
    printf("[step 4] after Pos\n"); fflush(stdout);
    ECS_COMPONENT(world, Vel);
    printf("[step 5] after Vel\n"); fflush(stdout);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    printf("[step 6] before ecs_new loop\n"); fflush(stdout);
    for (int i = 0; i < N; i++) es[i] = ecs_new(world);
    printf("[step 7] after ecs_new loop\n"); fflush(stdout);

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Pos), .inout = EcsIn },
            { .id = ecs_id(Vel), .inout = EcsIn },
        }
    });
    printf("[step 8] after ecs_query\n"); fflush(stdout);

    printf("[step 9] before ecs_set loop\n"); fflush(stdout);
    for (int i = 0; i < N; i++) {
        ecs_set(world, es[i], Pos, {(float)i, (float)i, (float)i});
        ecs_set(world, es[i], Vel, {(float)i, (float)i, (float)i});
    }
    printf("[step 10] after ecs_set loop\n"); fflush(stdout);

    double t0 = now_sec();
    long long total = 0;
    printf("[step 11] before iter loop\n"); fflush(stdout);
    for (int it = 0; it < n_iters; it++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Pos *p = ecs_field(&it, Pos, 0);
            Vel *v = ecs_field(&it, Vel, 1);
            for (int i = 0; i < it.count; i++) {
                p[i].x += v[i].x * 0.016f;
                total++;
            }
        }
    }
    double dt = now_sec() - t0;
    printf("[A] total=%lld, dt=%.3f, %.1f M ent/sec\n",
        total, dt, (total/dt)/1e6);

    ecs_query_fini(q);
    ecs_os_free(es);
    ecs_fini(world);
    printf("=== iso [A] DONE ===\n");
    return 0;
}
