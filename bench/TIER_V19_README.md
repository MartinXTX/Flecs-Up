# Tier-v19 Patches — Implementation Log

> **Status:** In-progress (11 parallel agents are integrating patches)
> **Base:** Tier-v18 (commit `330911c` + Tier-A1 + Tier-A2 + Tier-A1.3 LAZY)
> **Goal:** Unify all remaining Tiers into the Tier-v19 source

---

## Tier-v19 Patch List (12 Tiers)

| # | Tier | Patch | File | Status |
|---|---|---|---|---|
| 1 | BULGU-41 | EcsDontFragment pair crash fix | `flecs_patched_v19.c` (flecs_sparse_on_add_cr) | ⏳ agent running |
| 2 | P2-1 | entity-index flat array + alignment | `flecs_patched_v19.c` (entity_index) | ⏳ agent running |
| 3 | D2 | PGO + LTO + AVX2 pipeline | `bench/build_pgo_*.bat`, `bench/CMakePresets.json`, `bench/PGO_README.md` | ✅ complete |
| 4 | C2 | software prefetch full version | `flecs_patched_v19.c` (table_cache_next_) | ⏳ agent running |
| 5 | C1 | observer bitmap fast-path | `flecs_patched_v19.c` (observable) | ⏳ agent running |
| 6 | B2 | tiny archetype single chunk | `flecs_patched_v19.c` (table) | ⏳ agent running |
| 7 | B1 | hot/cold split + prefetch hints | `flecs_patched_v19.c` (entity_index lookup) | ⏳ agent running |
| 8 | A3 | persistent query arena complete | `flecs_patched_v19.c` (eval_iter) | ⏳ agent running |
| 9 | B3 | variable-size IDs framework | `include/flecs/private/api_defines.h` | ⏳ agent running |
| 10 | C3 | SIMD filter processing (AVX2) | `include/flecs/addons/cpp/simd_filter.hpp` (new) | ⏳ agent running |
| 11 | D1.2 | C++17 system template | `include/flecs/addons/cpp/system.hpp` (new) | ⏳ agent running |
| 12 | E2 + E3 | huge pages + cache-line alignment | `src/os_api.c`, `include/flecs/private/api_types.h` | ⏳ agent running |

**Status:** 11 parallel agents are applying patches; build + test agents complete as they finish.

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

## Implementation Order (Dependency)

**PHASE 1 (urgent):** BULGU-41 → P2-1 → D2 ✅
**PHASE 2 (parallel):** C2 + C1 + B2 (all isolated, different files)
**PHASE 3 (parallel):** B1 + A3 + B3 (eval_iter + entity_index different regions)
**PHASE 4 (parallel):** C3 (new header) + D1.2 (new header) + E2+E3 (os_api + api_types)
**PHASE 5:** Tier-A unified build

**C4 lock-free stages:** ⏸ DEFERRED — 12+ weeks, architectural risk, postponed.

---

## Expected Gain Summary

| Tier | Expected Gain | Patch Type |
|---|---|---|
| BULGU-41 | EcsDontFragment pair crash fix | Bug fix |
| P2-1 | +%5-15 `ecs_get_id`/lookup | Flat array |
| D2 PGO | +%10-20 hot-path | Build preset |
| C2 full | +%10-20 iter throughput | Prefetch |
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

**Success criteria:**
- 5-suite Tier-v18 regression PASS (backward compatible)
- Per-Tier test PASS (each Tier must pass its own test)
- Tier-A1 sparse hybrid 13/13 PASS
- LAZY heuristic 9/9 PASS
- ECS_DATA_COMPONENT 7/7 PASS
- 228+ total tests PASS, 0 leak
- `release/v19_flecs_patched.lib` production-ready
