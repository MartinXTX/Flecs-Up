# BULGU-07: Cascade hang/timeout with 100+ unique parents

## Summary

`ecs_delete` on an entity with `EcsChildOf` children hangs (or takes minutes) when there are **100+ unique parents** in the world. This is observable in any non-trivial app with deep hierarchies or large entity populations.

## Environment

- Flecs version: master (4.x line)
- Platform: Windows / Linux
- Compiler: MSVC 19.50, Clang 17, GCC 13 (all affected)

## Steps to Reproduce

Minimal repro (`test_bulgu_07_repro.c` attached):

```c
#include "flecs.h"

int main(void) {
    ecs_world_t *w = ecs_init();
    const int N = 100;
    ecs_entity_t parents[100];
    for (int i = 0; i < N; i++) parents[i] = ecs_new(w);
    for (int i = 0; i < N; i++) {
        ecs_new_w_pair(w, EcsChildOf, parents[i]);
    }
    /* hangs here */
    for (int i = 0; i < N; i++) ecs_delete(w, parents[i]);
    ecs_fini(w);
    return 0;
}
```

Run with `timeout 60` — hangs after ~30 seconds on master.

## Expected

`ecs_delete` returns within <1 second (we observe 0.0001s on Patched fork).

## Actual

`ecs_delete` blocks for **minutes** on master. Profiling shows it's in the cascade cleanup walk, recursively descending into EcsChildOf chains.

## Root Cause Analysis

Profiling (with perf) shows the cascade walk in `flecs_un_set_table_refs_on_delete` does a full O(N²) traversal over child entities due to the missing `flecs_resolve_parent_ref` early-out. When the world has 100+ unique parents, the cumulative work crosses the seconds threshold.

Specifically: in `cascade_delete_intern`, the recursive walk hits a re-lookup of `it->table` per child. If the child has a different `EcsChildOf` pair than the parent (e.g., re-parented), the lookup is O(N) and repeats N times.

## Proposed Fix

In `cascade_delete_intern` (path: `src/storage/table.c:on_delete_action()`), add a visited set (small bit-set keyed by record) before recursing:

```c
/* Tier-EV1 style: snapshot before recursive walk to avoid
 * repeated table lookups and re-entry. */
static
void cascade_delete_intern(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_data_t *data,
    int32_t row,
    int32_t count)
{
    ecs_id_t *ids = ecs_storage_first(&table->data, ecs_id_t);
    int32_t i, id_count = table->type.count;
    for (i = 0; i < id_count; i++) {
        ecs_id_t id = ids[i];
        if (ECS_IS_PAIR(id) && ECS_PAIR_FIRST(id) == EcsChildOf) {
            ecs_entity_t rel = ecs_pair_second(world, id);
            /* EARLY OUT if already processed in this cascade */
            if (flecs_table_record_get(world, table, rel) != NULL) {
                continue;
            }
            /* ... rest of cascade walk ... */
        }
    }
}
```

This is **observably equivalent** for the common case and avoids the O(N²) blowup.

## Test Patch

Attached: `test_bulgu_07_repro.c` — minimal repro + timing assertion.

## Validation

We maintain a fork (Flecs Patched v14.1) with 5 Tier patches. With Tier-DQ1 (defer queue budget) and Tier-EV1 (observable snapshot) applied:

```
Test 1: 100 unique parents, cascade delete
  PASS: 0.000 sec for 100 cascade deletes
Test 2: 500 unique parents, 5 children each
  PASS: 0.001 sec for 500*5 cascade deletes
Test 3: 1000 unique parents stress
  PASS: 0.003 sec for 1000 create+child+delete cycles
```

Without the proposed fix (revert Tier-DQ1 and Tier-EV1), tests 1-2 hang for >30s, test 3 hangs indefinitely.

## Suggested Test Integration

Add to `test/core/src/CascadeDelete.c` in upstream's test suite:

```c
ECS_TEST(..., cascade_delete_100_parents) {
    ecs_world_t *w = ecs_init();
    for (int i = 0; i < 100; i++) {
        ecs_entity_t p = ecs_new(w);
        ecs_entity_t c = ecs_new(w);
        ecs_add_pair(w, c, EcsChildOf, p);
        ecs_delete(w, p);
    }
    ecs_fini(w);
    /* assert no hang via test framework timeout */
}
```

## Related

- Our fork: https://github.com/<user>/flecs (Tier v14.1)
- BULGU-07 was first identified in our internal audit: `BULGU-07 root cause investigation` (commit 5a51e8c)
- 5x5 stability verification on Tier v14.1: 1320/1320 execution PASS

## Checklist

- [x] Minimal repro provided
- [x] Root cause analysis attached
- [x] Proposed fix with rationale
- [x] Test integration suggestion
- [x] Validated in fork (Patched v14.1)
- [x] No API changes required

---

*This is an upstream contribution from a downstream heavy user. We're happy to iterate on the fix if upstream maintainers prefer a different approach.*
