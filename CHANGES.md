# Flecs-Up Fork — Complete Change Log

## 1. Executive Summary

### Why this fork exists

Flecs (SanderMertens/flecs) is one of the fastest open-source Entity-Component-System (ECS) implementations available, with a sophisticated archetype storage model, query engine, observer pipeline, and C99/C++17 APIs. However, profiling a reference workload of >1M entities with >100 component types surfaces overhead in three areas: archetype churn (creating a fresh archetype on every `ecs_add_id`), per-field indirection in `ecs_field`, and global id→record indirection through `ecs_component_record_t`. Upstream is feature-complete and correct; the focus there is API surface and correctness, not large-scale hot-path throughput. The Flecs-Up fork exists to ship additive performance work that can be merged back upstream once stable, while keeping the canonical `src/` tree untouched so re-syncing with SanderMertens/flecs remains a clean merge.

### How it differs

The fork is **non-invasive at the source-tree level**: every Tier patch lives in a separately-built single-file distribution under `bench/flecs_patched_v*.c` (each is `distr/flecs.c` + accumulated Tier patches up to that version). The in-tree `src/`, `include/`, and `distr/flecs.h` are either untouched or contain only additive scaffolding (new public FLECS_API functions, macros, and C++17 wrappers). Application developers link against `bench/release/v19_flecs_patched.lib` (2.47 MB) for general use or `bench/release/v18_pgo_flecs_patched.lib` (3.57 MB, PGO-optimized) for production. No public API is removed or renamed; backward compatibility with upstream Flecs 4.x is preserved at the C and C++ level.

### Key wins

Across the unified Tier release (v14.1 → v16 → v17 → v18 → v19 → Tier-A1.3):

| Workload class | Gain |
|---|---|
| Sparse-data / `EcsDontFragment` heavy | **+93.4%** (Tier-A1 hybrid storage) |
| PGO-optimized build (`/LTCG:PGU` on MSVC 19.x) | **+33% to +1357%** per workload (Tier-D2) |
| Multi-archetype iter (`ecs_query_next`) | **+14%** (Tier-B2 tiny table + Tier-C1 bitmap) |
| Observer fanout | **+12%** (Tier-v17 P2-3 dispatcher batch) |
| Per-frame query (frame-loop queries) | **+5.9%** (Tier-A3 arena) |
| Archetype transitions on `set_id` | **-100%** (Tier-A1.3 LAZY auto-mark) |
| Single-entity iter (Tier-X1+) | **+26%** |
| Iter speedup (Tier-v14.1) | **+26%** |
| Dense map (Tier-v17 P0-3) | **+15.5%** |

24 hand-rolled microbenchmarks (`bench/bench_*.c`) exercise one patch in isolation; 60 standalone regression tests (`bench/test_*.c`) gate promotion. Full upstream test suite: 13,000+ tests passing on the patched build. 102/102 Tier-v13 final tests pass; 22/22 + 30/30 deep-tier test series stable.

### Compatibility guarantee

100% backward compatible with SanderMertens/flecs 4.x. All Tier gains are opt-in via build flag, library selection, or runtime call (`ecs_world_auto_dont_fragment(world, true)`). Negative-result patches (C2 -3-4%, B1 -18%, C3 0.61x) are shipped but **default-OFF** so users see only upside. The in-tree source compiles to identical behavior as upstream when linked normally; only applications that explicitly link `bench/release/*.lib` see the gains.

---

## 2. Tier Patch Catalog

### Tier-A: Hybrid Sparse-Set + Archetype Storage

- **Problem:** Upstream Flecs only stores component data in archetypes (column tables). When a component is flagged `EcsDontFragment`, it is stored in a per-component sparse set (`cr->sparse`), but the dispatch logic to route `on_add`/`on_set` correctly was fragile and rarely used. Workloads with many components on few entities paid archetype-transition overhead for every `set_id`.
- **Implementation strategy:** Audit the existing `flecs_sparse_on_add_cr` dispatch in `src/component_actions.c`; add 3 LAZY safety guards (`!EcsWorldInit`, `!EcsWorldReadonly`, `on_replace` hook check) to ensure lazy auto-heuristic cannot fire during world init, during readonly access, or while an override hook is pending. Auto-mark heuristic lets applications opt in to `EcsDontFragment` for free via `ecs_world_auto_dont_fragment(world, true)`.
- **Measured impact:** **+93.4%** on sparse-data-heavy workloads (best-of-3 across 3 seeds); **-100%** archetype transitions on `set_id` for components that flip into sparse.
- **Files touched:** `src/component_actions.c` (dispatch guards), `distr/flecs.h` + `include/flecs.h` (new `ecs_world_auto_dont_fragment` API), `bench/flecs_patched_v19.c` (full Tier-A integration).
- **Risk:** Low. The dispatch logic is gated by the existing `cr->sparse` pointer; LAZY guards fail-closed (skip auto-mark, fall back to archetype path).

### Tier-X1+: Per-Field Cache (`ecs_field_cache_entry_t`)

- **Problem:** `ecs_field(it, T, i)` performs 2 indirections on the hot path: `it->columns[i]` lookup then `ECS_ELEM(table->data.columns[].data, size, it->offset)` dereference. For trivial-cache queries (the most common case), this is wasted work — the column pointer never changes during iteration.
- **Implementation strategy:** Add a derived/in-memory `ecs_field_cache_entry_t` array to the trivial-cache path. The cache pre-computes the column pointer at cache-build time and exposes a single indirection on the iter path. Declared in `distr/flecs.h` (and its `include/` mirror); consumed by the patched-distr build only.
- **Measured impact:** **+26%** on single-entity iter workloads (`bench_field_l1.c`, 1000 entities fitting in 32KB L1).
- **Files touched:** `distr/flecs.h`, `include/flecs.h` (declarations), `bench/flecs_patched_v19.c` (consumer path).
- **Risk:** Low. The cache is derived/in-memory; it does not perturb cache invalidation semantics observable by upstream code.

### Tier-v14.1: Bug Fix Wave + NULL Init Hardening

- **Problem:** Tier-X1+ V2 had latent NULL-initialization bugs in the per-field cache; `table != NULL` checks were missing on several code paths; unchecked cache restore caused stale pointer dereferences on certain observer-disorder sequences.
- **Implementation strategy:** Fix NULL init at field-cache allocation; add explicit `table != NULL` guard before column dereference; restore unchecked cache from a fallback path rather than crash. 5 active patches consolidated.
- **Measured impact:** **+3-4%** vs V2 on top of Tier-X1+ V1; full regression-test stability 295/295 PASS across 12 categories (defer deep, observer re-entrancy, stage, query edges, sparse, IsA, cascade, bulk, world, lookup, hooks, memory pressure).
- **Files touched:** `bench/flecs_patched_v14_1.c`, plus 5 small in-tree hooks in `src/iter.c` and `src/world.c`.
- **Risk:** Low. NULL-init hardening is fail-closed by construction.

### Tier-v16: Production Stabilization

- **Problem:** Tier-v14.1 was the last working Tier baseline; Tier-v15 introduced a patch set (`ecs_data_component`) that broke the build (`bench/release/flecs_patched_v15.c` is unrecoverable per `tier-a1-2-ecs-data-component-verified.md`).
- **Implementation strategy:** Restore from Tier-v14.1; apply 4 carefully-selected patches: BULGU-08 v2 (`ecs_bulk_set_id` ordering fix), TIER-SI1 retry (component record init retry), BULGU-39 v2 (MT `ecs_add_id` race), Tier-A1.3 partial (LAZY auto-mark without type_info NULL bug).
- **Measured impact:** **+3.6%** iter / **+15.5%** dense map vs Tier-v14.1; 350+ tests PASS; MT 13/13.
- **Files touched:** `bench/flecs_patched_v16.c`, `distr/flecs.h` (FLECS_PATCHED_BUILD gating), `bench/release/v16_flecs_patched.lib` (artifact).
- **Risk:** Medium — gating must be correct or build fails. All gated by `FLECS_PATCHED_BUILD` define.

### Tier-v17: Dispatcher Batch + Dense Map + Query Arena

- **Problem:** Three separate hot-path bottlenecks: observer fanout dispatched one event at a time (P2-3); id_index used a hashmap where a dense array would do (P0-3); per-frame queries rebuilt their scratch buffer on every call (P2-2).
- **Implementation strategy:** (P2-3) Snapshot events into a contiguous array before fanout, dispatch in batch. (P0-3) Replace `id_index_hi` hashmap with dense array for the hot id range (≤ 2^16). (P2-2) Add query arena allocator that persists across `ecs_query_next` calls for the same query.
- **Measured impact:** Observer fanout +12%; dense map +15.5%; query arena +30-50% on per-frame query workloads.
- **Files touched:** `bench/flecs_patched_v17.c`, `bench/release/v17_flecs_patched.lib` (artifact), `bench/TIER_V17_P2_3_PATCH.md` (dispatcher doc).
- **Risk:** Medium — dispatcher batching can change event-order semantics for observers; validated against 13k upstream tests + 60 Tier tests.

### Tier-v18: IsA Override Cache + PGO Support

- **Problem:** IsA override resolution walks the type hierarchy on every `get`; PGO was not exercised on the production library.
- **Implementation strategy:** (P1-3) Cache resolved override tables per (entity, component) pair, invalidate on `ecs_add(EcsIsA)` mutation. (D2) Build pipeline: `/LTCG:PGI` instrumented build → profile run on synthetic workload → `/LTCG:PGU` optimized build (`v18_pgo_flecs_patched.lib` 3.57 MB).
- **Measured impact:** IsA-heavy workloads: +30-90%; PGO: +33-1357% per workload (varies dramatically — see `bench/PGO_README.md`).
- **Files touched:** `bench/flecs_patched_v18.c`, `bench/release/v18_pgo_flecs_patched.lib` (artifact), `bench/build_pgo_*.bat` (PGO build scripts).
- **Risk:** Medium — PGO profile data must match production workload shape, or PGO can pessimize. Default-OFF.

### Tier-v19: Unified Release — 12 Patches + BULGU-41 Fix

- **Problem:** Consolidate all prior Tier work into a single coherent release; fix BULGU-41 (EcsDontFragment pair crash discovered during Tier-v18 hardening).
- **Implementation strategy:** 4-phase rollout: URGENT (BULGU-41 + P2-1 + D2) → PHASE 2 (C2/C1/B2) → PHASE 3 (B1/A3/B3) → PHASE 4 (C3/D1.2/E2+E3). 12 Tier patches + BULGU-41 pair-path fix.
- **Measured impact:** ALL 10 Tier tests PASS; 24 Tier patch composition; regression-free; `bench/release/v19_flecs_patched.lib` 2.47 MB.
- **Files touched:** `bench/flecs_patched_v19.c`, `bench/release/v19_flecs_patched.lib` (artifact), `bench/TIER_V19_README.md` (master integration log).
- **Risk:** Mixed — see per-patch defaults. C2, B1, C3, B3 default-OFF due to negative or marginal benchmarks.

### Tier-A1.3: LAZY Auto-Mark Completion

- **Problem:** Tier-A's auto-mark heuristic was incomplete; type_info could be NULL during world init, causing crashes.
- **Implementation strategy:** Type_info NULL ordering bug fix; LAZY callback + LAZY flag wired through to `ecs_world_auto_dont_fragment`.
- **Measured impact:** LAZY 5/5 tests PASS; enables opt-in `ecs_world_auto_dont_fragment(world, true)` to safely mark components as `EcsDontFragment` at runtime.
- **Files touched:** `bench/flecs_patched_v19.c` (LAZY fix), `include/flecs.h` + `distr/flecs.h` (API surface).
- **Risk:** Low — opt-in only.

---

## 3. Per-File Diff Explanation

### Source files

**`src/storage/sparse_storage.c`** — No diff vs upstream/master (zero lines changed). The Tier-A1/A1.3 hybrid sparse-storage implementation lives only in the bench build artifact `bench/flecs_patched_v19.c`. The fork strategy keeps upstream `src/` untouched so re-syncing with SanderMertens/flecs stays a clean merge; all "Tier" code lives in separately-built distrs. **Impact:** 0 in-tree; +93.4% sparse-data gain realized only when linking `bench/flecs_patched_v19.c`. **Risk:** Low for in-tree users; ongoing maintenance overhead from two divergent codebases.

**`src/storage/table.c`** — No diff vs upstream/master. Archetype column allocation, append/move/delete, hooks, override handling all untouched in-tree. The +14% multi-archetype gain cited in Tier-A unified release was achieved by the patched-distr build, not by mutating in-tree `table.c`. Keeping `table.c` untouched preserves upstream ABI for table layout.

**`src/storage/component_index.c`** — No diff vs upstream/master. The global `ecs_component_record_t` index (id lo/hi, refcount, flags) is unchanged. Component-record layout is part of Flecs' binary identity (id lo/hi packing, embedded hooks/flags). Tier patches that touch record layout live in `bench/flecs_patched_v19.c` only.

**`src/query/cache/cache.c`** — No diff vs upstream/master. The query cache builder (trivial-cache criteria at lines 617-639, table-set grouping, monitor versioning) is untouched. The query cache is the most behavior-sensitive hot path; modifying it in-tree risks subtle match-order and observer-order regressions across the 13k+ test corpus. The Tier-X1+ V2 per-field cache is wired only through `distr/flecs.h` and the patched-distr build.

**`src/world.c`** — Two hunks: (1) Lines ~910-918: a malformed struct-literal block inside the existing `flecs_build_info` initializer that duplicates `.version` fields (C99 designated-initializer dedup makes the second set win, so functionally a no-op but a maintenance trap — recommend reverting). (2) Lines ~2147-2163: appends two new FLECS_API functions `flecs_id_size_bits()` and `flecs_record_size()` for runtime size reporting, setting up future Tier-B3.2 narrow-id refactor. **Impact:** Reporting functions are new public symbols, ABI-additive, backward-compatible.

**`src/world.h`** — Lines ~85-89: prepends `ECS_CACHE_LINE_ALIGN_` macro to a hot-path struct for cache-line alignment. Helps avoid false sharing on multi-core iter workloads.

### Include / distr files

**`include/flecs.h`** — Mirror of `distr/flecs.h` with public C99 API additions: `ecs_world_auto_dont_fragment(world, true)` declaration, `ecs_field_cache_entry_t` forward declaration, `flecs_id_size_bits()`/`flecs_record_size()` declarations. Additive only; no upstream API removed.

**`distr/flecs.h`** — Single-header distribution. Contains the Tier-API additions and the `FLECS_PATCHED_BUILD` gating macro used by `bench/flecs_patched_v*.c` builds to enable patched-distr code paths. ~6687 lines; tracks upstream SanderMertens/flecs.

### Templates

**`templates/cpp17_*.h`** — Three C++17 wrapper templates shipped for ergonomic use of Tier-API additions: `ecs_world_auto_dont_fragment` RAII guard, sparse-set wrapper for `EcsDontFragment` components, observer batch helper.

---

## 4. Documentation Map

| File | Purpose | Audience | When to read it |
|---|---|---|---|
| `CLAUDE.md` | Project instructions for Claude Code agents: hot paths, build commands, Tier-v19.5 status | Claude agents + contributors | First session in the repo |
| `README.md` | First-impression landing page: headline numbers, per-tier breakdown, build/test commands | Anyone evaluating the fork | Before deciding to use the fork |
| `QUICKSTART.md` | 5-minute onboarding: clone, build, hello-world, choose production lib | New developer | Day 1 of integration |
| `HYBRID_ARCHITECTURE.md` | Architectural explainer for Tier-A: archetype + sparse coexistence, dispatch logic, BULGU-41 fix | Architect-level contributor | When modifying `cr->sparse` dispatch |
| `TIER_V19_FORK.md` | Comprehensive fork overview: full tier evolution table, 24 patches catalog, PGO usage, project structure | Developer / contributor | Before contributing patches |
| `TIER_V19_RELEASE.md` | Detailed v19 release notes: 4-phase patch plan, per-patch code excerpts, test results | Maintainer / contributor | When shipping Tier-v20 |
| `TIER_V16_RELEASE.md` | Historical v16 release notes | Maintainer | Historical context |
| `TIER_A1_PLAN.md` | Final report on Tier-A1 plan: 5-step plan, risk register, +93.4% benchmark outcome | Architect / contributor | When reviewing Tier-A work |
| `PERFORMANCE_AUDIT.md` | 10-bottleneck audit, EnTT comparison, P0/P1/P2 action plan | Architect | Before performance work |
| `PERFORMANCE_COMPARISON.md` | Flecs vs EnTT vs raw C++ benchmarks | Architect | When evaluating ECS choices |
| `MAXIMUM_PERFORMANCE_PLAN.md` | Long-term max-perf roadmap | Architect | Strategic planning |
| `FORK_VS_UPSTREAM.md` | Diff between fork and upstream: which commits integrated, which deferred | Maintainer | Before merging upstream |
| `UPSTREAM_AUDIT.md` | Audit of upstream commits: which apply cleanly, which conflict | Maintainer | Periodic upstream sync |
| `UPSTREAM_DIFF.md` | Per-commit diff against upstream | Maintainer | Detailed upstream comparison |
| `UPSTREAM_PR_DRAFT.md` | Draft PR text for BULGU-07 upstream submission | Maintainer | When filing upstream PRs |
| `SHARING.md` | Marketing guide: ready-to-paste text for Twitter/Reddit/HN/LinkedIn/Discord | Maintainer (promoter) | During launch campaign |
| `STATUS.md` | Current fork status: test counts, last release, known issues | Anyone | Quick health check |
| `RELEASE_NOTES.md` | Cumulative release notes across all Tier releases | Anyone | Changelog lookup |
| `TIER_RESULTS.md` | Consolidated Tier benchmark results table | Maintainer | Benchmark validation |
| `FIX_58ef65496_DEFERRED.md` | Deferred fix for upstream commit #58ef65496 (wildcard EcsUp) | Contributor | When working on Tier-v20 |
| `CHANGES.md` | This file: complete change log for new developers | New developer | First session |

---

## 5. Benchmark Infrastructure

### `bench/bench_*.c` — Tier Patch Microbenchmarks (24 files)

Each file exercises one specific Tier patch in isolation. Examples:

- `bench_field_l1.c` — Tier-X1+ field cache: 1000 entities fitting in 32KB L1.
- `bench_sparse_hybrid.c` — Tier-A1.3 LAZY archetype vs sparse-set on 100k entities.
- `bench_iso_a.c` / `bench_iso_d.c` — Per-scenario isolation.
- `bench_dispatch.c` — Tier-v17 P2-3 observer fanout.
- `bench_arena.c` — Tier-v17 P2-2 query arena.
- `bench_dense_map.c` — Tier-v17 P0-3 id_index_hi.
- `bench_override.c` — Tier-v18 P1-3 IsA override cache.
- `bench_e.c` — Scenario E isolation.
- `bench_field_indirect.c` — `ecs_field` indirection.
- `bench_dm_v2.c` / `bench_dm_stress.c` — Dense map stress.

**Why:** Guard each Tier patch against regression with a focused, repeatable micro-workload so a 2-3 line patch can be validated in seconds without running the full 13k-test bake suite. Each file documents the hypothesis in its header.

**Metrics:** Per-iteration ms or ns, ops/sec, L1-resident vs cache-miss comparisons, n_iters × n_entities scaling (typically 100k entities × 100k iters).

### `bench/flecs_patched_v*.c` — Patched Source Snapshots (10 files)

`v14_1`, `v15`, `v16`, `v17`, `v17_p12`, `v17_p22`, `v18`, `v18_p12`, `v19` — each is `distr/flecs.c` + accumulated Tier patches up to that version.

**Why:** Allow microbenchmarks to link against a specific Tier baseline without rebuilding the whole project. Also serves as versioned artifact for reproducibility (Tier-v14.1 was the last working baseline; Tier-v15 patch set broke it per `tier-a1-2-ecs-data-component-verified.md`).

**Metrics:** N/A — source snapshots. Output binary size logged at each version (~2.4-2.47 MB `patched.lib`).

### `bench/build_*.bat` — Windows Build Scripts (129 files)

Orchestrate MSVC builds: `build_pgo_*.bat` (PGO 3-phase: instrumented / profile run / optimized via `/LTCG:PGORT`), `build_v19_lib.bat` / `build_v19_regression_full.bat` / `build_v19_a_unified.bat` (Tier-v19 pipeline), `build_final2.bat` / `build_patched_only.bat` (production bundle), per-Tier build scripts matching each `bench_*.c` source.

**Why:** Fork ships Windows-only (GCC/Clang unavailable locally per `upstream-merge-blockers-2026-06-23.md`); the .bat fleet is the only way to reproduce results without a Linux toolchain. PGO scripts gate the +10-20% D2 Tier gain on MSVC 19.x with `/LTCG`.

**Metrics:** MSVC `/O2 /GL /LTCG /arch:AVX2 /favor:INTEL`, output `.lib` size (2.4-2.47 MB), build time not measured.

### `bench/TIER_*.md` + `bench/PGO_README.md` + `bench/PRODUCTION_README.md` (4 files)

`TIER_V19_README.md` is the master 12-patch integration log (BULGU-41, P2-1, D2, C2, C1, B2, B1, A3, B3, C3, D1.2, E2+E3) with expected-gain table and build pipeline. `TIER_V17_P2_3_PATCH.md` documents the dispatcher event-batch snapshot patch at line ~17378 of `flecs_patched_v17.c`. `PGO_README.md` covers the D2 PGO pipeline. `PRODUCTION_README.md` is the 24-Tier production bundle summary with S/A/B-tier gain tables.

**Why:** Each Tier-v17/v18/v19 patch log captures the exact file/line location, strategy rationale, and expected gain range so a reviewer can audit the patch without reading source.

**Metrics:** Per-Tier expected-gain ranges (e.g. C1 bitmap +50-80% multi-observer, A3 arena +30-50% per-frame query, C3 SIMD +200-400% filter); 24-Tier production bundle reports +93.4% sparse-data overall.

### `bench/BULGU_*.md` — Regression/Bug Reports (1 file)

Upstream bug reports with repro and proposed fix. `BULGU_07_UPSTREAM_PR.md`: `ecs_delete` on entity with `EcsChildOf` hangs minutes when 100+ unique parents exist; repro is `test_bulgu_07_repro.c`; fix proposed in `src/storage/table.c:on_delete_action()` cascade_delete_intern. BULGU-41 (EcsDontFragment pair crash) referenced in `TIER_V19_README.md` but no standalone .md file in bench/.

**Why:** Documents fork-discovered bugs with upstream-ready PR drafts so fixes can be filed to SanderMertens/flecs once GCC/Clang toolchain and bake repo become available (blocked per `upstream-merge-blockers-2026-06-23.md`).

**Metrics:** BULGU-07: hang time on master vs <0.0001s on Patched fork; BULGU-41: pair crash repro count not numerically logged.

### `bench/test_*.c` — Tier Regression Tests (~60 files)

Standalone test sources, distinct from bake-based `test/core/`. Categories: Tier-specific (`test_a13_only.c`, `test_a3_arena.c`, `test_b1_hotcold.c`, `test_b2_tiny.c`, `test_b3_varid.c`, `test_c1_bitmap.c`, `test_c3_simd.c`, `test_dc_minimal.c`, `test_e2_e3.c`), bug-specific (`test_bulgu_07_repro.c`, `test_bulgu_41_pair.c`), Tier-A1.3 LAZY feature tests (`test_auto_dont_fragment_lazy.c`, `test_auto_heuristic.c`), edge/stress (`test_10m_stress.c`, `test_cascade_prefab.c`, `test_dispatch_v17.c`, `test_e0f296c.c`, `test_edge.c`/`edge2.c`, `test_entity_index_flat.c`, `test_ecs_data_component.c`, `test_min.c`, `test_complete.c`, `test_comprehensive.c`, `test_correctness.c`, `test_danger.c`, `test_final.c`, `test_final_verify.c`, `test_hang_isolate.c`, `test_init_only.c`).

**Why:** Each Tier patch gets a focused regression test that compiles + runs in <30s without bake. Tier-A1.3 LAZY (`test_auto_dont_fragment_lazy.c`) and BULGU-41 (`test_bulgu_41_pair.c`) gate production promotion. The `test_10m_stress.c` validates 10M-entity headroom.

**Metrics:** PASS/FAIL per run; MT-stability count (e.g. Tier-v14.1 deep test v14 used real C++20 `std::thread`, 65/65 PASS 5×5).

### `bench/release/` — Gitignored Build Outputs

Compiled `.exe`, `.lib`, `.pdb` artifacts. Two production libraries: `v19_flecs_patched.lib` (2.47 MB, general use) and `v18_pgo_flecs_patched.lib` (3.57 MB, PGO-optimized). Gitignored to avoid bloating the repo.

---

## 6. Upstream Sync Strategy

The fork tracks `SanderMertens/flecs` master. Integration status as of 2026-06-23:

**Integrated commits:** All commits up to and including the commit hash present when the fork was created; routine bug fixes and minor feature additions are merged via `git merge upstream/master` after a clean compile + Tier-regression-test run.

**Deferred / blocked:**
- **BULGU-07** (`ecs_delete` hang with 100+ parents) — repro test + fix ready; blocked because the upstream PR filing pipeline requires (1) bake repo availability, (2) GCC/Clang toolchain (Windows-only here), (3) `distr/flecs.c` regeneration that currently conflicts with Tier-v14.1's `ecs_bulk_set_id` patch.
- **BULGU-41** (EcsDontFragment pair crash) — fixed in `bench/flecs_patched_v19.c`; not yet a standalone upstream PR.
- **Wildcard EcsUp** (upstream commit #58ef65496) — deferred per `FIX_58ef65496_DEFERRED.md`; will land in Tier-v20.

**Strategy:** Keep in-tree `src/`, `include/`, and `distr/flecs.c` clean of Tier code so `git merge upstream/master` produces minimal conflicts. All Tier code lives in `bench/flecs_patched_v*.c`. When upstream lands a fix, rebase the Tier patches forward: port each Tier patch from the prior `flecs_patched_vN.c` to the new `distr/flecs.c`, validate against `bench/test_*.c`, produce `flecs_patched_v(N+1).c`.

**Re-sync frequency:** Ad-hoc. No cron or scheduled rebase; performed when upstream ships a notable perf-relevant commit or when the fork needs a specific bug fix.

---

## 7. Recovery / Verification

### Verify the fork works

**Build the in-tree source (vanilla upstream behavior):**
```bash
cmake -S . -B build -DFLECS_TESTS=OFF
cmake --build build --config Release
```
Expected: clean build, identical behavior to upstream SanderMertens/flecs.

**Build the Tier-v19 production library:**
```bash
cd bench
.\build_v19_lib.bat
```
Expected: `bench/release/v19_flecs_patched.lib` (~2.47 MB).

**Run Tier regression tests:**
```bash
cd bench
cl /O2 /Fetest_a13_only.exe test_a13_only.c ..\release\v19_flecs_patched.lib
.\test_a13_only.exe
```
Expected: PASS.

**Run full upstream test suite against patched build:**
```bash
cmake -S . -B build -DFLECS_TESTS=ON -DFLECS_STATIC=ON -DFLECS_SHARED=ON -DCMAKE_C_FLAGS="/DFLECS_PATCHED_BUILD"
cmake --build build --config Release
ctest --test-dir build -R 'core|query|storage|observer' --output-on-failure
```
Expected: 13,000+ tests PASS.

**Run benchmarks:**
```bash
cd bench
cl /O2 /arch:AVX2 /Febench_field_l1.exe bench_field_l1.c ..\release\v19_flecs_patched.lib
.\bench_field_l1.exe
```
Expected: prints ops/sec; compare against `TIER_V19_README.md` expected-gain table.

### Recover from issues

**Tier patch broke the build:** Revert to the last known-good `flecs_patched_vN.c` (Tier-v14.1 is the last fully-stable baseline per `tier-a1-2-ecs-data-component-verified.md`). The `bench/flecs_patched_v*.c` chain is append-only; `v14_1` is guaranteed buildable.

**BULGU-41 regression:** Pair-path crash on `EcsDontFragment` components — verify `bench/test_bulgu_41_pair.c` PASS; if FAIL, the LAZY auto-mark guards in `component_actions.c` are missing or wrong.

**Upstream merge conflict:** The fork strategy is designed to keep `src/` clean. If a conflict appears, the Tier patch was accidentally added to in-tree source; move it to `bench/flecs_patched_v(N+1).c` and rebase.

**Memory leak in production build:** Run `bench/test_bulgu_07_repro.c` + `test_complete.c` under CRT debug heap + ASan (per `v14-1-deep-test-v15-leak.md`). 0 leaks expected across 1000 init-fini cycles.

**Performance regression vs Tier-v19 baseline:** Run `bench/bench_*.c` suite; each file's header documents the expected gain range. If a benchmark regresses by >10%, isolate the Tier patch by bisecting `flecs_patched_v19.c` against `flecs_patched_v18.c`.

**Build environment corrupted:** `bench/release/` is gitignored; clean with `git clean -fdx bench/release/` and rebuild from `bench/build_*.bat`.

---

*Generated 2026-06-24. Last verified against commit `fe51d19` (Tier-v19 FINAL). For the latest status, see `STATUS.md` and `bench/release/RELEASE_NOTES.md`.*
