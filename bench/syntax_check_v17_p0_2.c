/* syntax_check_v17_p0_2.c — minimal compile-check for P0-2 helper.
 * Tests just the helper body with stub flag/table types. */
#include <stdint.h>
#include <stdbool.h>

/* Stub flag bits matching flecs_patched.h:552-588 */
#define EcsTableHasCtors              (1u << 11u)
#define EcsTableHasDtors              (1u << 12u)
#define EcsTableHasToggle             (1u << 15u)
#define EcsTableHasSparse             (1u << 21u)
#define EcsTableIsComplex             (EcsTableHasCtors | EcsTableHasDtors | EcsTableHasToggle | EcsTableHasSparse)
#define EcsTableHasIsA                (1u << 2u)
#define EcsTableHasTraversable        (1u << 27u)
#define EcsTableHasOnAdd              (1u << 16u)
#define EcsTableHasOnRemove           (1u << 17u)
#define EcsTableHasOnSet              (1u << 18u)
#define EcsTableHasOnTableCreate      (1u << 19u)
#define EcsTableHasOnTableDelete      (1u << 20u)

typedef uint32_t ecs_flags32_t;
typedef struct { ecs_flags32_t flags; } ecs_table_t;

static bool flecs_table_can_skip_column_walk(const ecs_table_t *table) {
    ecs_table_t *t = (ecs_table_t *)table;

    if (t->flags & EcsTableIsComplex) return false;

    if (t->flags & (EcsTableHasOnAdd | EcsTableHasOnRemove |
                    EcsTableHasOnSet | EcsTableHasOnTableCreate |
                    EcsTableHasOnTableDelete)) {
        return false;
    }

    if (t->flags & EcsTableHasIsA) return false;
    if (t->flags & EcsTableHasTraversable) return false;

    return true;
}

int main(void) {
    ecs_table_t t_clean = {0};
    ecs_table_t t_dtor = {EcsTableHasDtors};
    ecs_table_t t_obs  = {EcsTableHasOnAdd};
    ecs_table_t t_isa  = {EcsTableHasIsA};

    bool a = flecs_table_can_skip_column_walk(&t_clean); /* expect true */
    bool b = flecs_table_can_skip_column_walk(&t_dtor);  /* expect false */
    bool c = flecs_table_can_skip_column_walk(&t_obs);   /* expect false — P0-2 */
    bool d = flecs_table_can_skip_column_walk(&t_isa);   /* expect false */

    return (a && !b && !c && !d) ? 0 : 1;
}