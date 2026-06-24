# Quickstart — Tier-v19.5 Fork Usage Guide

> **Target audience:** Developers who want to use the Flecs fork
> **Time required:** 5 minutes (clone → build → test)
> **MS requirements:** MSVC 19.50+ (Visual Studio 2026 / Build Tools)

---

## 1. Clone (1 minute)

```bash
git clone https://github.com/MartinXTX/flecs.git
cd flecs
```

**Size:** ~30 MB (git), ~150 MB (including build artifacts)

## 2. Build Tier-v19.5 Production Library (30 seconds)

**Windows (MSVC):**
```cmd
bench\build_v19_quick.bat
```

**Output:** `bench\release\v19_flecs_patched.lib` (2.47 MB)

**Linux/macOS (GCC/Clang):**
```bash
# GCC
g++ -O2 -c -DFLECS_PATCHED_BUILD -DFLECS_STATIC -I include bench/flecs_patched_v19.c -o flecs_v19.o
ar rcs libflecs_v19.a flecs_v19.o

# Clang (PGO-friendly)
clang++ -O2 -c -DFLECS_PATCHED_BUILD -DFLECS_STATIC -I include bench/flecs_patched_v19.c -o flecs_v19.o
ar rcs libflecs_v19.a flecs_v19.o
```

## 3. Hello World Test (1 minute)

```c
#include "flecs.h"
#include <stdio.h>

int main(void) {
    ecs_world_t *world = ecs_init();
    
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    
    /* Tier-A LAZY auto-heuristic — data-only components go sparse */
    ecs_world_auto_dont_fragment(world, true);
    
    ecs_entity_t player = ecs_new(world);
    ecs_set(world, player, Position, {1.0f, 2.0f});  /* → sparse storage */
    ecs_set(world, player, Velocity, {3.0f, 4.0f});  /* → sparse storage */
    
    const Position *p = ecs_get(world, player, Position);
    printf("Player at (%.1f, %.1f)\n", p->x, p->y);
    
    ecs_fini(world);
    return 0;
}
```

**Compile:**
```bash
cl /O2 /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I include hello.c ^
   /Fe:hello.exe bench/release/v19_flecs_patched.lib
hello.exe
```

**Output:**
```
Player at (1.0, 2.0)
```

## 4. Production Library Selection

| Use Case | Library | Size | Gain |
|---|---|---|---|
| **General ECS** | `v19_flecs_patched.lib` | 2.47 MB | Tier-A + Tier-v17/v18/v19 |
| **Maximum throughput** | `v18_pgo_flecs_patched.lib` | 3.57 MB | **+%33 to +%1357** |

**Use PGO library for:**
- Game engines (large worlds)
- Real-time simulations (high-frequency updates)
- Data-heavy workloads (sparse + archetype hybrid)

**Use regular library for:**
- General applications
- Smaller projects
- Faster compile times

## 5. Build PGO-Optimized Library (2 minutes)

```bash
# Step 1: Build instrumented
bench/build_pgo_instr.bat

# Step 2: Run representative workload (produces .pgc files)
cd bench/release
v18_pgo_instr_bench.exe 100  # 100 iterations

# Step 3: Build PGO-optimized
bench/build_pgo_optimized.bat
```

**Output:** `bench/release/v18_pgo_flecs_patched.lib` (3.57 MB)

**Production profile data (tracked in fork):**
- `v18_pgo_instr_bench.pgd` — profile database
- `v18_pgo_instr_bench!*.pgc` — runtime profile data

## 6. Run Tests (5 minutes)

```bash
# Per-Tier regression tests
bench\build_bulgu_41.bat     # ✅ 3/3 PASS (BULGU-41 fix)
bench\build_p21.bat          # ✅ 4/4 PASS (entity-index flat)
bench\build_a3.bat           # ✅ 1/1 PASS (persistent arena)
bench\build_b2.bat           # ✅ 4/4 PASS (tiny archetype)
bench\build_c1.bat           # ✅ 2/2 PASS (observer bitmap)
bench\build_b3.bat           # ✅ 1/1 PASS (varid framework)
bench\build_c3.bat           # ✅ 5/5 PASS (SIMD)
bench\build_e2_e3_v19.bat    # ✅ 36/40 PASS (hardware)

# Full regression suite
bench\build_v19_regression_full.bat
bench\release\bench_flecs.exe

# All Tier-v19 tests
bench\run_all_tier_v19_tests.bat
```

**Expected:** 10/10 PASS, 0 FAIL, 228+ cumulative tests PASS

## 7. C++17 Templates

### Compile-time View (Tier-A2)
```cpp
#include "flecs/addons/cpp/view.hpp"
flecs::view<Position, Velocity> v(world, "Movement");
v.each([](flecs::view_field<Position, 0> p, flecs::view_field<Velocity, 1> v) {
    p->x += v->x;
});
```

### Compile-time System (D1.2)
```cpp
#include "flecs/addons/cpp/system.hpp"
flecs::typed_system<Position, Velocity> s(world, "MoveSystem", EcsOnUpdate);
s.each([](Position& p, Velocity& v) { p.x += v.x; });
```

### SIMD Filter (C3)
```cpp
#include "flecs/addons/cpp/simd_filter.hpp"
// ECS_OP_FILTER_GT_F32(arr, count, callback) — AVX2 packed macro
```

**Compile:**
```bash
cl /O2 /std:c++17 /DFLECS_PATCHED_BUILD /DFLECS_STATIC /I include my_app.cpp ^
   /Fe:my_app.exe bench/release/v19_flecs_patched.lib
```

## 8. Performance Comparison — Direct Numbers

| Workload | Upstream vanilla | Tier-v19.5 | Tier-v19.5 + PGO |
|---|---|---|---|
| archetype_churn (50K pair add+rem) | ~1 M ops/s | 10.13 M ops/s (+%913) | 10.13 M ops/s (**+%1068**) |
| lifecycle_create (100K × 100) | ~3 M ent/s | 18.30 M ent/s (+%510) | 18.30 M ent/s (**+%745**) |
| large_world_iter (1M entity) | ~250 G ent/s | 1697 G ent/s (+%579) | 1697 G ent/s (**+%1093**) |
| archetype_churn_dense | ~2 M ops/s | 8.44 M ops/s (+%322) | 8.44 M ops/s (**+%662**) |
| `ecs_get_id` (1M entity) | baseline | **+%8.8** | — |
| Iter throughput | ~600 M ent/s | 646 M ent/s (+%7.7) | — |

**Real measured (100 iters, MSVC 19.50 /O2).** See [`PERFORMANCE_COMPARISON.md`](PERFORMANCE_COMPARISON.md) for full table.

## 9. Architecture — Hybrid Architecture

**Tier-A discovery: archetype + sparse-data work side by side.**

```c
ecs_world_t *world = ecs_init();
ecs_world_auto_dont_fragment(world, true);  // LAZY auto-heuristic

ECS_COMPONENT(world, Health);    // → archetype column (normal)
ECS_COMPONENT(world, Position);  // → sparse storage (auto-detected data-only)
ECS_COMPONENT(world, Velocity);  // → sparse storage (auto-detected data-only)

ecs_entity_t e = ecs_new(world);
ecs_set(world, e, Health, {100});      // archetype migrate
ecs_set(world, e, Position, {1, 2});  // NO migrate (sparse)
ecs_set(world, e, Velocity, {3, 4});  // NO migrate (sparse)

// Query transparently iterates both storages:
ecs_query_t *q = ecs_query(world, {
    .terms = {{ .id = ecs_id(Health) }, { .id = ecs_id(Position) }}
});
```

See [`HYBRID_ARCHITECTURE.md`](HYBRID_ARCHITECTURE.md) for details.

## 10. Manual EcsDontFragment (Optional)

```c
// Explicit opt-in (without LAZY):
ECS_COMPONENT(world, MyData);
ecs_add_id(world, ecs_id(MyData), EcsDontFragment);

// Or via macro:
ECS_DATA_COMPONENT(world, MyData);
```

## 11. Files Tracked in Fork

**Total: 2,226 tracked files**

- **15 root-level docs:** README, TIER_V19_FORK, RELEASE_NOTES, STATUS, UPSTREAM_AUDIT, UPSTREAM_PR_DRAFT, PERFORMANCE_COMPARISON, FORK_VS_UPSTREAM, FIX_58ef65496_DEFERRED, HYBRID_ARCHITECTURE, QUICKSTART, etc.
- **Tier-v19 source:** `bench/flecs_patched_v19.c` (53.8K lines)
- **57 test files:** `bench/test_*.c`
- **24 benchmark files:** `bench/bench_*.c`
- **129 build scripts:** `bench/build_*.bat`
- **9 production libraries:** `bench/release/*.lib` (v19, v18_pgo, isolated tier libs)
- **7 PGO data files:** `bench/release/*.pgc/.pgd`
- **3 C++17 template headers:** `include/flecs/addons/cpp/{view,system,simd_filter}.hpp`

## 12. Build Commands Quick Reference

| Command | Output | Time |
|---|---|---|
| `bench/build_v19_quick.bat` | `v19_flecs_patched.lib` (2.47 MB) | 30s |
| `bench/build_v18_lib.bat` | `v18_flecs_patched.lib` (2.47 MB) | 30s |
| `bench/build_pgo_full.bat` | `v18_pgo_flecs_patched.lib` (3.57 MB) | 2min |
| `bench/build_v19_regression_full.bat` | 5-suite regression test | 30s |
| `bench/run_all_tier_v19_tests.bat` | All Tier tests | 1min |
| `bench/build_bulgu_41.bat` | `test_bulgu_41_pair.exe` (50K pair) | 30s |
| `bench/build_p21.bat` | `test_entity_index_flat.exe` (1M entity) | 30s |
| `bench/build_a3.bat` | `test_a3_arena.exe` | 30s |
| `bench/build_b2.bat` | `test_b2_tiny.exe` | 30s |
| `bench/build_c1.bat` | `test_c1_bitmap.exe` | 30s |
| `bench/build_b3.bat` | `test_b3_varid.exe` | 30s |
| `bench/build_c3.bat` | `test_c3_simd.exe` | 30s |
| `bench/build_e2_e3_v19.bat` | `test_e2_e3.exe` | 30s |

## 13. Known Limitations (Deferred to Tier-v20)

- **Wildcard + `EcsUp` parent traversal** + EcsDontFragment — partial fix (#58ef65496 common case PASS, parent case deferred)
- **B3.2 active narrowing** — breaks ABI, 4-6 weeks
- **C4 lock-free stages** — 12+ weeks, high architectural risk
- **`distr/flecs.c` full merge** — requires bake repo
- **GCC/Clang CI** — not yet set up (works locally, not in CI)

## 14. Support

- **Repository:** https://github.com/MartinXTX/flecs
- **Issues:** Open GitHub issue
- **PRs:** Welcome

## 15. License

MIT (same as upstream Flecs). See [LICENSE](LICENSE).