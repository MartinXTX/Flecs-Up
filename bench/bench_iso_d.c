/* İzole test: sadece senaryo D — large_world */
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

typedef struct { int x; } DA; typedef struct { int x; } DB;
typedef struct { int x; } DC; typedef struct { int x; } DD;
typedef struct { int x; } DE; typedef struct { int x; } DF;
typedef struct { int x; } DG; typedef struct { int x; } DH;

int main(int argc, char **argv) {
    int n_iters = argc > 1 ? atoi(argv[1]) : 100;
    printf("=== iso [D] n=%d ===\n", n_iters);
    fflush(stdout);

    ecs_world_t *world = ecs_init();
    ECS_COMPONENT(world, DA); ECS_COMPONENT(world, DB);
    ECS_COMPONENT(world, DC); ECS_COMPONENT(world, DD);
    ECS_COMPONENT(world, DE); ECS_COMPONENT(world, DF);
    ECS_COMPONENT(world, DG); ECS_COMPONENT(world, DH);
    printf("components OK\n"); fflush(stdout);

    const int N = 1000000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);

    /* Varlıkları 8 archetype'e dağıtarak oluştur */
    double t0 = now_sec();
    for (int i = 0; i < N; i++) {
        ecs_entity_t e = ecs_new(world);
        es[i] = e;
        switch (i & 7) {
            case 0: ecs_add(world, e, DA); break;
            case 1: ecs_add(world, e, DB); ecs_add(world, e, DC); break;
            case 2: ecs_add(world, e, DD); break;
            case 3: ecs_add(world, e, DE); ecs_add(world, e, DF); break;
            case 4: ecs_add(world, e, DG); break;
            case 5: ecs_add(world, e, DA); ecs_add(world, e, DB); break;
            case 6: ecs_add(world, e, DC); ecs_add(world, e, DD); break;
            case 7: ecs_add(world, e, DH); ecs_add(world, e, DA); break;
        }
    }
    double t_create = now_sec() - t0;
    printf("create: %.3f sec (%.0f ent/sec)\n", t_create, N / t_create);
    fflush(stdout);

    /* DA sorgusu — 8 archetype'in 4'ünde eşleşir */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(DA) }}
    });

    double t_iter = 0;
    long long hits = 0;
    for (int it = 0; it < n_iters; it++) {
        t0 = now_sec();
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            DA *p = ecs_field(&it, DA, 0);
            (void)p;
            hits += it.count;
        }
        t_iter += now_sec() - t0;
    }

    printf("[D] large_world: %d varlik (8 archetype) | "
           "iter: %.3f sec avg | %.1f M ent/sec iter throughput\n",
           N, t_iter / n_iters, ((double)hits / t_iter) / 1e6);

    ecs_query_fini(q);
    ecs_os_free(es);
    ecs_fini(world);
    printf("=== iso [D] DONE ===\n");
    return 0;
}
