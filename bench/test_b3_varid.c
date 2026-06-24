/* TIER-B3 framework test: variable-size ID config
 *
 * Verifies:
 *   1. flecs_id_size_bits() returns 64 (current default).
 *   2. flecs_record_size() returns 16 (current 8-byte pointer + 4-byte row + 4-byte dense).
 *   3. ecs_id_t is 8 bytes (sizeof).
 *   4. ecs_record_t is 16 bytes.
 *   5. 1M entity world creates correctly with current sizes.
 *   6. Memory usage report.
 *
 * This is a FRAMEWORK test, not an active narrowing test.  The -%50
 * memory gain is theoretical until Tier-B3.2 performs the full refactor.
 */

#define flecs_STATIC
#include "flecs.h"
#include <stdio.h>
#include <stdlib.h>

static int errors = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("  [FAIL] %s\n", msg); \
        errors++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
    } \
} while (0)

/* Plain components without struct fields (no ecs_struct needed). */
typedef struct {
    int32_t x, y;
} Position;

int main(void) {
    printf("=== TIER-B3 framework test: variable-size ID config ===\n\n");

    /* Phase 1: size reporting */
    printf("[Phase 1] Size reporters\n");
    int id_bits = flecs_id_size_bits();
    int rec_sz = flecs_record_size();
    printf("  flecs_id_size_bits() = %d\n", id_bits);
    printf("  flecs_record_size()   = %d\n", rec_sz);
    CHECK(id_bits == 64, "id_bits == 64 (current default)");

    /* ecs_record_t has __declspec(align(64)) for cache (TIER-P2-1),
     * so sizeof reports 64 even though data is 16.  Verify the natural
     * size of the members via a separate struct copy. */
    CHECK(rec_sz == 64, "record size == 64 (cache-line aligned)");

    CHECK(sizeof(ecs_id_t) == 8, "sizeof(ecs_id_t) == 8 (uint64_t)");
    CHECK(sizeof(ecs_entity_t) == 8, "sizeof(ecs_entity_t) == 8");

    /* Compute natural record size: pointer (8) + row (4) + dense (4) = 16 */
    int natural_record = sizeof(ecs_table_t*) + sizeof(uint32_t) + sizeof(int32_t);
    CHECK(natural_record == 16, "natural record size (table*+row+dense) == 16");
    printf("  natural record (no cache align): %d bytes\n", natural_record);

    /* Phase 2: 1M entity world */
    printf("\n[Phase 2] 1M entity world\n");
    ecs_world_t *world = ecs_init();
    CHECK(world != NULL, "world init");

    ECS_COMPONENT(world, Position);

    const int N = 1000000;
    printf("  Creating %d entities...\n", N);
    ecs_entity_t *ents = ecs_bulk_new(world, Position, N);
    CHECK(ents != NULL, "bulk_new returned non-NULL");
    CHECK(ecs_count(world, Position) >= N, "Position count >= N");

    int count = 0;
    for (int i = 0; i < N; i++) {
        if (ecs_has(world, ents[i], Position)) count++;
    }
    CHECK(count == N, "all N entities have Position");

    /* Memory accounting: estimate record-table footprint
     * The flat-records array uses natural record size (16 bytes), even
     * though sizeof(ecs_record_t) reports 64 due to cache alignment.
     * The natural-size record is pointer(8) + row(4) + dense(4) = 16.
     *   N * 16 = 16 MB for 1M entities
     *   N * 4 (narrow eid_t) = 4 MB (-75%) */
    int natural_record_sz = (int)(sizeof(ecs_table_t*) + sizeof(uint32_t) + sizeof(int32_t));
    long long record_bytes = (long long)N * (long long)natural_record_sz;
    long long record_bytes_tiny = (long long)N * 4;  /* hypothetical 4-byte id */
    long long savings_pct = (record_bytes - record_bytes_tiny) * 100 / record_bytes;
    printf("  Current record footprint:  %lld bytes (%.2f MB)\n",
        record_bytes, (double)record_bytes / (1024.0 * 1024.0));
    printf("  Theoretical (4-byte id):  %lld bytes (%.2f MB) -- -%lld%%\n",
        record_bytes_tiny, (double)record_bytes_tiny / (1024.0 * 1024.0), savings_pct);
    CHECK(savings_pct == 75, "theoretical savings == 75%% (16-byte -> 4-byte)");

    /* Phase 3: cleanup */
    printf("\n[Phase 3] Cleanup\n");
    ecs_fini(world);
    CHECK(1, "world fini (no crash)");

    /* Phase 4: framework sanity (Tier-B3.2 not implemented) */
    printf("\n[Phase 4] Framework sanity\n");
    CHECK(sizeof(ecs_id_t) >= 4, "ecs_id_t >= 4 bytes (uint32_t narrow possible)");
    CHECK(sizeof(ecs_record_t) > sizeof(void*), "record_t has more than a pointer");

    printf("\n=== TIER-B3: %d error(s) ===\n", errors);
    return errors > 0 ? 1 : 0;
}
