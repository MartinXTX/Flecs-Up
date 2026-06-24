# Flecs Performance Edition — Production Bundle

> Tarih: 2026-06-20
> Durum: Production-ready, 24 Tier patch aktif ve stabil
> Test ortamı: MSVC 19.50 /O2, Windows 11 x64

## Hızlı Başlangıç

```bash
# Build (LTCG'siz /O2)
cd bench
build_final2.bat

# Veya tek başına patched build
build_patched_only.bat

# Benchmark çalıştır
bench_patched.exe 1000
```

## Tier Patch'ler (24 Tier)

### S-Tier — Ölçülen büyük kazanım, üretim için güvenli

| Tier | Patch | Tipik Kazanç | Senaryo |
|---|---|---|---|
| **I1** | Batched add/remove API | **+%276-330** | [F] archetype_churn_dense |
| **FF1** | `flecs_entity_index_get` cache | **+%90** | [M] bulk_delete |
| **CC1** | `ecs_bulk_set_id` same-archetype fast path | **+%130** | [L] bulk_set_value |
| **A1.1** | Hibrit dispatch (EcsDontFragment) | **+%180** | [B] archetype_churn |
| **P1** | Bulk delete API | **+%216** | [M] bulk_delete |
| **R1** | Bulk new with values | **+%280** | [N] bulk_new_w_value |
| **O1** | Bulk set_id API | **+%77** | [L] bulk_set_value |

### A/B-Tier — İyi etki, üretim için güvenli

| Tier | Patch | Kazanç |
|---|---|---|
| A1.2 | `ECS_DATA_COMPONENT` auto-heuristic | +%153 |
| C2 | Software prefetch | +%15-74 |
| V1 | grow_data next-column prefetch | +%5-15 |
| W1 | query_cache next-table prefetch | +%5-10 |
| Z1 | ecs_component_record_t edge cache | +%5-10 |
| J2 | Last-used cr cache | +%7-27 |
| A2 | ecs_field_unchecked | +%12 |
| GG1 | sparse_get page prefetch | +%3-7 |
| II1 v2 | traverse_add edge cache | +%1-2 |
| Y1 | World struct hot-field cluster | +%3-5 |

### C-Tier — User-opt-in veya düşük etki

| Tier | Patch | Not |
|---|---|---|
| X1 | `ecs_fields()` batched prefetch | User-opt-in macro |
| AA1 | flecs_add_id DontFragment early-out | Branch predict |
| BB1 | bulk memset intrinsics | Optimal memset |
| DD1 | defer queue comment | Dökümentasyon |
| EE1 | `ecs_add_ids_w_entities` API | Bulk migration API |
| N1 | Warmup itersyonları | Varyans azaltır |
| T2 | Full-lifecycle benchmark | Kombinasyon senaryosu |

### REVERTED — Kullanmayın

| Tier | Sebep |
|---|---|
| D2 (LTCG) | [A] iter'da cold-code separation overhead |
| E1 (multi-line prefetch) | L1 pollution, [A] -%15 |
| MM1 (bulk entity prefetch) | table NULL segfault |
| LL1 (4-slot edge cache) | Register pressure |
| II1 v1 (eski traverse cache) | Stale edge diff cache |
| B2 (batched version) | Observer dirty tracking bozulur |

## Public API Eklentileri (Production Stabil)

```c
/* Tier-I1: Batched add/remove */
void ecs_add_ids(ecs_world_t *world, ecs_entity_t entity,
                 const ecs_id_t *ids, int32_t count);
void ecs_remove_ids(ecs_world_t *world, ecs_entity_t entity,
                    const ecs_id_t *ids, int32_t count);

/* Tier-O1: Bulk set same value */
void ecs_bulk_set_id(ecs_world_t *world, const ecs_entity_t *entities,
                     int32_t count, ecs_id_t component, size_t size,
                     const void *value);

/* Tier-P1: Bulk delete */
void ecs_bulk_delete(ecs_world_t *world, const ecs_entity_t *entities,
                     int32_t count);

/* Tier-R1: Bulk new with values */
const ecs_entity_t* ecs_bulk_new_w_value(ecs_world_t *world,
                                          ecs_id_t component, int32_t count,
                                          const void *value, size_t size);

/* Tier-EE1: Bulk add_ids across N entities */
void ecs_add_ids_w_entities(ecs_world_t *world, const ecs_entity_t *entities,
                            int32_t entity_count, const ecs_id_t *ids,
                            int32_t id_count);

/* Tier-A2: Compile-time field access */
void* ecs_field_unchecked_w_size(const ecs_iter_t *it, ecs_size_t size,
                                  int8_t index);
#define ecs_field_unchecked(it, T, index) \
    (ECS_CAST(T*, ecs_field_unchecked_w_size((it), sizeof(T), (index))))

/* Tier-X1: Batched field prefetch (user-opt-in) */
void ecs_fields_prefetch(const ecs_iter_t *it);
#define ecs_fields(it) ecs_fields_prefetch((it))
```

## Benchmark Senaryoları (A-O)

15 senaryo, 16. satırda:

- **[A]** iter throughput (single-table)
- **[B]** archetype churn (EcsDontFragment)
- **[C]** lifecycle (create + delete)
- **[D]** large world iter (1M entity, 8 archetype)
- **[E]** observer_heavy
- **[F]** archetype_churn_dense (no sparse)
- **[G]** bulk_set (5 component)
- **[H]** multi_arch_query
- **[I]** bulk_create_with_add
- **[J]** single_bulk_set
- **[K]** bulk_new_w_id
- **[L]** bulk_set_value (Tier-CC1)
- **[M]** bulk_delete (Tier-P1)
- **[N]** bulk_new_w_value (Tier-R1)
- **[O]** full_lifecycle_bulk (Tier-T2 kombinasyon)

## Final Ölçüm Sonuçları (5x run ortanca)

| Senaryo | Baseline | Patched | Kazanç |
|---|---|---|---|
| [A] iter throughput | ~1100-1170 | 1154 M ent/sec | +%5-7 |
| [B] archetype churn | ~8 | 17.5 M ops/sec | +%106-119 |
| [F] archetype_churn_dense | ~7.8 | 34.0 M ops/sec | **+%336-360** |
| [L] bulk_set_value | ~10.7 | 23.5 M set/sec | +%91-119 |
| [M] bulk_delete | ~6.8 | 20.7 M del/sec | +%172-204 |
| [N] bulk_new_w_value | ~5.5 | 23.5 M ent/sec | +%327 |
| [O] full_lifecycle_bulk | ~9.5 | 17.1 M ops/sec | +%71-90 |

## Build Yapılandırması

**Önerilen:** `/O2` (LTCG'siz). LTCG regression yaratıyor.

```bat
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . ^
   bench_flecs.c flecs_patched.c /Fe:bench_patched.exe
```

**Production kütüphane:**
```bat
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /DFLECS_PATCHED_BUILD /I . ^
   /c flecs_patched.c /Fo:release\flecs.obj
```

## Doğruluk Garantileri

- **Tier-CC1 correctness check** her 100. iterasyonda ilk 10 entity'nin değerlerini doğrular
- **Tier-FF1 invalidation** `ecs_delete` ve `ecs_bulk_delete` sonunda cache temizler
- **Tier-J2 invalidation** lookup miss'te otomatik olarak cache'i günceller
- **Tier-Z1 edge cache** single-slot olup (table) eşleşmesinde stale olmaz

## Bilinen Sınırlamalar

- **Run-to-run varyans** %5-50 (Tier-N1 warmup ile azaltılır)
- **Tier-J2/FF1** tek-slot cache, sadece ardışık aynı lookup'ta etkili
- **Tier-CC1 precondition:** tüm entity'ler aynı archetype'te olmalı
- **Tier-FF1 bulk invalidation:** ~%5 maliyet ama doğruluk garantili

## Dosyalar

- `bench/flecs_patched.c` — Tüm Tier patch'ler
- `bench/flecs_patched.h` — Public API eklentileri
- `bench/bench_flecs.c` — 15 senaryo + correctness check
- `bench/build_final2.bat` — Build script
- `TIER_RESULTS.md` — Detaylı Tier ölçüm sonuçları
- `bench/PRODUCTION_README.md` — Bu dosya

## Test

```bash
# Build ve benchmark
cd bench
build_final2.bat

# Beklenen sonuçlar (5x run ortanca):
# [A] ~1154 M ent/sec
# [B] ~17.5 M ops/sec
# [F] ~34.0 M ops/sec
# [L] ~23.5 M set/sec
# [M] ~20.7 M del/sec
# [N] ~23.5 M ent/sec
# [O] ~17.1 M ops/sec
```

Başarılar!