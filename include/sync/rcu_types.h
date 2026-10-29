/* @title: RCU Types */
#pragma once
#include <structures/list.h>
#include <types/types.h>

struct rcu_cb;

typedef void (*rcu_fn)(struct rcu_cb *);

struct rcu_cb {
    struct list_node list;
    rcu_fn fn;

    /* TODO: as debug flags get better, move away from raw TEST_ENABLED */
#ifdef TEST_ENABLED
    size_t gen_when_called; /* Diagnostics */
    size_t enqueued_waiting_on_gen;
    size_t target_gen;
#endif
};
