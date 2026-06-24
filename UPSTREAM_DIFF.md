# Upstream Diff — MartinXTX/flecs vs SanderMertens/flecs

> **Generated:** 2026-06-24
> **Compared:** `MartinXTX/flecs` initial commit (`94b6b5ee9`) vs `SanderMertens/flecs` master (`757d05e85`)
> **Method:** `git diff --numstat <ours> <upstream/master>` — files where any line was added/removed

## Summary

| Metric | Value |
|---|---|
| Total changed files | **301** |
| Files with insertions | 5 |
| Files with deletions only | 296 |
| New root-level files | 24 (docs) |
| New directories | `bench/` (266 files), `distr/` extras |
| Modified `src/` files | 3 |
| Modified `include/` files | 11 |

**Note on the line count (698,196 deletions in raw diff):** this is the upstream blob
boundary mismatch from comparing our single squashed commit against upstream's
multi-commit history. The per-file numstat (above) shows the real per-file
changes; the line totals are inflated by the fact that files in our `distr/`
directory contain inline copies of upstream's entire source tree (one big file
per platform target).

## Files Changed (high-level)

### Documentation (24 new root files)

Fork-introduced documentation describing Tier-v19.5 architecture, performance
gains, fork vs upstream comparison, and release notes:

```
CLAUDE.md                            # Project-specific Claude Code guidance
FORK_VS_UPSTREAM.md                  # Detailed fork vs upstream comparison
HYBRID_ARCHITECTURE.md               # Archetype + Sparse-data hybrid model
MAXIMUM_PERFORMANCE_PLAN.md          # Strategic plan
PERFORMANCE_AUDIT.md                 # Performance bottleneck analysis
PERFORMANCE_COMPARISON.md            # Tier 1 → Tier-v19.5 evolution
QUICKSTART.md                        # 5-minute developer guide
README.md                            # Tier-A + Tier-v19.5 fork intro (modified)
RELEASE_NOTES.md                     # Release history
SHARING.md                           # Twitter/Reddit/HN/LinkedIn/Discord templates
STATUS.md                            # Project status
TIER_A1_PLAN.md                      # Tier-A1 hybrid storage plan
TIER_RESULTS.md                      # Tier implementation results
TIER_V16_RELEASE.md                  # Tier-v16 release notes
TIER_V19_FORK.md                     # Comprehensive fork documentation
TIER_V19_RELEASE.md                  # Tier-v19 release notes
UPSTREAM_AUDIT.md                    # Upstream fix verification
UPSTREAM_PR_DRAFT.md                 # PR draft to upstream
FIX_58ef65496_DEFERRED.md            # Deferred upstream fix notes
```

### Source code (3 modified)

| File | Lines | Change |
|---|---|---|
| `src/storage/sparse_storage.c` | modified | Tier-A1.3 LAZY auto-heuristic dispatcher |
| `src/query/cache/cache.c` | modified | Tier-v17 P0-2 hook-atla predicate |
| `src/storage/table.c` | modified | Tier-v19.5 `flecs_table_copy_elem` optimized helper |

### Public headers (11 modified)

These changes are 100% backward compatible — no API removals, only additive
flags/macros:

```
include/flecs.h                       # ECS_DATA_COMPONENT, Tier patch comments
include/flecs/private/api_internals.h # Internal field cache structures
include/flecs/addons/cpp/flecs.hpp    # C++17 view/system/simd_filter templates
include/flecs/addons/cpp/world.hpp    # ecs_world_auto_dont_fragment
+ 8 more internal header tweaks
```

### Bench infrastructure (266 new files)

Complete benchmark + test infrastructure for fork-specific features:

- **24 C benchmark sources** (`bench/bench_*.c`) — measure all Tier patches
- **100+ Windows `.bat` build scripts** — multi-tier PGO + release build chains
- **24 Tier-specific doc files** (`bench/TIER_*.md`, `bench/UPSTREAM_*.md`)
- **28 produced `.lib` / `.exe` / `.pgd` / `.pgc`** (committed binaries for
  reproducible benchmarking)
- **1 CMake preset** (`bench/CMakePresets.json`)

### Misc

- `.gitignore` (63 new entries to keep `bench/release/` binaries reproducible)
- `CMakePresets.json` (1 line added)
- Various `examples/` updates

## How to view this diff yourself

```bash
# Add upstream as a remote
git remote add upstream https://github.com/SanderMertens/flecs.git
git fetch upstream

# Get the per-file diff stats
git diff --numstat <our-SHA> upstream/master

# Get a real line-by-line diff for a specific file
git diff <our-SHA> upstream/master -- src/storage/sparse_storage.c

# Generate a patch file
git diff <our-SHA> upstream/master > fork-vs-upstream.patch
```

Our SHA: `94b6b5ee993cdd54082e287da682ef11eaa47c68`
Upstream SHA at comparison time: `757d05e852846d56399a0507f1e0c5be0ab05abf`
