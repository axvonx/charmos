/* @title: Scheduler Globals */
#pragma once
#include <atomic.h>
#include <topo/topology.h>

struct scheduler;
struct scheduler_domain;

struct sch_globals {
    atomic_size_t idle_cpu_count;

    /* counted by topo_global.cpu_count */
    struct scheduler **schedulers;

    struct scheduler_domain *scheduler_domains[TOPOLOGY_LEVEL_MAX];
};

extern struct sch_globals sch_global;
