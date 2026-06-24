# Tier-v19 Patches — Implementation Log

> **Status:** In-progress (11 paralel ajan patch'leri entegre ediyor)
> **Base:** Tier-v18 (commit `330911c` + Tier-A1 + Tier-A2 + Tier-A1.3 LAZY)
> **Hedef:** Tüm kalan Tier'ları Tier-v19 source'unda birleştirmek

---

## Tier-v19 Patch Listesi (12 Tier)

| # | Tier | Patch | Dosya | Durum |
|---|---|---|---|---|
| 1 | BULGU-41 | EcsDontFragment pair crash fix | `flecs_patched_v19.c` (flecs_sparse_on_add_cr) | ⏳ ajan çalışıyor |
| 2 | P2-1 | entity-index flat array + alignment | `flecs_patched_v19.c` (entity_index) | ⏳ ajan çalışıyor |
| 3 | D2 | PGO + LTO + AVX2 pipeline | `bench/build_pgo_*.bat`, `bench/CMakePresets.json`, `bench/PGO_README.md` | ✅ tamamlandı |
| 4 | C2 | software prefetch tam sürüm | `flecs_patched_v19.c` (table_cache_next_) | ⏳ ajan çalışıyor |
| 5 | C1 | observer bitmap fast-path | `flecs_patched_v19.c` (observable) | ⏳ ajan çalışıyor |
| 6 | B2 | tiny archetype single chunk | `flecs_patched_v19.c` (table) | ⏳ ajan çalışıyor |
| 7 | B1 | hot/cold split + prefetch hints | `flecs_patched_v19.c` (entity_index lookup) | ⏳ ajan çalışıyor |
| 8 | A3 | persistent query arena tam | `flecs_patched_v19.c` (eval_iter) | ⏳ ajan çalışıyor |
| 9 | B3 | variable-size IDs framework | `include/flecs/private/api_defines.h` | ⏳ ajan çalışıyor |
| 10 | C3 | SIMD filter processing (AVX2) | `include/flecs/addons/cpp/simd_filter.hpp` (yeni) | ⏳ ajan çalışıyor |
| 11 | D1.2 | C++17 system template | `include/flecs/addons/cpp/system.hpp` (yeni) | ⏳ ajan çalışıyor |
| 12 | E2 + E3 | huge pages + cache-line alignment | `src/os_api.c`, `include/flecs/private/api_types.h` | ⏳ ajan çalışıyor |

**Status:** 11 paralel ajan patch'leri uyguluyor, build + test ajanlar tamamlandıkça.

---

## Build Pipeline

```bash
# Tier-v19 lib
bench\build_v19_lib.bat
# → release\v19_flecs_patched.lib

# Full regression
bench\build_v19_regression_full.bat
# → 5-suite + per-Tier tests

# Tier-A unified release
bench\build_v19_a_unified.bat
# → C++ glue + D1.2 runtime test + full benchmark
```

---

## Uygulama Sırası (Dependency)

**FAZA 1 (acil):** BULGU-41 → P2-1 → D2 ✅
**FAZA 2 (paralel):** C2 + C1 + B2 (hepsi izole, dosyalar farklı)
**FAZA 3 (paralel):** B1 + A3 + B3 (eval_iter + entity_index farklı bölgeler)
**FAZA 4 (paralel):** C3 (new header) + D1.2 (new header) + E2+E3 (os_api + api_types)
**FAZA 5:** Tier-A unified build

**C4 lock-free stages:** ⏸ ERTELENDİ — 12+ hafta, mimari risk, sonraya bırakıldı.

---

## Beklenen Kazanç Özeti

| Tier | Beklenen Kazanç | Patch Tipi |
|---|---|---|
| BULGU-41 | EcsDontFragment pair crash fix | Bug fix |
| P2-1 | +%5-15 `ecs_get_id`/lookup | Flat array |
| D2 PGO | +%10-20 hot-path | Build preset |
| C2 tam | +%10-20 iter throughput | Prefetch |
| C1 bitmap | +%50-80 multi-observer | Bitmap fast-path |
| B2 tiny | +%30-50 create | Single chunk |
| B1 hot/cold | +%3-5 lookup (prefetch) | Minimal win |
| A3 arena | +%30-50 per-frame query | Persistent arena |
| B3 var-id | +%0 (framework only) | Compile-time config |
| C3 SIMD | +%200-400 filter | AVX2 |
| D1.2 system | +%10-30 system call | C++17 template |
| E2 huge | +%10-20 large world | VirtualAlloc |
| E3 align | +%5-15 MT perf | `_Alignas(64)` |

---

## Verification

```bash
bench\build_v19_lib.bat
bench\build_v19_regression_full.bat
bench\build_v19_a_unified.bat
```

**Başarı kriterleri:**
- 5-suite Tier-v18 regression PASS (geriye uyumlu)
- Per-Tier test PASS (her Tier kendi testini geçmeli)
- Tier-A1 sparse hybrid 13/13 PASS
- LAZY heuristic 9/9 PASS
- ECS_DATA_COMPONENT 7/7 PASS
- 228+ test toplam PASS, 0 leak
- `release/v19_flecs_patched.lib` production-ready