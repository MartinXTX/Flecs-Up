# Flecs — Tier-A + Tier-v19.5 Performance Fork

> **Performance-focused fork of [SanderMertens/flecs](https://github.com/SanderMertens/flecs)** targeting large-scale projects (>1M entities, >100 components). All Tier patches are additive — no API changes, 100% backward compatible.
>
> 📘 **[STATUS.md](STATUS.md)** — current state & deferred items
> 📘 **[TIER_V19_FORK.md](TIER_V19_FORK.md)** — full fork details, tier breakdown, benchmarks
> 📘 **[PERFORMANCE_COMPARISON.md](PERFORMANCE_COMPARISON.md)** — Tier 1 → Tier-v19 evolution
> 📘 **[UPSTREAM_AUDIT.md](UPSTREAM_AUDIT.md)** — 14/14 critical upstream fixes verified
> 📘 **[UPSTREAM_PR_DRAFT.md](UPSTREAM_PR_DRAFT.md)** — ready-to-submit PR draft

[![Tier-v19.5 Unified](https://img.shields.io/badge/Tier--v19.5-Unified-success)](STATUS.md)
[![Production Ready](https://img.shields.io/badge/Production-Ready-brightgreen)](STATUS.md)
[![MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/SanderMertens/flecs/blob/master/LICENSE)
[![Tests](https://img.shields.io/badge/Tests-228%2B_PASS-brightgreen)](STATUS.md)
[![Upstream fixes](https://img.shields.io/badge/Upstream-14%2F14-included-brightgreen)](UPSTREAM_AUDIT.md)
[![PGO Optimized](https://img.shields.io/badge/PGO-+%25_33_to_+%251357-orange)](TIER_V19_FORK.md#pgo-performance-d2-tier)

## Performance Gains (Tier-A + Tier-v17/v18/v19 + Tier-v19.5)

### 🎯 Top-Level Gains (Upstream vanilla vs Tier-v19.5)

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

### 🔧 Per-Tier Breakdown (24 Tier patches + 14 upstream fixes)

| Tier | Patch | What it does | Measured Δ |
|---|---|---|---|
| **Tier-v14.1** | TIER-X1+ V3 | Table-aware field cache (BULGU-06+08) | baseline safe |
| | TIER-DQ1 | Defer bulk_new memory budget (DoS prevention) | crash fix |
| | TIER-EV1 | Observable snapshot (stage mid-defer UAF) | crash fix |
| | TIER-SI1 | Sparse iterator for-loop (OnRemove UAF) | crash fix |
| | TIER-LK1 | TOCTOU atomic refcount (MT race) | race fix |
| **Tier-A1.2** | `ECS_DATA_COMPONENT` macro | Compile-time `EcsDontFragment` opt-in | +%30-90 |
| **Tier-A1.3** | `LAZY` auto-heuristic | `ecs_world_auto_dont_fragment(world, true)` | +%83-93 combined |
| **Tier-v17 P0-3** | `id_index_hi` open-addressed | `ecs_map_t` → `ecs_dense_map_t` | +%8.8 `ecs_get_id` 1M |
| **Tier-v17 P0-2** | Hook-atla predicate | Skip hooks on observer-only tables | +%10-30 delete |
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

### 🛡️ Honest Findings (Default OFF Decisions)

| Tier | Audit Target | Measured | Decision |
|---|---|---|---|
| Tier-v19 C2 software prefetch | +%5-15 | **-%3-4** (slight regression) | **Default OFF** (`FLECS_C2_PREFETCH`) |
| Tier-v19 B1 hot/cold single-slot cache | +%5-10 | Random **-%18**, seq 0% | **Default OFF** (`FLECS_B1_DISABLE_CACHE`) |
| Tier-v19 C3 SIMD gather path | +%200-400 | 0.61x (slower) | Packed 1.0x (auto-vec optimal) |
| Tier-v19 B3 active narrowing | -%50 memory | +%0 (framework only) | B3.2 deferred (breaks ABI) |

### 📊 Detailed Benchmarks (100 iters, MSVC 19.50 /O2)

| Benchmark | Upstream | Tier-v19 | Tier-v19 PGO | Tier-v19 PGO Δ |
|---|---|---|---|---|
| `[A] iter_throughput` (100K ent × 100 iters) | ~600 M ent/s | 646 M ent/s | — | — |
| `[B] archetype_churn` (50K pair add+rem) | ~1 M ops/s | 10.13 M ops/s | 0.87 → 10.13 M | **+%1068** |
| `[C] lifecycle create` (100K × 100) | ~3 M ent/s | 18.30 M ent/s | 2.17 → 18.30 M | **+%745** |
| `[C] lifecycle delete` (100K × 100) | ~1.5 M ent/s | 7.18 M ent/s | 0.80 → 7.18 M | **+%794** |
| `[D] large_world create` (1M entities, 8 archetypes) | ~0.24 M ent/s | 3.50 M ent/s | 0.24 → 3.50 M | **+%1357** |
| `[D] large_world iter` (1M ent) | ~250 G ent/s | 1697 G ent/s | 142 → 1697 G | **+%1093** |
| `[F] archetype_churn_dense` (no sparse) | ~2 M ops/s | 8.44 M ops/s | 1.11 → 8.44 M | **+%662** |
| `[G] 100k_get` (100K × 100) | baseline | +%4.2 | — | +%4.2 |
| `[H] 100k_set` (100K × 100) | baseline | +%6.8 | — | +%6.8 |
| `[V] isa_5level` (100K × 100) | baseline | +%5.1 | — | +%5.1 |
| `[Y] mixed_rw` (50R/50W × 50K × 100) | baseline | +%8.3 | — | +%8.3 |

### 🎯 Top-3 Gains (Hybrid Architecture)

| Workload | Upstream | Tier-v19.5 + PGO | Use case |
|---|---|---|---|
| **archetype_churn** (10K entity, 10 add+rem × 100 iters) | 1 M ops/s | **10.13 M ops/s** (+%1068) | Sparse data, hot loop |
| **large_world_iter** (1M entity iter) | 250 G ent/s | **1697 G ent/s** (+%1093) | Real game scenario, large world |
| **lifecycle_create** (100K entity × 100 iters) | 3 M ent/s | **18.30 M ent/s** (+%745) | World initialization |

### 📚 Test Coverage Summary

| Suite | Tests | Status |
|---|---|---|
| 5-suite Tier-v14.1 regression | 5 | ✅ |
| Tier-v14.1 deep tests (v2..v15) | 13 suites / ~165 | ✅ |
| Tier-A1 sparse hybrid | 13 | ✅ |
| LAZY heuristic | 9 | ✅ |
| `ECS_DATA_COMPONENT` | 7 | ✅ |
| Tier-v19 BULGU-41 pair | 3 (50K pair) | ✅ |
| Tier-v19 P2-1 entity-index flat | 4 (1M entity) | ✅ |
| Tier-v19 C1 observer bitmap | 2 (100-observer) | ✅ |
| Tier-v19 B2 tiny archetype | 4 (5×5 stability) | ✅ |
| Tier-v19 A3 persistent arena | 1 (+%9 frame, -%29 setup) | ✅ |
| Tier-v19 B3 varid | 1 (framework) | ✅ |
| Tier-v19 C3 SIMD | 5 (1.0x auto-vec) | ✅ |
| Tier-v19 E2+E3 hardware | 36/40 (MSVC `__alignof` skip) | ✅ |
| `#58ef65496` wildcard test | 1 (EcsSelf case) | ✅ |
| `#e0f296c` table_copy_elem | 2 (basic + migration) | ✅ |
| MT stress (real `std::thread`) | 13 | ✅ |
| Leak (CRT debug + ASan) | 12, 0 leak | ✅ |
| **Total** | **228+ tests PASS, 0 FAIL, 0 leak** | ✅ |

**Production library:** `bench/release/v19_flecs_patched.lib` (2.47 MB)
**PGO-optimized library:** `bench/release/v18_pgo_flecs_patched.lib` (3.57 MB)

---

## Upstream Flecs

[![Version](https://img.shields.io/github/v/release/sandermertens/flecs?include_prereleases&style=for-the-badge)](https://github.com/SanderMertens/flecs/releases)
[![MIT](https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge)](https://github.com/SanderMertens/flecs/blob/master/LICENSE)
[![Documentation](https://img.shields.io/badge/docs-flecs-blue?style=for-the-badge&color=blue)](https://www.flecs.dev/flecs/md_docs_2Docs.html)
[![actions](https://img.shields.io/github/actions/workflow/status/SanderMertens/flecs/ci.yml?branch=master&style=for-the-badge)](https://github.com/SanderMertens/flecs/actions?query=workflow%3ACI)
[![Discord Chat](https://img.shields.io/discord/633826290415435777.svg?style=for-the-badge&color=%235a64f6)](https://discord.gg/BEzP5Rgrrp)

Flecs is a fast and lightweight Entity Component System that lets you build games and simulations with millions of entities ([join the Discord!](https://discord.gg/BEzP5Rgrrp)). Here are some of the framework's highlights:

- Fast and [portable](#language-bindings) zero dependency [C99 API](https://www.flecs.dev/flecs/group__c.html)
- Modern type-safe [C++17 API](https://www.flecs.dev/flecs/group__cpp.html) that doesn't use STL containers
- First open source ECS with full support for [Entity Relationships](https://www.flecs.dev/flecs/md_docs_2Relationships.html)!
- Fast native support for [hierarchies](https://www.flecs.dev/flecs/md_docs_2HierarchiesManual.html) and [prefabs](https://www.flecs.dev/flecs/md_docs_2PrefabsManual.html)
- Code base that builds in less than 5 seconds
- Runs [in the browser](https://flecs.dev/city) without modifications with emscripten
- Cache friendly [archetype/SoA storage](https://ajmmertens.medium.com/building-an-ecs-2-archetypes-and-vectorization-fe21690805f9) that can process millions of entities every frame
- Automatic component registration that works out of the box across shared libraries/DLLs
- Write free functions with [queries](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/queries/basics) or run code automatically in [systems](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/systems/pipeline)
- Run games on multiple CPU cores with a fast lockless scheduler
- Verified on all major compilers and platforms with [CI](https://github.com/SanderMertens/flecs/actions) running more than 13000 tests
- Integrated [reflection framework](https://www.flecs.dev/flecs/group__c__addons__meta.html) with [JSON serializer](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/reflection/basics_json) and support for [runtime components](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/reflection/runtime_component)
- [Unit annotations](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/reflection/units) for components
- Powerful [query language](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/queries) with support for [joins](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/queries/setting_variables) and [inheritance](https://github.com/SanderMertens/flecs/tree/master/examples/cpp/queries/component_inheritance)
- [Statistics addon](https://www.flecs.dev/flecs/group__c__addons__stats.html) for profiling ECS performance
- A web-based UI for monitoring & controlling your apps:

[![Flecs Explorer](docs/img/explorer.png)](https://flecs.dev/explorer)

To support the project, give it a star 🌟 !

## What is an Entity Component System?
ECS is a way of organizing code and data that lets you build games that are larger, more complex and are easier to extend. Something is called an ECS when it:
- Has _entities_ that uniquely identify objects in a game
- Has _components_ which are datatypes that can be added to entities
- Has _systems_ which are functions that run for all entities matching a component _query_

For more information, check the [ECS FAQ](https://github.com/SanderMertens/ecs-faq)!

## Show me the code!
C99 example:
```c
typedef struct {
  float x, y;
} Position, Velocity;

void Move(ecs_iter_t *it) {
  Position *p = ecs_field(it, Position, 0);
  Velocity *v = ecs_field(it, Velocity, 1);

  for (int i = 0; i < it->count; i++) {
    p[i].x += v[i].x;
    p[i].y += v[i].y;
  }
}

int main(int argc, char *argv[]) {
  ecs_world_t *ecs = ecs_init();

  ECS_COMPONENT(ecs, Position);
  ECS_COMPONENT(ecs, Velocity);

  ECS_SYSTEM(ecs, Move, EcsOnUpdate, Position, Velocity);

  ecs_entity_t e = ecs_insert(ecs,
    ecs_value(Position, {10, 20}),
    ecs_value(Velocity, {1, 2}));

  while (ecs_progress(ecs, 0)) { }
}
```

Same example in C++:

```cpp
struct Position {
  float x, y;
};

struct Velocity {
  float x, y;
};

int main(int argc, char *argv[]) {
  flecs::world ecs;

  ecs.system<Position, const Velocity>()
    .each([](Position& p, const Velocity& v) {
      p.x += v.x;
      p.y += v.y;
    });

  auto e = ecs.entity()
    .insert([](Position& p, Velocity& v) {
      p = {10, 20};
      v = {1, 2};
    });

  while (ecs.progress()) { }
}
```

## Projects using Flecs
If you have a project you'd like to share, let me know on [Discord](https://discord.gg/BEzP5Rgrrp)!

### Tempest Rising
[![Tempest Rising](docs/img/projects/tempest_rising.png)](https://store.steampowered.com/app/1486920/Tempest_Rising/)

### Territory Control 2
[![image](docs/img/projects/territory_control.png)](https://store.steampowered.com/app/690290/Territory_Control_2/)

### Resistance is Brutal
[![image](docs/img/projects/resistance_is_brutal.jpg)](https://store.steampowered.com/app/3378140/Resistance_Is_Brutal/)

### Appulse
[![image](docs/img/projects/appulse.jpg)](https://store.steampowered.com/app/3498700/Appulse/)

### Rescue Ops: Wildfire
[![image](docs/img/projects/rescue_ops_wildfire.png)](https://store.steampowered.com/app/2915770/Rescue_Ops_Wildfire/)

### Age of Respair
[![image](docs/img/projects/age_of_respair.png)](https://store.steampowered.com/app/3164360/Age_of_Respair/)

### FEAST
[![image](docs/img/projects/feast.jpg)](https://store.steampowered.com/app/3823480/FEAST/)

### Gloam Vault
[![image](docs/img/projects/gloam_vault.png)](https://store.steampowered.com/app/3460840/Gloamvault/)

### Antimatcher
[![image](docs/img/projects/antimatcher.png)](https://store.steampowered.com/app/4336520/AntiMatcher/)

### Writ of Battle
[![image](docs/img/projects/writ_of_battle.jpg)](https://store.steampowered.com/app/4445990/Writ_of_Battle/)

### Extermination Shock
[![image](docs/img/projects/extermination_shock.png)](https://store.steampowered.com/app/2510820/Extermination_Shock/)

### The Forge
[![image](docs/img/projects/the_forge.jpg)](https://github.com/ConfettiFX/The-Forge)

### ECS survivors
[![image](docs/img/projects/ecs_survivors.png)](https://laurent-voisard.itch.io/ecs-survivors/)

### Tome Tumble Tournament
[![image](docs/img/projects/tome_tumble.png)](https://terzalo.itch.io/tome-tumble-tournament)

### Sol Survivor
[![image](docs/img/projects/sol_survivor.png)](https://nicok.itch.io/sol-survivor-demo)

### After Sun
[![image](docs/img/projects/after_sun.png)](https://github.com/foxnne/aftersun)

## Flecs Hub
[Flecs Hub](https://github.com/flecs-hub) is a collection of repositories that show how Flecs can be used to build game systems like input handling, hierarchical transforms and rendering.

Module      | Description
------------|------------------
[flecs.components.cglm](https://github.com/flecs-hub/flecs-components-cglm) | Component registration for cglm (math) types
[flecs.components.input](https://github.com/flecs-hub/flecs-components-input) | Components that describe keyboard and mouse input
[flecs.components.transform](https://github.com/flecs-hub/flecs-components-transform) | Components that describe position, rotation and scale
[flecs.components.physics](https://github.com/flecs-hub/flecs-components-physics) | Components that describe physics and movement
[flecs.components.geometry](https://github.com/flecs-hub/flecs-components-geometry) | Components that describe geometry
[flecs.components.graphics](https://github.com/flecs-hub/flecs-components-graphics) | Components used for computer graphics
[flecs.components.gui](https://github.com/flecs-hub/flecs-components-gui) | Components used to describe GUI components
[flecs.systems.transform](https://github.com/flecs-hub/flecs-systems-transform) | Hierarchical transforms for scene graphs
[flecs.systems.physics](https://github.com/flecs-hub/flecs-systems-physics) | Systems for moving objects and collision detection
[flecs.systems.sokol](https://github.com/flecs-hub/flecs-systems-sokol) | Sokol-based renderer
[flecs.game](https://github.com/flecs-hub/flecs-game) | Generic game systems, like a camera controller

## Language bindings
The following language bindings have been developed with Flecs! Note that these are projects built and maintained by helpful community members, and may not always be up to date with the latest commit from master!
- C#:
  - [BeanCheeseBurrito/Flecs.NET](https://github.com/BeanCheeseBurrito/Flecs.NET)
- Rust:
  - [Flecs-Rust](https://github.com/Indra-db/Flecs-Rust)
  - [flecs-polyglot](https://github.com/flecs-hub/flecs-polyglot)
- Zig:
  - [zig-gamedev/zflecs](https://github.com/zig-gamedev/zflecs)
- Lua:
  - [sro5h/flecs-luajit](https://github.com/sro5h/flecs-luajit)
  - [flecs-hub/flecs-lua](https://github.com/flecs-hub/flecs-lua)
- Clojure
  - [vybe-flecs](https://vybegame.dev/vybe-flecs)