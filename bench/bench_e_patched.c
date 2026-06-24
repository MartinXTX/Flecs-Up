/* Just scenario E, patched */
#include "flecs_patched.h"
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
#endif

typedef struct { int v; } EA;
typedef struct { int v; } EB;
typedef struct { int v; } EC;
typedef struct { int v; } ED;

int main(int argc, char **argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 200;
    ecs_world_t *world = ecs_init();
    fprintf(stderr, "[DBG] world init done\n");
    ECS_COMPONENT(world, EA);
    fprintf(stderr, "[DBG] EA registered\n");
    ECS_COMPONENT(world, EB);
    fprintf(stderr, "[DBG] EB registered\n");
    ECS_COMPONENT(world, EC);
    fprintf(stderr, "[DBG] EC registered\n");
    ECS_COMPONENT(world, ED);
    fprintf(stderr, "[DBG] ED registered\n");

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        if (i & 1) ecs_add(world, es[i], EA);
        if (i & 2) ecs_add(world, es[i], EB);
        if (i & 4) ecs_add(world, es[i], EC);
        if (i & 8) ecs_add(world, es[i], ED);
        if (i < 5 || i == N-1) fprintf(stderr, "[DBG] entity %d created\n", i);
    }
    fprintf(stderr, "[DBG] all entities created\n");

    ecs_query_t *q_ea = ecs_query(world, { .terms = {{ .id = ecs_id(EA) }} });
    fprintf(stderr, "[DBG] query EA created\n");
    ecs_query_t *q_eb = ecs_query(world, { .terms = {{ .id = ecs_id(EB) }} });
    fprintf(stderr, "[DBG] query EB created\n");

    double t = 0;
    long long hits = 0;
    for (int it = 0; it < n; it++) {
        ecs_iter_t itea = ecs_query_iter(world, q_ea);
        ecs_iter_t iteb = ecs_query_iter(world, q_eb);
        double t0 = now_sec();
        while (ecs_query_next(&itea)) {
            EA *p = ecs_field(&itea, EA, 0); (void)p;
            hits += itea.count;
        }
        while (ecs_query_next(&iteb)) {
            EB *p = ecs_field(&iteb, EB, 0); (void)p;
            hits += iteb.count;
        }
        t += now_sec() - t0;
    }
    printf("[E-only-patched] many_archetypes: %d varlik, 16 archetype | "
           "iter (2 sorgu x %d): %.3f sec | %.1f M ent/sec iter\n",
           N, n, t, ((double)hits / t) / 1e6);

    ecs_query_fini(q_ea);
    ecs_query_fini(q_eb);
    ecs_os_free(es);
    ecs_fini(world);
    return 0;
}
