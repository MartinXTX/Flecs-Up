/* Tier v15 baseline: Sparse iterator micro-benchmark
 *
 * Measures ecs_query + EcsSparse component performance.
 * Pre/post Tier-A1.4 (iterator sparse path) comparison.
 */
#include "flecs_patched.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { int32_t v; } SP;

static double now_sec(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int n_iters = (argc > 1) ? atoi(argv[1]) : 100;
    int n_ents = (argc > 2) ? atoi(argv[2]) : 100000;

    printf("=== Sparse Iterator Baseline ===\n");
    printf("n_iters=%d, n_ents=%d\n\n", n_iters, n_ents);

    ecs_world_t *w = ecs_init();
    ECS_COMPONENT(w, SP);
    ecs_add_id(w, ecs_id(SP), EcsSparse);

    /* Setup: create N entities with SP */
    for (int i = 0; i < n_ents; i++) {
        ecs_entity_t e = ecs_new(w);
        SP val = { .v = i };
        ecs_set_id(w, e, ecs_id(SP), sizeof(SP), &val);
    }

    /* Baseline: per-entity get */
    {
        ecs_entity_t *es = malloc(n_ents * sizeof(ecs_entity_t));
        ecs_entities_t ents = ecs_get_entities(w);
        for (int i = 0; i < n_ents && i < ents.count; i++) es[i] = ents.ids[i];

        double t0 = now_sec();
        long long checksum = 0;
        long long total = 0;
        for (int it = 0; it < n_iters; it++) {
            for (int i = 0; i < n_ents; i++) {
                const SP *p = ecs_get(w, es[i], SP);
                if (p) checksum += p->v;
            }
            total += n_ents;
        }
        double dt = now_sec() - t0;
        printf("[Baseline] per-entity get: %.3f sec | %.2f M ops/sec (cs=%lld)\n",
               dt, (double)total / dt / 1e6, checksum);
        free(es);
    }

    /* Iter query (sparse) — likely slow path */
    {
        ecs_query_t *q = ecs_query(w, { .terms = { { .id = ecs_id(SP) } } });
        double t0 = now_sec();
        long long total = 0;
        for (int it = 0; it < n_iters; it++) {
            ecs_iter_t it2 = ecs_query_iter(w, q);
            while (ecs_query_next(&it2)) {
                total += it2.count;
            }
        }
        double dt = now_sec() - t0;
        printf("[Query iter]  %.3f sec | %.2f M iter/sec\n",
               dt, (double)(n_iters * n_ents) / dt / 1e6);
    }

    ecs_fini(w);
    return 0;
}
