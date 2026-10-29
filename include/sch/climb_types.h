/* @title: CLIMB Types */
#pragma once
#include <structures/list.h>
#include <structures/rbt.h>
#include <types/types.h>

typedef fx32_32_t climb_pressure_t;

enum climb_pressure_kind {
    CLIMB_PRESSURE_DIRECT,
    CLIMB_PRESSURE_INDIRECT,
};

struct climb_handle {
    struct list_node list;
    climb_pressure_t pressure;
    climb_pressure_t applied_pressure_internal; /* If 0, this has not
                                                 * applied presssure */

    enum climb_pressure_kind kind;

#ifdef DEBUG_CLIMB
    struct thread *given_by; /* Debugging */
#endif

    struct thread *given_to;
};

struct climb_thread_data {
    /* pressure */
    climb_pressure_t direct_pressure;   /* Contributed direct pressure */
    climb_pressure_t indirect_pressure; /* Indirect pressure from others */
    climb_pressure_t pressure_ewma;     /* EWMA of pressure over time */

    /* boost */
    int32_t wanted_boost;    /* 0..20, how much this thread "wants" */
    fx32_32_t boost_ewma;    /* EWMA of the wanted_boost_raw */
    int32_t effective_boost; /* How much boosted (derived from wanted_boost_raw)
                              * this thread "gets". This exists because CLIMB
                              * will take into account the existence of other
                              * threads and their boosts and periods spent
                              * to compute the boost of one thread */

    /* time */
    int32_t pressure_periods; /* >=1 active, < 0 decaying, 0 inactive */

    /* accounting */
    struct list_head handles; /* active pressure handles */

    /* scheduler integration */
    bool on_climb_tree : 1; /* Used to logically verify things */

    struct rbt_node climb_node;

    /* if this thread becomes a CLIMB source,
     * what would it "apply"? */
    struct climb_handle handle;
};
