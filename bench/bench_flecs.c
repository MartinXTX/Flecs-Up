/* Flecs performans benchmark'i
 * - 4 senaryo: iterasyon throughput, archetype churn, entity lifecycle, büyük dünya kurulumu
 * - Windows MSVC uyumlu, distr/flecs_no_addons.c statik kütüphane olarak derlenir
 *
 * Derleme:
 *   cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /I distr bench/bench_flecs.c distr/flecs_no_addons.c /Fe:bench_flecs.exe
 *
 * Calistirma:
 *   bench_flecs.exe [iterasyon_sayisi]
 */

#define FLECS_NO_OS_API
/* When building with patched build, use patched header for Tier-A2 macros. */
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

#define N_DEFAULT 10
#define N_WARMUP 50  /* TIER-N1: warmup itersyonları (cold cache/L1 doldurma) */

/* -------------------------------------------------------------------------- */
/* Senaryo A: Iterasyon throughput — cache'li trivial sorgu                   */
/* -------------------------------------------------------------------------- */
typedef struct { float x, y, z; } Pos;
typedef struct { float x, y, z; } Vel;

static int scenario_iter_throughput(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, Pos);
    ECS_COMPONENT(world, Vel);

    /* 100k varlık yarat, Pos+Vel ile */
    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
    }

    ecs_query_t *q = ecs_query(world, {
        .terms = {
            { .id = ecs_id(Pos), .inout = EcsIn },
            { .id = ecs_id(Vel), .inout = EcsIn },
        }
    });

    /* Toplu Pos+Vel ata */
    for (int i = 0; i < N; i++) {
        ecs_set(world, es[i], Pos, {(float)i, (float)i, (float)i});
        ecs_set(world, es[i], Vel, {(float)i, (float)i, (float)i});
    }

    /* Warmup */
    {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            Pos *p = ecs_field(&it, Pos, 0);
            Vel *v = ecs_field(&it, Vel, 1);
            (void)p; (void)v;
        }
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            /* TIER-A2: ecs_field_unchecked skips bounds checks for hot path. */
            Pos *p =
#ifdef FLECS_PATCHED_BUILD
                ecs_field_unchecked(&it, Pos, 0);
#else
                ecs_field(&it, Pos, 0);
#endif
            Vel *v =
#ifdef FLECS_PATCHED_BUILD
                ecs_field_unchecked(&it, Vel, 1);
#else
                ecs_field(&it, Vel, 1);
#endif
            for (int i = 0; i < it.count; i++) {
                p[i].x += v[i].x * 0.016f;
                p[i].y += v[i].y * 0.016f;
                p[i].z += v[i].z * 0.016f;
                total++;
            }
        }
    }
    double dt = now_sec() - t0;
    double entities_per_sec = (double)(total) / dt;
    double ns_per_iter = (dt * 1e9) / (double)total;

    printf("[A] iter_throughput: %d iters x %d entities = %lld total | "
           "%.3f sec total | %.1f M ent/sec | %.2f ns/iter\n",
           n_iters, N, total, dt, entities_per_sec / 1e6, ns_per_iter);

    ecs_query_fini(q);
    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo B: Archetype churn — 50 component, 100k varlık, ardışık ekleme    */
/* -------------------------------------------------------------------------- */
typedef struct { float v; } C01;
typedef struct { float v; } C02;
typedef struct { float v; } C03;
typedef struct { float v; } C04;
typedef struct { float v; } C05;
typedef struct { float v; } C06;
typedef struct { float v; } C07;
typedef struct { float v; } C08;
typedef struct { float v; } C09;
typedef struct { float v; } C10;

static int scenario_archetype_churn(ecs_world_t *world, int n_iters) {
    /* 10 bileşen tanımla — TIER-A1.2: ECS_COMPONENT + auto EcsDontFragment
     * (Tier-A1.2 patched build'de aktif). */
    ECS_COMPONENT(world, C01);
    ECS_COMPONENT(world, C02);
    ECS_COMPONENT(world, C03);
    ECS_COMPONENT(world, C04);
    ECS_COMPONENT(world, C05);
    ECS_COMPONENT(world, C06);
    ECS_COMPONENT(world, C07);
    ECS_COMPONENT(world, C08);
    ECS_COMPONENT(world, C09);
    ECS_COMPONENT(world, C10);
#ifdef FLECS_PATCHED_BUILD
    /* TIER-A1.2: sparse storage path aktif */
    ecs_add_id(world, ecs_id(C01), EcsDontFragment);
    ecs_add_id(world, ecs_id(C02), EcsDontFragment);
    ecs_add_id(world, ecs_id(C03), EcsDontFragment);
    ecs_add_id(world, ecs_id(C04), EcsDontFragment);
    ecs_add_id(world, ecs_id(C05), EcsDontFragment);
    ecs_add_id(world, ecs_id(C06), EcsDontFragment);
    ecs_add_id(world, ecs_id(C07), EcsDontFragment);
    ecs_add_id(world, ecs_id(C08), EcsDontFragment);
    ecs_add_id(world, ecs_id(C09), EcsDontFragment);
    ecs_add_id(world, ecs_id(C10), EcsDontFragment);
#endif

    /* 100k varlık yarat */
    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
    }

    /* Her iter'de 10 component sırayla ekle/sil: archetype churn stress.
     * Patched build'de Tier-I1 batched API kullanır — tek archetype
     * transition, N add yerine. */
#ifdef FLECS_PATCHED_BUILD
    ecs_id_t comp_ids[10] = {
        ecs_id(C01), ecs_id(C02), ecs_id(C03), ecs_id(C04), ecs_id(C05),
        ecs_id(C06), ecs_id(C07), ecs_id(C08), ecs_id(C09), ecs_id(C10),
    };
#endif

    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
#ifdef FLECS_PATCHED_BUILD
        /* TIER-I1: Batched add — single archetype traversal + commit. */
        ecs_add_ids(world, es[0], comp_ids, 10);
        ecs_remove_ids(world, es[0], comp_ids, 10);
#else
        ecs_add(world, es[0], C01);
        ecs_add(world, es[0], C02);
        ecs_add(world, es[0], C03);
        ecs_add(world, es[0], C04);
        ecs_add(world, es[0], C05);
        ecs_add(world, es[0], C06);
        ecs_add(world, es[0], C07);
        ecs_add(world, es[0], C08);
        ecs_add(world, es[0], C09);
        ecs_add(world, es[0], C10);

        ecs_remove(world, es[0], C01);
        ecs_remove(world, es[0], C02);
        ecs_remove(world, es[0], C03);
        ecs_remove(world, es[0], C04);
        ecs_remove(world, es[0], C05);
        ecs_remove(world, es[0], C06);
        ecs_remove(world, es[0], C07);
        ecs_remove(world, es[0], C08);
        ecs_remove(world, es[0], C09);
        ecs_remove(world, es[0], C10);
#endif
    }
    double dt = now_sec() - t0;

    double ops = (double)n_iters * 20.0;
    printf("[B] archetype_churn: %d iters x 20 ops (10 add+remove) | "
           "%.3f sec | %.0f ops/sec | %.0f ns/op\n",
           n_iters, dt, ops / dt, dt * 1e9 / ops);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo C: Varlık yaşam döngüsü — bulk create + bulk delete                */
/* -------------------------------------------------------------------------- */
static int scenario_lifecycle(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, Pos);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);

    double t_create = 0, t_delete = 0;
    for (int it = 0; it < n_iters; it++) {
        double t0 = now_sec();
        for (int i = 0; i < N; i++) {
            es[i] = ecs_new_w_id(world, ecs_id(Pos));
        }
        t_create += now_sec() - t0;

        t0 = now_sec();
        for (int i = 0; i < N; i++) {
            ecs_delete(world, es[i]);
        }
        t_delete += now_sec() - t0;
    }

    printf("[C] lifecycle: %d rounds x %d create+delete | "
           "create: %.3f sec (%.0f ent/sec) | delete: %.3f sec (%.0f ent/sec)\n",
           n_iters, N, t_create, (double)(n_iters * N) / t_create,
           t_delete, (double)(n_iters * N) / t_delete);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo D: Büyük dünya — 1M varlık, 20 farklı bileşen, çoklu archetype     */
/* -------------------------------------------------------------------------- */
typedef struct { int x; } DA; typedef struct { int x; } DB;
typedef struct { int x; } DC; typedef struct { int x; } DD;
typedef struct { int x; } DE; typedef struct { int x; } DF;
typedef struct { int x; } DG; typedef struct { int x; } DH;

static int scenario_large_world(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, DA); ECS_COMPONENT(world, DB);
    ECS_COMPONENT(world, DC); ECS_COMPONENT(world, DD);
    ECS_COMPONENT(world, DE); ECS_COMPONENT(world, DF);
    ECS_COMPONENT(world, DG); ECS_COMPONENT(world, DH);

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
            /* TIER-A2: unchecked field */
            DA *p =
#ifdef FLECS_PATCHED_BUILD
                ecs_field_unchecked(&it, DA, 0);
#else
                ecs_field(&it, DA, 0);
#endif
            (void)p;
            hits += it.count;
        }
        t_iter += now_sec() - t0;
    }

    printf("[D] large_world: %d varlik (8 archetype) | "
           "create: %.3f sec (%.0f ent/sec) | iter: %.3f sec avg | "
           "%.1f M ent/sec iter throughput\n",
           N, t_create, N / t_create, t_iter / n_iters,
           ((double)hits / t_iter) / 1e6);

    ecs_query_fini(q);
    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo E: Observer-heavy — 10 OnAdd observer, bulk add                    */
/* Tier-C1'in etkisini ölçmek için.                                          */
/* -------------------------------------------------------------------------- */
typedef struct { int x; } EO1; typedef struct { int x; } EO2;
typedef struct { int x; } EO3; typedef struct { int x; } EO4;
typedef struct { int x; } EO5; typedef struct { int x; } EO6;
typedef struct { int x; } EO7; typedef struct { int x; } EO8;
typedef struct { int x; } EO9; typedef struct { int x; } EO10;

static int obs_invokes = 0;
static void eo_observer_cb(ecs_iter_t *it) {
    (void)it;
    obs_invokes += it->count;
}

static int scenario_observer_heavy(ecs_world_t *world, int n_iters) {
    /* Bu senaryo observer tetiklemesini ölçer, ECS_COMPONENT (regular) kullanır
     * çünkü EcsDontFragment observer dispatch'i bypass eder. */
    ECS_COMPONENT(world, EO1);
    ECS_COMPONENT(world, EO2);
    ECS_COMPONENT(world, EO3);
    ECS_COMPONENT(world, EO4);
    ECS_COMPONENT(world, EO5);
    ECS_COMPONENT(world, EO6);
    ECS_COMPONENT(world, EO7);
    ECS_COMPONENT(world, EO8);
    ECS_COMPONENT(world, EO9);
    ECS_COMPONENT(world, EO10);
    (void)0;

    ecs_entity_t comps[10] = {
        ecs_id(EO1), ecs_id(EO2), ecs_id(EO3), ecs_id(EO4), ecs_id(EO5),
        ecs_id(EO6), ecs_id(EO7), ecs_id(EO8), ecs_id(EO9), ecs_id(EO10),
    };
    for (int i = 0; i < 10; i++) {
        ecs_observer(world, {
            .query.terms = {{ .id = comps[i] }},
            .events = { EcsOnAdd },
            .callback = eo_observer_cb
        });
    }

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) es[i] = ecs_new(world);

    obs_invokes = 0;
    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
        for (int k = 0; k < 10; k++) {
            ecs_add_id(world, es[it % N], comps[k]);
        }
    }
    double dt = now_sec() - t0;
    double ops = (double)n_iters * 10.0;
    printf("[E] observer_heavy: %d iters x 10 add | "
           "%.3f sec | %.2f M add/sec | %.0f ns/add | invokes=%d\n",
           n_iters, dt, ops / dt / 1e6, dt * 1e9 / ops, obs_invokes);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo F: Archetype churn WITHOUT EcsDontFragment — Tier-I1'in etkisini   */
/* ölçer (batch add vs N×add).                                                */
/* -------------------------------------------------------------------------- */
typedef struct { float v; } F01;
typedef struct { float v; } F02;
typedef struct { float v; } F03;
typedef struct { float v; } F04;
typedef struct { float v; } F05;
typedef struct { float v; } F06;
typedef struct { float v; } F07;
typedef struct { float v; } F08;
typedef struct { float v; } F09;
typedef struct { float v; } F10;

static int scenario_archetype_churn_dense(ecs_world_t *world, int n_iters) {
    /* 10 bileşen — archetype storage (EcsDontFragment YOK).
     * Bu senaryo Tier-I1'in gerçek kazancını gösterir: 10× flecs_commit yerine
     * 1× flecs_commit (batched path). */
    ECS_COMPONENT(world, F01);
    ECS_COMPONENT(world, F02);
    ECS_COMPONENT(world, F03);
    ECS_COMPONENT(world, F04);
    ECS_COMPONENT(world, F05);
    ECS_COMPONENT(world, F06);
    ECS_COMPONENT(world, F07);
    ECS_COMPONENT(world, F08);
    ECS_COMPONENT(world, F09);
    ECS_COMPONENT(world, F10);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
    }

    ecs_id_t comp_ids[10] = {
        ecs_id(F01), ecs_id(F02), ecs_id(F03), ecs_id(F04), ecs_id(F05),
        ecs_id(F06), ecs_id(F07), ecs_id(F08), ecs_id(F09), ecs_id(F10),
    };

    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
#ifdef FLECS_PATCHED_BUILD
        /* TIER-I1: Batched add+remove — tek archetype transition. */
        ecs_add_ids(world, es[0], comp_ids, 10);
        ecs_remove_ids(world, es[0], comp_ids, 10);
#else
        for (int k = 0; k < 10; k++) ecs_add_id(world, es[0], comp_ids[k]);
        for (int k = 0; k < 10; k++) ecs_remove_id(world, es[0], comp_ids[k]);
#endif
    }
    double dt = now_sec() - t0;

    double ops = (double)n_iters * 20.0;
    printf("[F] archetype_churn_dense (no sparse): %d iters x 20 ops | "
           "%.3f sec | %.0f ops/sec | %.0f ns/op\n",
           n_iters, dt, ops / dt, dt * 1e9 / ops);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo G: Bulk set_id — 10k varlığa 5 component set etme                 */
/* Tier-A2 (ecs_field_unchecked) + Tier-C2 (prefetch) ölçümü.                */
/* -------------------------------------------------------------------------- */
typedef struct { float x, y, z, w; } GP;
typedef struct { float a, b, c, d; } GQ;
typedef struct { int x; } GR;

static int scenario_bulk_set(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, GP);
    ECS_COMPONENT(world, GQ);
    ECS_COMPONENT(world, GR);

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        ecs_set(world, es[i], GP, {(float)i, (float)i, (float)i, (float)i});
        ecs_set(world, es[i], GQ, {(float)i, (float)i, (float)i, (float)i});
        ecs_set(world, es[i], GR, {i});
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_set(world, es[i], GP, {(float)i, (float)i, (float)i, (float)i});
            total++;
        }
    }
    double dt = now_sec() - t0;

    printf("[G] bulk_set: %d iters x %d sets | %.3f sec | "
           "%.2f M set/sec | %.0f ns/set\n",
           n_iters, N, dt, (double)(total) / dt / 1e6, dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo H: Multi-archetype query — 50 archetype, 100k ent, 10 column scan */
/* -------------------------------------------------------------------------- */
typedef struct { int x; } HA; typedef struct { int x; } HB;
typedef struct { int x; } HC; typedef struct { int x; } HD;
typedef struct { int x; } HE; typedef struct { int x; } HF;
typedef struct { int x; } HG; typedef struct { int x; } HH;
typedef struct { int x; } HI; typedef struct { int x; } HJ;
typedef struct { int x; } HK; typedef struct { int x; } HL;
typedef struct { int x; } HM; typedef struct { int x; } HN;
typedef struct { int x; } HO; typedef struct { int x; } HP;

static int scenario_multi_arch_query(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, HA); ECS_COMPONENT(world, HB);
    ECS_COMPONENT(world, HC); ECS_COMPONENT(world, HD);
    ECS_COMPONENT(world, HE); ECS_COMPONENT(world, HF);
    ECS_COMPONENT(world, HG); ECS_COMPONENT(world, HH);
    ECS_COMPONENT(world, HI); ECS_COMPONENT(world, HJ);
    ECS_COMPONENT(world, HK); ECS_COMPONENT(world, HL);
    ECS_COMPONENT(world, HM); ECS_COMPONENT(world, HN);
    ECS_COMPONENT(world, HO); ECS_COMPONENT(world, HP);

    const int N = 100000;
    ecs_id_t arch_comps[16] = {
        ecs_id(HA), ecs_id(HB), ecs_id(HC), ecs_id(HD),
        ecs_id(HE), ecs_id(HF), ecs_id(HG), ecs_id(HH),
        ecs_id(HI), ecs_id(HJ), ecs_id(HK), ecs_id(HL),
        ecs_id(HM), ecs_id(HN), ecs_id(HO), ecs_id(HP),
    };

    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        /* Her varlığa 1-4 arası bileşen ata (i % 4) */
        int n_comp = (i % 4) + 1;
        for (int k = 0; k < n_comp; k++) {
            ecs_add_id(world, es[i], arch_comps[k]);
        }
    }

    /* HA sorgusu — 4 archetype'in tümünde eşleşir (her HA bileşeni 1+ kere var) */
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(HA) }}
    });

    double t_iter = 0;
    long long hits = 0;
    for (int it = 0; it < n_iters; it++) {
        double t0 = now_sec();
        ecs_iter_t it = ecs_query_iter(world, q);
        while (ecs_query_next(&it)) {
            HA *p =
#ifdef FLECS_PATCHED_BUILD
                ecs_field_unchecked(&it, HA, 0);
#else
                ecs_field(&it, HA, 0);
#endif
            (void)p;
            hits += it.count;
        }
        t_iter += now_sec() - t0;
    }

    printf("[H] multi_arch_query: %d iters, %d ent, 16 mut, %lld hits total | "
           "%.3f sec total | %.1f M ent/sec iter\n",
           n_iters, N, hits, t_iter,
           ((double)hits / t_iter) / 1e6);

    ecs_query_fini(q);
    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo I: Bulk_new archetype — 10k varlığı tek seferde oluşturma          */
/* Tier-A1'in bulk path üzerindeki etkisini ölçer.                           */
/* -------------------------------------------------------------------------- */
typedef struct { float x, y, z; } IP;
typedef struct { float a, b, c; } IQ;

static int scenario_bulk_new(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, IP);
    ECS_COMPONENT(world, IQ);

    const int N = 10000;
    const ecs_type_info_t *ti_p = ecs_get_type_info(world, ecs_id(IP));
    const ecs_type_info_t *ti_q = ecs_get_type_info(world, ecs_id(IQ));
    (void)ti_p; (void)ti_q;

    /* Toplu oluşturulacak entity ID'leri önceden allocate et */
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    ecs_os_memset_t(es, 0, ecs_entity_t, N);

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            es[i] = ecs_new(world);
        }
        ecs_add(world, es[0], IP);
        ecs_add(world, es[0], IQ);
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[I] bulk_create_with_add: %d iters x %d new+2add | "
           "%.3f sec | %.2f M ent/sec | %.0f ns/ent\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo J: Single component bulk_set — 100k varlığa tek component yaz     */
/* -------------------------------------------------------------------------- */
typedef struct { float x; } JP;

static int scenario_single_bulk_set(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, JP);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_set(world, es[i], JP, {(float)i});
            total++;
        }
    }
    double dt = now_sec() - t0;
    printf("[J] single_bulk_set: %d iters x %d set | "
           "%.3f sec | %.2f M set/sec | %.0f ns/set\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo K: Bulk new — 100k entity aynı component ile tek seferde           */
/* Tier-L1 (ecs_bulk_new_w_id) + Tier-J2 (last_cr cache) test                */
/* -------------------------------------------------------------------------- */
typedef struct { int x; } KP;

static int scenario_bulk_new_set(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, KP);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* bulk_new_w_id: N entity'i tek seferde KP component ile oluşturur */
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(KP), N);
        if (ids) {
            ecs_os_memcpy(es, ids, sizeof(ecs_entity_t) * N);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[K] bulk_new_w_id: %d iters x %d entities | "
           "%.3f sec | %.2f M ent/sec | %.0f ns/ent\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo L: Bulk set_id — 100k entity'ye tek component value (Tier-O1)     */
/* Tier-O1: ecs_bulk_set_id — N entity'ye aynı value'yu tek defer cycle'de  */
/* -------------------------------------------------------------------------- */
typedef struct { float x, y, z; } LP;

static int scenario_bulk_set_value(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, LP);

    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    LP shared_value = {1.0f, 2.0f, 3.0f};

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* Bulk_new ile N entity oluştur (LP component ile) */
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(LP), N);
        if (ids) {
            ecs_os_memcpy(es, ids, sizeof(ecs_entity_t) * N);
        }
        /* Tier-O1: tüm entity'lere tek value'yu set et */
#ifdef FLECS_PATCHED_BUILD
        ecs_bulk_set_id(world, es, N, ecs_id(LP), sizeof(LP), &shared_value);
#else
        for (int i = 0; i < N; i++) {
            ecs_set(world, es[i], LP, {1.0f, 2.0f, 3.0f});
        }
#endif
        total += N;

#ifdef FLECS_PATCHED_BUILD
        /* Tier-CC1 correctness check: validate first 10 entities have the
         * correct value. Tier-CC1 uses direct column indexing so a bug
         * would produce wrong data — verify on every 100th iter. */
        if ((it % 100) == 0) {
            for (int j = 0; j < 10; j++) {
                const LP *p = ecs_get(world, es[j], LP);
                if (!p || p->x != 1.0f || p->y != 2.0f || p->z != 3.0f) {
                    printf("CORRECTNESS FAIL: entity[%d]=%lld wrong value\n",
                        j, (long long)es[j]);
                    break;
                }
            }
        }
#endif
    }
    double dt = now_sec() - t0;
    printf("[L] bulk_set_value: %d iters x %d sets | "
           "%.3f sec | %.2f M set/sec | %.0f ns/set\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo M: Bulk delete — 10k entity'yi tek seferde sil (Tier-P1)           */
/* -------------------------------------------------------------------------- */
typedef struct { int x; } MP;

static int scenario_bulk_delete(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, MP);

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* Bulk_new ile N entity oluştur (MP component ile) */
        const ecs_entity_t *ids = ecs_bulk_new_w_id(world, ecs_id(MP), N);
        if (ids) {
            ecs_os_memcpy(es, ids, sizeof(ecs_entity_t) * N);
        }
        /* Tier-P1: tüm entity'leri tek defer cycle'da sil */
#ifdef FLECS_PATCHED_BUILD
        ecs_bulk_delete(world, es, N);
#else
        for (int i = 0; i < N; i++) {
            ecs_delete(world, es[i]);
        }
#endif
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[M] bulk_delete: %d iters x %d deletes | "
           "%.3f sec | %.2f M del/sec | %.0f ns/del\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo N: Bulk new with values — bulk_new + bulk_set combined (Tier-R1)  */
/* -------------------------------------------------------------------------- */
typedef struct { float x, y, z; } NP;

static int scenario_bulk_new_value(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, NP);

    const int N = 100000;
    NP shared_value = {1.0f, 2.0f, 3.0f};

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
#ifdef FLECS_PATCHED_BUILD
        /* Tier-R1: tek çağrıda bulk_new + bulk_set */
        ecs_bulk_new_w_value(world, ecs_id(NP), N, &shared_value, sizeof(NP));
#else
        for (int i = 0; i < N; i++) {
            ecs_entity_t e = ecs_new(world);
            ecs_set(world, e, NP, {1.0f, 2.0f, 3.0f});
        }
#endif
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[N] bulk_new_w_value: %d iters x %d new+set | "
           "%.3f sec | %.2f M ent/sec | %.0f ns/ent\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Senaryo O: Full lifecycle bulk — bulk_new_w_value + bulk_delete (Tier-T2)  */
/* -------------------------------------------------------------------------- */
typedef struct { float x; } OP;

static int scenario_full_lifecycle_bulk(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, OP);

    const int N = 50000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    OP shared_value = {42.0f};

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* Phase 1: bulk_new + bulk_set combined */
        const ecs_entity_t *ids =
#ifdef FLECS_PATCHED_BUILD
            ecs_bulk_new_w_value(world, ecs_id(OP), N, &shared_value, sizeof(OP));
#else
            NULL;
#endif
        if (ids) {
            ecs_os_memcpy(es, ids, sizeof(ecs_entity_t) * N);
        }
        /* Phase 2: delete */
#ifdef FLECS_PATCHED_BUILD
        ecs_bulk_delete(world, es, N);
#else
        for (int i = 0; i < N; i++) {
            ecs_delete(world, es[i]);
        }
#endif
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[O] full_lifecycle_bulk: %d iters x %d (create+set+del) | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);

    ecs_os_free(es);
    return 0;
}

/* --------------------------------------------------------------------------
 * Tier v14.1 v8 Benchmark Suite — 10 NEW scenarios for regression coverage
 * (Added 2026-06-22)
 * ------------------------------------------------------------------------- */

/* Senaryo P: ecs_new + ecs_delete churn (entity churn) */
typedef struct { int32_t v; } PP;

static int scenario_entity_churn(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, PP);
    const int N = 50000;

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
        for (int i = 0; i < N; i++) {
            es[i] = ecs_new(world);
            ecs_add(world, es[i], PP);
        }
        for (int i = 0; i < N; i++) {
            ecs_delete(world, es[i]);
        }
        ecs_os_free(es);
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[P] entity_churn: %d iters x %d new+del | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    return 0;
}

/* Senaryo Q: Observer 100K events */
typedef struct { int32_t v; } QP;

static int g_q_count = 0;
static void q_obs(ecs_iter_t *it) {
    g_q_count += it->count;
    (void)it;
}

static int scenario_observer_100k_events(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, QP);
    ecs_observer_init(world, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(QP) }},
        .events = { EcsOnAdd },
        .callback = q_obs
    });

    const int N = 100000;

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_entity_t e = ecs_new(world);
            ecs_add(world, e, QP);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[Q] observer_100k_events: %d iters x %d add | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    return 0;
}

/* Senaryo R: Pair add/remove */
typedef struct { int32_t v; } RP;

static int scenario_pair_add_remove(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, RP);
    ecs_entity_t rel = ecs_new(world);

    const int N = 50000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        ecs_add(world, es[i], RP);
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_add_id(world, es[i], rel);
            ecs_remove_id(world, es[i], rel);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[R] pair_add_remove: %d iters x %d add+rem | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    ecs_os_free(es);
    return 0;
}

/* Senaryo S: 100K get */
typedef struct { int32_t v; } SP;

static int scenario_100k_get(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, SP);
    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        SP val = { .v = i };
        ecs_set_id(world, es[i], ecs_id(SP), sizeof(SP), &val);
    }

    long long checksum = 0;
    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            const SP *p = ecs_get(world, es[i], SP);
            if (p) checksum += p->v;
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[S] 100k_get: %d iters x %d get | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op (cs=%lld)\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total, checksum);
    ecs_os_free(es);
    return 0;
}

/* Senaryo T: 100K set */
typedef struct { int32_t v; } TP;

static int scenario_100k_set(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, TP);
    const int N = 100000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            TP val = { .v = i };
            ecs_set_id(world, es[i], ecs_id(TP), sizeof(TP), &val);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[T] 100k_set: %d iters x %d set | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    ecs_os_free(es);
    return 0;
}

/* Senaryo U: Archetype migration (1K add+remove) */
typedef struct { int32_t v; } UP1;
typedef struct { int32_t v; } UP2;
typedef struct { int32_t v; } UP3;
typedef struct { int32_t v; } UP4;

static int scenario_archetype_migration(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, UP1);
    ECS_COMPONENT(world, UP2);
    ECS_COMPONENT(world, UP3);
    ECS_COMPONENT(world, UP4);

    const int N = 1000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        ecs_add(world, es[i], UP1);
    }

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* Add UP2, UP3, UP4 — forces migration */
        for (int i = 0; i < N; i++) {
            ecs_add(world, es[i], UP2);
            ecs_add(world, es[i], UP3);
            ecs_add(world, es[i], UP4);
        }
        /* Remove UP4, UP3, UP2 — migrate back */
        for (int i = 0; i < N; i++) {
            ecs_remove(world, es[i], UP4);
            ecs_remove(world, es[i], UP3);
            ecs_remove(world, es[i], UP2);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[U] archetype_migration: %d iters x %d (6 add/remove) | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    ecs_os_free(es);
    return 0;
}

/* Senaryo V: IsA 5-level chain */
typedef struct { int32_t v; } VP;

static int scenario_isa_5level(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, VP);
    ecs_entity_t bs[5];
    bs[0] = ecs_new(world);
    VP val = { .v = 100 };
    ecs_set_id(world, bs[0], ecs_id(VP), sizeof(VP), &val);
    for (int i = 1; i < 5; i++) {
        bs[i] = ecs_new(world);
        ecs_add_pair(world, bs[i], EcsIsA, bs[i-1]);
    }
    ecs_entity_t inst = ecs_new(world);
    ecs_add_pair(world, inst, EcsIsA, bs[4]);

    const int N = 100000;
    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            const VP *p = ecs_get(world, inst, VP);
            (void)p;
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[V] isa_5level: %d iters x %d get | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    return 0;
}

/* Senaryo W: OnAdd + OnRemove stress */
typedef struct { int32_t v; } WP;

static void w_add(ecs_iter_t *it) { (void)it; }
static void w_rem(ecs_iter_t *it) { (void)it; }

static int scenario_add_remove_stress(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, WP);
    ecs_observer_init(world, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(WP) }},
        .events = { EcsOnAdd },
        .callback = w_add
    });
    ecs_observer_init(world, &(ecs_observer_desc_t){
        .query.terms = {{ .id = ecs_id(WP) }},
        .events = { EcsOnRemove },
        .callback = w_rem
    });

    const int N = 10000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) es[i] = ecs_new(world);

    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_add(world, es[i], WP);
        }
        for (int i = 0; i < N; i++) {
            ecs_remove(world, es[i], WP);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[W] add_remove_stress: %d iters x %d (add+rem) | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    ecs_os_free(es);
    return 0;
}

/* Senaryo X: Fini 10K entities */
typedef struct { int32_t v; } XP;

static int scenario_fini_10k(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, XP);
    const int N = 10000;
    long long total = 0;
    double t0 = now_sec();
    for (int it = 0; it < n_iters; it++) {
        for (int i = 0; i < N; i++) {
            ecs_entity_t e = ecs_new(world);
            ecs_add(world, e, XP);
        }
        ecs_fini(world);
        world = ecs_init();
        ECS_COMPONENT(world, XP);
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[X] fini_10k: %d iters x %d (create+fini) | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total);
    ecs_fini(world);
    return 0;
}

/* Senaryo Y: Mixed R/W 50/50 */
typedef struct { int32_t v; } YP;

static int scenario_mixed_rw(ecs_world_t *world, int n_iters) {
    ECS_COMPONENT(world, YP);
    const int N = 50000;
    ecs_entity_t *es = ecs_os_malloc_n(ecs_entity_t, N);
    for (int i = 0; i < N; i++) {
        es[i] = ecs_new(world);
        YP val = { .v = i };
        ecs_set_id(world, es[i], ecs_id(YP), sizeof(YP), &val);
    }

    long long checksum = 0;
    double t0 = now_sec();
    long long total = 0;
    for (int it = 0; it < n_iters; it++) {
        /* 50% read */
        for (int i = 0; i < N; i += 2) {
            const YP *p = ecs_get(world, es[i], YP);
            if (p) checksum += p->v;
        }
        /* 50% write */
        for (int i = 1; i < N; i += 2) {
            YP val = { .v = i + it };
            ecs_set_id(world, es[i], ecs_id(YP), sizeof(YP), &val);
        }
        total += N;
    }
    double dt = now_sec() - t0;
    printf("[Y] mixed_rw: %d iters x %d (50R/50W) | "
           "%.3f sec | %.2f M ops/sec | %.0f ns/op (cs=%lld)\n",
           n_iters, N, dt, (double)total / dt / 1e6,
           dt * 1e9 / (double)total, checksum);
    ecs_os_free(es);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Ana                                                                       */
/* -------------------------------------------------------------------------- */
int main(int argc, char **argv) {
    int n_iters = N_DEFAULT;
    if (argc > 1) n_iters = atoi(argv[1]);
    if (n_iters <= 0) n_iters = N_DEFAULT;

    printf("==== Flecs Benchmark (n_iters=%d) ====\n", n_iters);
    printf("Compiler: %s, sizeof(void*)=%zu\n",
#ifdef _MSC_VER
        "MSVC", sizeof(void*)
#elif defined(__clang__)
        "Clang", sizeof(void*)
#elif defined(__GNUC__)
        "GCC", sizeof(void*)
#else
        "Unknown", sizeof(void*)
#endif
    );

    /* TIER-N1: Warmup — n_iters küçük olduğunda veya cold-cache koşullarında
     * ilk itersyonlar yavaş olabiliyor (heap allocator first-use, page table
     * alloc, code-cache miss). Warmup ile bu soğuk etkiyi azaltıyoruz. */
    printf("==== Flecs Warmup (n_warmup=%d) ====\n", N_WARMUP);
    {
        ecs_world_t *w = ecs_init();
        scenario_iter_throughput(w, N_WARMUP);
        ecs_fini(w);
    }

    /* Her senaryo için ayrı dünya — izolasyon */
    ecs_world_t *world_a = ecs_init();
    scenario_iter_throughput(world_a, n_iters);
    ecs_fini(world_a);

    ecs_world_t *world_b = ecs_init();
    scenario_archetype_churn(world_b, n_iters);
    ecs_fini(world_b);

    ecs_world_t *world_c = ecs_init();
    scenario_lifecycle(world_c, n_iters);
    ecs_fini(world_c);

    ecs_world_t *world_d = ecs_init();
    scenario_large_world(world_d, n_iters);
    ecs_fini(world_d);

    ecs_world_t *world_e = ecs_init();
    scenario_observer_heavy(world_e, n_iters);
    ecs_fini(world_e);

    ecs_world_t *world_f = ecs_init();
    scenario_archetype_churn_dense(world_f, n_iters);
    ecs_fini(world_f);

    ecs_world_t *world_g = ecs_init();
    scenario_bulk_set(world_g, n_iters);
    ecs_fini(world_g);

    ecs_world_t *world_h = ecs_init();
    scenario_multi_arch_query(world_h, n_iters);
    ecs_fini(world_h);

    ecs_world_t *world_i = ecs_init();
    scenario_bulk_new(world_i, n_iters);
    ecs_fini(world_i);

    ecs_world_t *world_j = ecs_init();
    scenario_single_bulk_set(world_j, n_iters);
    ecs_fini(world_j);

    ecs_world_t *world_k = ecs_init();
    scenario_bulk_new_set(world_k, n_iters);
    ecs_fini(world_k);

    ecs_world_t *world_l = ecs_init();
    scenario_bulk_set_value(world_l, n_iters);
    ecs_fini(world_l);

    ecs_world_t *world_m = ecs_init();
    scenario_bulk_delete(world_m, n_iters);
    ecs_fini(world_m);

    ecs_world_t *world_n = ecs_init();
    scenario_bulk_new_value(world_n, n_iters);
    ecs_fini(world_n);

    ecs_world_t *world_o = ecs_init();
    scenario_full_lifecycle_bulk(world_o, n_iters);
    ecs_fini(world_o);

    /* Tier v14.1 v8 — 10 new regression benchmark scenarios */
    ecs_world_t *world_p = ecs_init();
    scenario_entity_churn(world_p, n_iters);
    ecs_fini(world_p);

    ecs_world_t *world_q = ecs_init();
    scenario_observer_100k_events(world_q, n_iters);
    ecs_fini(world_q);

    ecs_world_t *world_r = ecs_init();
    scenario_pair_add_remove(world_r, n_iters);
    ecs_fini(world_r);

    ecs_world_t *world_s = ecs_init();
    scenario_100k_get(world_s, n_iters);
    ecs_fini(world_s);

    ecs_world_t *world_t = ecs_init();
    scenario_100k_set(world_t, n_iters);
    ecs_fini(world_t);

    ecs_world_t *world_u = ecs_init();
    scenario_archetype_migration(world_u, n_iters);
    ecs_fini(world_u);

    ecs_world_t *world_v = ecs_init();
    scenario_isa_5level(world_v, n_iters);
    ecs_fini(world_v);

    ecs_world_t *world_w = ecs_init();
    scenario_add_remove_stress(world_w, n_iters);
    ecs_fini(world_w);

    /* X: fini_10k manages its own world, no extra fini here */
    {
        ecs_world_t *world_x = ecs_init();
        scenario_fini_10k(world_x, n_iters);
    }

    ecs_world_t *world_y = ecs_init();
    scenario_mixed_rw(world_y, n_iters);
    ecs_fini(world_y);

    return 0;
}
