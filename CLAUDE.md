# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Flecs — Performance-Focused Fork

This repository is a copy of **SanderMertens/flecs** and is configured to **reduce overhead in large-scale (>1M entities, >100 components) projects**. Goal: hot-path gains while preserving architecture.

> Companion references: `C:/Project/entt` (skypjack/entt). Inspired by EnTT's **sparse-set** and **view** model, the target is to transition Flecs to an **archetype + sparse hybrid** model.
> Comprehensive technical analysis: [`PERFORMANCE_AUDIT.md`](PERFORMANCE_AUDIT.md).

---

## Core Commands

### Build (CMake)
```bash
# Library
cmake -S . -B build -DFLECS_TESTS=OFF
cmake --build build --config Release

# With tests (requires bake repository — into `bake` subdirectory or via BAKE_DIRECTORY)
cmake -S . -B build -DFLECS_TESTS=ON -DFLECS_STATIC=ON -DFLECS_SHARED=ON
cmake --build build --config Release

# Single test (e.g. core)
cmake --build build --target test_core_flecs_static --config Release
ctest --test-dir build -R core --output-on-failure
```

### Build (Meson)
```bash
meson setup builddir -Dbuildtype=release
meson compile -C builddir
meson test -C builddir
```

### Running a single Flecs test
Test files like `test/core/src/Entity.c` are **not independently executable**; they depend on the bake test framework. For a single category:
```bash
ctest --test-dir build -R 'core|query|storage|observer'
```

### Examples
```bash
cmake -S examples/c -B build/examples -DFLECS_PATH=../..
cmake --build build/examples --config Release
```

### Code style and formatting
- C99, 4-space indent, K&R braces.
- `_typos.toml` for typo checking; run `cargo typos` or `typos` before PR.

---

## Architecture — The Big Picture

Flecs's core is built on **two main abstractions**: **world** (root scene) and **archetype** (table of entities sharing the same component set).

### Module map
| Path | Responsibility |
|---|---|
| `src/world.c` (~2145 lines) | World lifecycle, store, root table |
| `src/entity.c` (~2700 lines) | Entity create/delete, `flecs_commit`/`flecs_move_entity` |
| `src/storage/table.c` (~3002 lines) | Table (archetype) implementation: append/move/delete, hooks, override |
| `src/storage/table_graph.c` | Edge graph: archetype transitions (`traverse_add`/`remove`) |
| `src/storage/table_cache.c` | Per-component matching table cache (currently **linked list** — performance bottleneck) |
| `src/storage/component_index.c` | Global id → `ecs_component_record_t` (id lo/hi, refcount, flags) |
| `src/storage/entity_index.c` | Entity → record page-table (3 indirection) — **Tier-v19 P2-1 flat array** |
| `src/storage/sparse_storage.c` | Hybrid `EcsSparse`/`EcsDontFragment` storage — **Tier-A architecture exploration +93.4%** |
| `src/query/` | Query engine: `cache/`, `engine/`, `compiler/` |
| `src/observable.c` | Observer/signal: `EcsOnAdd`/`OnRemove`/`OnSet`/`UnSet` emit |
| `src/commands.c` | Defer/queue + bulk operation batch |
| `src/component_actions.c` | OnAdd/OnRemove/OnSet hook execution + sparse routing |
| `src/iter.c` | `ecs_iter_t` lifecycle, `ecs_field` (2 indirection fast-path) |
| `src/datastructures/` | `vec`, `map`, `hashmap`, `bitset`, `block_allocator` |
| `include/flecs.h` (~6687 lines) | Public C99 API |
| `include/flecs/addons/cpp/` | C++17 wrapper |
| `src/addons/` | Addons: pipeline, system, meta, json, rest, http, parser, script, stats |

### Hot paths
1. **`ecs_field(it, T, i)`** — `src/iter.c:106-140`. Fast-path: `it->ptrs[i]` (staging-only). Normal: `it->columns[i]` + `ECS_ELEM(table->data.columns[].data, size, it->offset)` = 2 indirection.
2. **`ecs_query_next`** — for trivial cache `cache_iter.c:127` (sequential vec); non-trivial cache `cache_iter.c:39`; uncached planned VM `engine/eval.c`.
3. **`ecs_add_id`** — `src/entity.c:449` → `flecs_table_traverse_add` (`table_graph.c`) → `flecs_commit` (`entity.c:233`) → `flecs_move_entity` (`entity.c:189`).
4. **`ecs_delete`** — `src/entity.c:1647` → cascade (`on_delete.c`) + `flecs_actions_move_remove` + sparse cleanup + table delete + entity recycle.

### Compile-time vs runtime
- **Compile-time:** `ECS_COMPONENT`, `ECS_SYSTEM`, `ECS_TAG` macros declare type-safely.
- **Runtime:** World has powerful introspection at runtime (`ecs_get_id`, observer dispatch, query DSL parse) — this is the point of divergence from EnTT.

---

## Scope of the Performance Work

The [`PERFORMANCE_AUDIT.md`](PERFORMANCE_AUDIT.md) document:
- The hottest **10 bottlenecks** (B-1 … B-10) — with file:line references
- Comparison of EnTT's **sparse-set**, **view**, **dispatcher** patterns
- **P0/P1/P2** prioritized action plan, at file + function level
- 5-week implementation order
- Behavior and API compatibility risks

**Quick summary:**
- **Biggest bottleneck:** archetype proliferation (`storage/table_graph.c:702`) + table-cache linked-list iteration (`storage/table_cache.c:84-103`).
- **Safest start:** converting `ecs_table_cache_t` to `ecs_vec_t` (P0-1). Low test risk, 2-3× iter throughput improvement.
- **Biggest gain:** Sparse-set hybrid storage (P1-1). The `EcsDontFragment` flag already exists; full EnTT-style sparse-set for non-archetype components.

---

## Important Notes

- The **`EcsWorldReadonly`** flag disables observer/hook calls; storage mutation must not occur while `world->readonly` is held.
- **Stage defer:** In multi-threaded scenarios, use the `ecs_read_write_begin`/`end` pair; the `flecs_defer_begin`/`end` flush order in `commands.c` is important.
- **`ecs_set_id` fast path:** If `world->non_trivial_set[id]` is absent, it copies directly via `ecs_os_memcpy` (`entity.c:2382-2386`).
- **Trivial-cache criteria** (`query/cache/cache.c:617-639`): all terms And/Not/Optional; no EcsUp; no EcsQueryMatchWildcards; no EcsQueryCacheWithFilter; no order_by/group_by/DetectChanges. When these criteria are met: **lowest fixed cost per query**.
- **Sparse components:** Components flagged with `EcsSparse` or `EcsDontFragment` are stored via `src/storage/sparse_storage.c` — they do not join the archetype; the hook point is `flecs_sparse_on_add_cr` (`component_actions.c:154`).

---

## Project Structure Summary

```
Flecs/
├── include/flecs.h          # public C99 API (single header)
├── include/flecs/           # private + addons headers
├── src/                     # core implementation
│   ├── world.c entity.c commands.c observable.c iter.c ...
│   ├── storage/             # table, archetype, sparse, component_index
│   ├── query/               # cache/, engine/, compiler/
│   ├── datastructures/      # vec, map, hashmap, bitset
│   └── addons/              # pipeline, system, meta, json, rest, http...
├── examples/c  examples/cpp # runnable examples
├── test/                    # bake-based unit tests (13,000+ tests)
├── cmake/  meson.build      # build systems
├── docs/                    # markdown docs
├── distr/                   # single-header distributions
└── PERFORMANCE_AUDIT.md     # performance audit and EnTT comparison
```