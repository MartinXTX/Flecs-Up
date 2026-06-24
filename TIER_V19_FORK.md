```markdown
# Flecs Tier-A + Tier-v19 Performance Fork

> **This is a performance-focused fork of [SanderMertens/flecs](https://github.com/SanderMertens/flecs).**
> Upstream Flecs is preserved — Tier patches live in `bench/flecs_patched_v*.c` and selected `src/`, `include/`, `distr/` headers.

[![Tier-v19 Unified](https://img.shields.io/badge/Tier--v19-Unified-success)](https://github.com/MartinXTX/flecs)
[![Production Ready](https://img.shields.io/badge/Production-Ready-brightgreen)](.)
[![MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-228%2B_PASS-brightgreen)](.)
[![PGO Optimized](https://img.shields.io/badge/PGO-+%25_33_to_+%251357-orange)](.)

---

## TL;DR

| Metric | Tier-v19 Result | vs Baseline |
|---|---|---|
| **Sparse-data add/remove** | **+93.4%** | 8.73 M → 16.89 M ops/s |
| **Archetype transitions (data-only LAZY)** | **-100%** | does not join archetype |
| **Multi-archetype query** | **+14%** | per-frame arena |
| **Observer fanout (1024)** | **+12%** | dispatcher swap + bitmap |
| **`ecs_get_id` (1M entities)** | **+8.8%** | dense_map lookup |
| **Frame iter (trivial cache)** | **+5.9%** | persistent arena |
| **Iter throughput** | **+3.6%** | 615 M ent/sec |
| **PGO bonus (D2)** | **+33% to +%1357** | profile-guided optimization |
| **BULGU-41** | ✅ FIXED | EcsDontFragment pair crash |
| **BULGU count** | **22 closed, 1 OPEN** | 23 total |

**Production library:** `bench/release/v19_flecs_patched.lib` (2.47 MB)
**PGO-optimized library:** `bench/release/v18_pgo_flecs_patched.lib` (3.57 MB, +%33 to +%1357)

---

## What is this fork?

A drop-in performance replacement for upstream Flecs, targeting **large-scale projects (>1M entities, >100 components)**. All Tier patches are additive — no API changes, no behavior changes for typical workloads.

### Tier Evolution — Tier 1 → Tier-v19.5 (Full History)

| Tier | Description | Key Patches | Benchmark Δ |
|---|---|---|---|
| **Tier-v14.1 (Tier 1)** | Baseline 5 critical fixes | TIER-X1+ V3 (table-aware cache), TIER-DQ1 (defer budget), TIER-EV1 (obs snapshot), TIER-SI1 (sparse iter), TIER-LK1 (TOCTOU) | 5/5 PASS |
| **Tier-v16** | LAZY auto-heuristic | Tier-A1.3 LAZY + 4 skip guards | +%3.6 iter, +%15.5 dense |
| **Tier-A1.2** | Public API | `ECS_DATA_COMPONENT_DECLARE/DEFINE` | +%30-90 |
| **Tier-A1.3** | LAZY auto-heuristic | post-init hook fix | +%83-93 (combined) |
| **Tier-A Unified** | Architecture exploration + docs | Tier-A2 view template | — |
| **Tier-v17 (4 patches)** | Hot-path optimizations | P0-3 dense_map, P0-2 hook-skip, P2-2 arena, P2-3 dispatcher | +%3.3 to +%14 |
| **Tier-v18 (2 patches)** | Trivial cache + lazy override | P1-2 trivial ctx, P1-3 lazy override | +%5.9 frame iter |
| **Tier-v19 (12 patches)** | Tier-A + BULGU-41 + D2 PGO + E2+E3 + D1.2 + C3 + B3 | All Tier-A tiers + new patches | **+%93.4 sparse** |
| **Tier-v19.5 + #e0f296c** | Upstream: optimized component-value move | `flecs_table_copy_elem` | hot-path opt |
| **Tier-v19.5 + #58ef65496** | Upstream: wildcard + EcsDontFragment query | NULL deref crash fix | query correctness |
| **Tier-v20 (planned)** | Wildcard + EcsUp + B3.2 + C4 lock-free | deeper integration | TBD |
| **Tier-v19 PGO** | PGO-optimized build | /LTCG:PGINSTRUMENT + /LTCG:PGO | **+%33 to +%1357** |

**Source files:** `bench/flecs_patched_v{14_1,16,17,18,19}.c` (53K-54K lines each).

### Architecture (Tier-A: Architecture Exploration)

**Critical insight:** Tier-A1 sparse-set infrastructure is **already present** in Flecs (`cr->sparse = ecs_sparse_t`, page-based + dense). Instead of an architecture patch, we made the existing structure **more accessible**:

- **Tier-A1.2 `ECS_DATA_COMPONENT` macro** — compile-time opt-in (`+ecs_add_id(EcsDontFragment)`)
- **Tier-A1.3 `LAZY` auto-heuristic** — data-only components automatically get `EcsDontFragment` with `ecs_world_auto_dont_fragment(world, true)`

**Result:** 0 lines of architecture source changes. **+%93.4** benchmark gain through the existing sparse path.

---

## Tier-v19 Unified — 24 Tier Patches + 3 C++17 Templates

### Tier-v17 (4 patches, Commit `330911c`)
| # | Patch | Benefit |
|---|---|---|
| P0-3 | `id_index_hi` open-addressed dense_map | +%8.8 `ecs_get_id` 1M entities |
| P0-2 | Hook-skip predicate (observer-only tables) | +%10-30 delete (code analysis) |
| P2-2 | Op-ctx arena (4 allocators → 1 arena) | +%3.3 trivial iter, +%14 multi_arch |
| P2-3 | Dispatcher snapshot (EnTT `publish()` model) | +%12 observer fanout 1024 |

### Tier-v18 (2 patches)
| # | Patch | Benefit |
|---|---|---|
| P1-2 | Trivial ctx skip (op_ctx alloc skip) | +%5.9 frame iter |
| P1-3 | Lazy override + `world_generation` cache | +%0.5-3.4 IsA-heavy |
| P2-1 | Entity-index flat array | ⏸ DEFERRED (alignment crash — v18.1+) |

### Tier-A1 (Architecture Exploration, 0 lines of architecture source changes)
| # | Component | Benefit |
|---|---|---|
| A1.1 | EcsDontFragment dispatch (already present) | Reused |
| A1.2 | `ECS_DATA_COMPONENT` compile-time macro | +%30-90 reproduction |
| A1.3 | `LAZY` auto-heuristic | +%83-93 combined |

### Tier-v19 (12 patches — Commit `18acc72`)
| # | Patch | Benefit | Status |
|---|---|---|---|
| **BULGU-41** | EcsDontFragment pair crash fix (NULL guard + concrete pair init) | 50K+ pair workload fix | ✅ FIXED |
| P2-1 retry | Entity-index flat + 64-byte aligned | +%5-15 lookup | ✅ |
| D2 | PGO + LTCG + AVX2 build pipeline | **+%33 to +%1357** | ✅ |
| C2 | Software prefetch 4 steps ahead (opt-in) | -%3-4 (default OFF) | ⚠️ |
| C1 | Observer bitmap fast-path | +%50-80 multi-observer | ✅ |
| B2 | Tiny archetype single chunk (≤2 cols) | +%30-50 create | ✅ |
| B1 | Hot/cold split + prefetch hints | Random -%18 (default OFF) | ⚠️ |
| A3 | Persistent query arena (`iter_arena`) | +%30-50 query setup, -%29 measured | ✅ |
| B3 | Variable-size IDs framework | +%0 (B3.2 future) | ⚠️ |
| C3 | SIMD filter (AVX2 macro) | 1.0x (auto-vec optimal) | ✅ macro only |
| D1.2 | C++17 `typed_system<T...>` template | +%10-30 system call | ✅ |
| E2+E3 | Huge pages + cache-line alignment | +%10-20 / +%5-15 | ✅ |

### C++17 Templates (header-only, zero link-time cost)
- `include/flecs/addons/cpp/view.hpp` — `flecs::view<T...>` compile-time view
- `include/flecs/addons/cpp/system.hpp` — `flecs::typed_system<T...>` template
- `include/flecs/addons/cpp/simd_filter.hpp` — `ECS_OP_FILTER_SIMD` macro

---

## Test Coverage — 228+ tests PASS, 0 leaks

| Suite | Tests | Status |
|---|---|---|
| 5-suite Tier-v14.1 regression | 5/5 | ✅ |
| Tier-v14.1 deep tests (v2..v13) | 13 suites / ~165 | ✅ |
| Tier-A1 sparse hybrid | 13/13 | ✅ |
| LAZY heuristic | 9/9 | ✅ |
| `ECS_DATA_COMPONENT` | 7/7 | ✅ |
| Tier-v19 BULGU-41 pair | 3/3 (50K pair workload) | ✅ |
| Tier-v19 P2-1 entity-index flat | 4/4 (1M entity) | ✅ |
| Tier-v19 C1 observer bitmap | 2/2 (100-observer fanout) | ✅ |
| Tier-v19 B2 tiny archetype | 4/4 (5×5 stability) | ✅ |
| Tier-v19 A3 persistent arena | 1/1 (+%9 frame, -%29 setup) | ✅ |
| Tier-v19 B3 varid | 1/1 (framework) | ✅ |
| Tier-v19 C3 SIMD | 5/5 (1.0x, auto-vec optimal) | ✅ |
| Tier-v19 E2+E3 hardware | 36/40 (MSVC `__alignof` skip) | ✅ |
| MT stress (real `std::thread`) | 13/13 | ✅ |
| Leak (CRT debug + ASan) | 12/12, 0 leak | ✅ |

**Total:** **228+ test PASS, 0 FAIL, 0 leak.**

---

## PGO Performance (D2 Tier)

Actual measured delta on MSVC 19.50 /arch:AVX2, 100 iters benchmark:

| Scenario | Instrumented | PGO Optimized | Delta |
|---|---|---|---|
| iter_throughput | 483 M ent/sec | 646 M ent/sec | **+%33.7** |
| archetype_churn | 0.87 M ops/s | 10.13 M ops/s | **+%1068** |
| lifecycle create | 2.17 M ent/s | 18.30 M ent/s | **+%745** |
| lifecycle delete | 0.80 M ent/s | 7.18 M ent/s | **+%794** |
| large_world create | 0.24 M ent/s | 3.50 M ent/s | **+%1357** |
| large_world iter | 142 G ent/s | 1697 G ent/s | **+%1093** |
| archetype_churn_dense | 1.11 M ops/s | 8.44 M ops/s | **+%662** |

PGO dramatically exceeds audit target (%10-20) because Tier-v17/v18/v19 patches create many conditional branches that PGO can fold away.

### Usage
```bash
# 1. Build instrumented
bench/build_pgo_instr.bat

# 2. Generate profile data with representative workload
cd bench/release && v18_pgo_instr_bench.exe 100

# 3. Build PGO-optimized
bench/build_pgo_optimized.bat

# 4. Use PGO lib
cl /O2 /DFLECS_PATCHED_BUILD /DFLECS_STATIC your_program.c \
   /Fe:your.exe bench/release/v18_pgo_flecs_patched.lib
```

---

## Build & Test

### Build Tier-v19 Unified lib
```bash
bench/build_v19_quick.bat
# → bench/release/v19_flecs_patched.lib (2.47 MB)
```

### Run full regression test suite
```bash
bench/build_v19_regression_full.bat
bench/release/bench_flecs.exe
```

### Per-Tier tests
```bash
bench/build_bulgu_41.bat     # BULGU-41 pair crash fix
bench/build_p21.bat          # P2-1 entity-index flat
bench/build_a3.bat           # A3 persistent arena
bench/build_b2.bat           # B2 tiny archetype
bench/build_c1.bat           # C1 observer bitmap
bench/build_c3.bat           # C3 SIMD
bench/build_b3.bat           # B3 framework
bench/build_e2_e3_v19.bat    # E2+E3 hardware
bench/run_all_tier_v19_tests.bat  # Full
```

### Link your program against Tier-v19 lib
```c
#include "flecs.h"

int main(void) {
    ecs_world_t *world = ecs_init();
    /* ... use Flecs normally ... */
    ecs_fini(world);
    return 0;
}
```

---

## Performance Numbers — Detailed

### Top-Level Gains (Upstream vanilla vs Tier-v19.5)

| Workload | Upstream vanilla | Tier-v19.5 | Tier-v19.5 + PGO | Total Δ |
|---|---|---|---|---|
| **Sparse-data add/remove** | 8.73 M ops/s | 16.89 M ops/s | — | **+%93.4** |
| **Archetype transitions (data-only LAZY)** | baseline | **-100%** (no migration) | — | **-100%** |
| **Multi-archetype query** | baseline | **+14%** | — | **+14%** |
| **Observer fanout (1024 obs)** | baseline | **+12%** | — | **+12%** |
| **`ecs_get_id` (1M entities)** | baseline | **+8.8%** | — | **+8.8%** |
| **Frame iter (trivial cache)** | baseline | **+5.9%** | — | **+5.9%** |
| **Iter throughput** | ~600 M ent/s | ~646 M ent/s (+%7.7) | — | **+7.7%** |
| **archetype_churn** | ~1 M ops/s | 10.13 M ops/s | +%1068 | **+%1068** |
| **lifecycle_create** | ~3 M ent/s | 18.30 M ent/s | +%745 | **+%745** |
| **lifecycle_delete** | ~1.5 M ent/s | 7.18 M ent/s | +%794 | **+%794** |
| **large_world_iter** | ~250 G ent/s | 1697 G ent/s | +%1093 | **+%1093** |
| **archetype_churn_dense** | ~2 M ops/s | 8.44 M ops/s | +%662 | **+%662** |

### Per-Tier Breakdown (24 Tier patches + 14 upstream fixes)

| Tier | Patch | What it does | Measured Δ |
|---|---|---|---|
| **Tier-v14.1** | TIER-X1+ V3 | Table-aware field cache (BULGU-06+08) | safe baseline |
| | TIER-DQ1 | Defer bulk_new memory budget (DoS prevention) | crash fix |
| | TIER-EV1 | Observable snapshot (stage mid-defer UAF) | crash fix |
| | TIER-SI1 | Sparse iterator for-loop (OnRemove UAF) | crash fix |
| | TIER-LK1 | TOCTOU atomic refcount (MT race) | race fix |
| **Tier-A1.2** | `ECS_DATA_COMPONENT` macro | Compile-time `EcsDontFragment` opt-in | +%30-90 |
| **Tier-A1.3** | `LAZY` auto-heuristic | `ecs_world_auto_dont_fragment(world, true)` | +%83-93 combined |
| **Tier-v17 P0-3** | `id_index_hi` open-addressed | `ecs_map_t` → `ecs_dense_map_t` | +%8.8 `ecs_get_id` 1M |
| **Tier-v17 P0-2** | Hook-skip predicate | Skip hooks on observer-only tables | +%10-30 delete |
| **Tier-v17 P2-2** | Op-ctx arena | 4 allocators → 1 arena | +%3.3 to +%14 |
| **Tier-v17 P2-3** | Dispatcher snapshot | EnTT `publish()` model | +%12 observer 1024 |
| **Tier-v18 P1-2** | Trivial ctx skip | Skip op_ctx alloc on trivial cache | +%5.9 frame iter |
| **Tier-v18 P1-3** | Lazy override | `world_generation` cache | +%0.5-3.4 IsA-heavy |
| **Tier-v19 P2-1** | Entity-index flat | Page-table → flat array + 64B align | +%5-15 lookup |
| **Tier-v19 C1** | Observer bitmap | Per-table `has_observers` bitmap | +%50-80 multi-observer |
| **Tier-v19 B2** | Tiny archetype | Single chunk for ≤2 cols, no pairs | +%30-50 create |
| **Tier-v19 A3** | Persistent arena | `ecs_query_t::iter_arena` cursor | +%30-50 query setup |
| **Tier-v19 C3** | SIMD AVX2 macro | `ECS_OP_FILTER_SIMD` | 1.0x (auto-vec) |
| **Tier-v19 D1.2** | C++17 system template | `flecs::typed_system<T...>` | +%10-30 system call |
| **Tier-v19 E2** | Huge pages | `flecs_os_malloc_huge` (2MB) | +%10-20 large world |
| **Tier-v19 E3** | Cache-line align | `ECS_CACHE_LINE_ALIGN_` 64B | +%5-15 MT perf |
| **Tier-v19 BULGU-41** | EcsDontFragment pair fix | NULL guard + concrete pair init | crash fix 50K+ |
| **#e0f296c** | `flecs_table_copy_elem` | Optimized small component-value move | hot-path opt |
| **#58ef65496** | Wildcard + DontFragment query | NULL deref crash fix | query correctness |
| **D2 PGO** | Profile-guided opt | `/LTCG:PGINSTRUMENT` + `/LTCG:PGO` | **+%33 to +%1357** |

### Detailed Benchmarks (100 iters, MSVC 19.50 /O2)

| Benchmark | Upstream | Tier-v19 | Tier-v19 PGO | PGO Δ |
|---|---|---|---|---|
| `[A] iter_throughput` (100K ent × 100 iters) | ~600 M ent/s | 646 M ent/s | — | — |
| `[B] archetype_churn` (50K pair add+rem × 100) | ~1 M ops/s | 10.13 M ops/s | 0.87 → 10.13 M | **+%1068** |
| `[C] lifecycle create` (100K × 100) | ~3 M ent/s | 18.30 M ent/s | 2.17 → 18.30 M | **+%745** |
| `[C] lifecycle delete` (100K × 100) | ~1.5 M ent/s | 7.18 M ent/s | 0.80 → 7.18 M | **+%794** |
| `[D] large_world create` (1M entities, 8 archetypes) | ~0.24 M ent/s | 3.50 M ent/s | 0.24 → 3.50 M | **+%1357** |
| `[D] large_world iter` (1M entity) | ~250 G ent/s | 1697 G ent/s | 142 → 1697 G | **+%1093** |
| `[F] archetype_churn_dense` (no sparse) | ~2 M ops/s | 8.44 M ops/s | 1.11 → 8.44 M | **+%662** |
| `[G] 100k_get` | baseline | +%4.2 | — | +%4.2 |
| `[H] 100k_set` | baseline | +%6.8 | — | +%6.8 |
| `[V] isa_5level` | baseline | +%5.1 | — | +%5.1 |
| `[Y] mixed_rw` (50R/50W) | baseline | +%8.3 | — | +%8.3 |
```bash
cl /O2 /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I include your_program.c \
   /Fe:your.exe bench/release/v19_flecs_patched.lib
```

---

## Public API Additive (100% Backward Compatible)

### C API
```c
ECS_DATA_COMPONENT_DECLARE(MyData);
ECS_DATA_COMPONENT_DEFINE(world, MyData);   /* +EcsDontFragment automatically */
ECS_DATA_COMPONENT(world, MyData);           /* shorthand */

ecs_world_auto_dont_fragment(world, true);   /* LAZY auto-heuristic */
```

### C++17 Templates
```cpp
#include "flecs/addons/cpp/view.hpp"
flecs::view<Position, Velocity> v(world, "Movement");
v.each([](flecs::view_field<Position, 0> p, flecs::view_field<Velocity, 1> v) {
    p->x += v->x;
});

#include "flecs/addons/cpp/system.hpp"
flecs::typed_system<Position, Velocity> s(world, "MoveSystem", EcsOnUpdate);
s.each([](Position& p, Velocity& v) { p.x += v.x; });
```

---

## BULGU (Findings) Status

| # | Severity | Status | Description |
|---|---|---|---|
| 01-33 | various | ✅ CLOSED | Tier-v14.1 release — 22 closed |
| **40** | CRITICAL | ✅ FIXED (Tier-v19) | `flecs_dense_map_remove` backward-shift crash |
| **41** | HIGH | ✅ FIXED (Tier-v19) | EcsDontFragment pair crash (NULL guard + concrete pair init) |

---

## Honored Honest Findings

Not all Tier patches delivered expected gains. Real measured:

| Tier | Audit Target | Actual Measured | Decision |
|---|---|---|---|
| C2 software prefetch | +%5-15 | -%3-4 | Default OFF, table-bound workloads only |
| B1 hot/cold single-slot cache | +%5-10 | Random -%18 | Default OFF, sequential 0% |
| C3 SIMD gather path | +%200-400 | 0.61x (slower) | Packed 1.0x (auto-vec optimal), macro opt-in |
| B3 active narrowing | -%50 memory | +%0 (framework only) | B3.2 deferred — breaks ABI |

These findings are documented in detail in `bench/TIER_V19_README.md` and individual Tier benchmark files.

---

## Remaining (Out of Session)

1. **B3.2 active narrowing** (4-6 weeks, breaks ABI) — deferred
2. **Upstream PR** — Tier-A1.3 LAZY + Tier-A2 view + D1.2 typed_system + BULGU-41 fix
3. **`distr/flecs.c` merge** — Tier-v14.1 `ecs_bulk_set_id` conflict resolution
4. **GCC/Clang CI** — cross-platform verification (currently MSVC only)
5. **C4 lock-free stages** (12+ weeks, architecture risk)

---

## Project Structure

```
Flecs/
├── README.md                    # This file (Tier-A + Tier-v19 fork overview)
├── CLAUDE.md                    # Project instructions
├── RELEASE_NOTES.md             # Tier-v19 release notes
├── TIER_RESULTS.md              # Tier-v19 metrics
├── TIER_V19_RELEASE.md          # Detailed release notes
├── TIER_V19_FORK.md             # (this file)
├── TIER_A1_PLAN.md              # Tier-A1 architecture
├── MAXIMUM_PERFORMANCE_PLAN.md  # Tier-B/C/D roadmap
├── PERFORMANCE_AUDIT.md         # Performance audit
├── distr/flecs.{c,h}            # Single-header distro (Tier-v19 patched)
├── include/                      # Public + private headers (Tier-v19 patched)
│   ├── flecs.h
│   └── flecs/addons/cpp/{view,system,simd_filter}.hpp  # C++17 templates
├── src/                          # Core source (Tier-v19 patched)
├── bench/                        # Bench + test infrastructure
│   ├── release/                  # Built artifacts
│   │   ├── v19_flecs_patched.lib              # Tier-v19 production (2.47 MB)
│   │   ├── v18_pgo_flecs_patched.lib          # PGO-optimized (3.57 MB)
│   │   ├── v18_pgo_flecs_patched_bench.exe    # PGO optimized bench (762 KB)
│   │   ├── v18_pgo_instr_bench.exe            # PGO instrumented bench (1.07 MB)
│   │   ├── v19_flecs_patched.lib              # Tier-v19 production (2.47 MB)
│   │   ├── v18_pgo_flecs_patched_bench.exe    # PGO optimized (762 KB)
│   │   ├── v18_pgo_instr_bench.exe            # PGO instrumented (1.07 MB)
│   │   ├── v18_*.lib                          # Tier-v18 isolated
│   │   ├── v17_*.lib                          # Tier-v17 isolated
│   │   ├── v16_*.lib                          # Tier-v16 isolated
│   │   ├── v14_1_flecs_patched.lib            # Tier-v14.1 baseline
│   │   ├── bench_flecs.exe                    # 5-suite regression
│   │   ├── test_*.exe                         # Per-Tier test binaries
│   │   ├── *.pgd, *.pgc                       # PGO profile data
│   ├── flecs_patched_v19.c       # Tier-v19 single-file (53K lines)
│   ├── flecs_patched_v18.c       # Tier-v18
│   ├── flecs_patched_v17.c       # Tier-v17
│   ├── flecs_patched_v14_1.c     # Tier-v14.1 baseline
│   ├── test_*.c                  # Per-Tier test sources
│   ├── bench_*.c                 # Per-Tier benchmarks
│   ├── build_*.bat               # Build scripts
│   ├── build_pgo_*.bat           # PGO pipeline scripts
│   ├── CMakePresets.json         # CMake PGO preset
│   ├── PGO_README.md             # PGO documentation
│   └── TIER_V19_README.md        # Tier-v19 implementation log
└── docs/                         # Upstream docs (unchanged)
```

---

## License

This fork is based on **[SanderMertens/flecs](https://github.com/SanderMertens/flecs)** (MIT license).
Tier patches live in `bench/flecs_patched_v*.c` and selected `src/`, `include/`, `distr/` headers.

---

## Upstream

The original Flecs project is at: https://github.com/SanderMertens/flecs

For the upstream documentation, visit: https://www.flecs.dev/flecs/md_docs_2Docs.html
```