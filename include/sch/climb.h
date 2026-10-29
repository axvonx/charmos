/* @title: CLIMB framework */
#pragma once
#include <container_of.h>
#include <math/fixed.h>
#include <sch/climb_types.h>
#include <structures/list.h>
#include <structures/rbt.h>
#include <thread/thread_types.h>
#include <types/types.h>

struct climb_handle;

#define CLIMB_BOOST_LEVELS 20
#define CLIMB_MIN_GLOBAL_BOOST 1
#define CLIMB_GLOBAL_BOOST_SCALE(nt) (CLIMB_BOOST_LEVELS / nt)
#define CLIMB_PRESSURE_KEY_SHIFT 31 /* one period == 0.5 pressure points */
#define CLIMB_MAX_DECAY_PERIODS 20

/* Pressures */
#define CLIMB_PRESSURE_THREAD_BASE FX(0.05)
#define CLIMB_PRESSURE_IO_BASE FX(0.20)
#define CLIMB_PRESSURE_LOCK_BASE FX(0.10)
#define CLIMB_PRESSURE(x) FX(x)

/* Pressure space */
#define CLIMB_PRESSURE_MAX FX(1.0)
#define CLIMB_DIRECT_PRESSURE_MAX FX(1.0)
#define CLIMB_INDIRECT_PRESSURE_MAX FX(1.0)

/* Indirect pressure scaling */
#define CLIMB_INDIRECT_MIN_SCALE FX(0.10)
#define CLIMB_INDIRECT_WEIGHT FX(0.85)

/* Boost space */
#define CLIMB_BOOST_LEVEL_MAX 20

/* EWMA smoothing */
#define CLIMB_BOOST_EWMA_ALPHA FX(0.75)

/* Pressure to boost shaping */
#define CLIMB_PRESSURE_EXPONENT 3 /* cubic */
#define CLIMB_PRESSURE_TO_BOOST_SCALE FX(8.0)

#define climb_thread_data_from_tree_node(tn)                                   \
    (container_of(tn, struct climb_thread_data, climb_node))

struct climb_source {
    char *name;
    climb_pressure_t base;
};

#define CLIMB_SOURCE_EXTERN(name) extern struct climb_source __climb_src_##name
#define CLIMB_SOURCE_CREATE(n, strname, b)                                     \
    struct climb_source __climb_src_##n = {.name = strname, .base = b}
#define CLIMB_SOURCE(name) &(__climb_src_##name)

climb_pressure_t climb_thread_applied_pressure(struct thread *t);
climb_pressure_t climb_thread_compute_pressure_to_apply(struct thread *t);

void climb_handle_apply(struct thread *t, struct climb_handle *h);
void climb_handle_update(struct thread *t, struct climb_handle *h,
                         climb_pressure_t new_pressure);
void climb_handle_remove(struct climb_handle *h);

void climb_handle_apply_locked(struct thread *t, struct climb_handle *h);
void climb_handle_remove_locked(struct climb_handle *h);

void climb_recompute_pressure(struct thread *t);
void climb_thread_remove(struct thread *t);
void climb_thread_init(struct thread *t);
void climb_post_migrate_hook(struct thread *t, size_t old_cpu, size_t new_cpu);
size_t climb_get_thread_data(struct rbt_node *n);
int32_t climb_cmp_threads(const struct rbt_node *a, const struct rbt_node *b);

static inline struct climb_handle *
climb_handle_init(struct climb_handle *ch, struct climb_source *cs,
                  enum climb_pressure_kind k) {
    list_node_init(&ch->list);
    if (cs)
        ch->pressure = cs->base;

    ch->kind = k;
    ch->applied_pressure_internal = FX_ZERO;
    return ch;
}
