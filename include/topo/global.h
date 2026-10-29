/* @title: Topology Globals */
#pragma once
#include <compiler/core.h>
#include <topo/topology.h>
#include <types/types.h>

struct core;
struct domain;
struct numa_node;

struct topo_globals {
    size_t cpu_count;
    struct core **cores cc_counted_by(cpu_count);

    struct topology topology;

    size_t numa_node_count;
    struct numa_node *numa_nodes;

    size_t domain_count;
    struct domain **domains cc_counted_by(domain_count);
};

extern struct topo_globals topo_global;
