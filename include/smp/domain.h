/* @title: Domains */
#pragma once
#include <smp/core.h>
#include <stdint.h>

/* NOTE: Definition of a "domain"
 *
 * Domains are a first-class topology abstraction used to group CPUs by NUMA
 * nodes when present or by a fixed number on UMA systems
 *
 * The premise: kernels benefit from NUMA awareness, however, the benefits
 * reaped by NUMA also benefit UMA systems. For instance, memory allocators
 * that are NUMA aware get better memory locality AND lower lock contention,
 * however, under UMA, the benefit of lower lock contention, and potentially
 * better cache performance are helpful.
 *
 * Because NUMA logic already exists, we can reuse the logic that handles
 * NUMA systems with UMA systems, and reap the benefits without NUMA.
 *
 */

/* For UMA: TODO: we likely want this to be command line
 * configurable/adjusted at boot time */
#define CORES_PER_DOMAIN 4

struct domain {
    size_t id;
    size_t num_cores;
    struct core **cores;
    struct numa_node *associated_node;
    struct slab_domain *slab_domain;
    struct domain_buddy *domain_buddy;
    struct cpu_mask cpu_mask;
};

void domain_init(void);
struct cpu_mask *domain_create_cpu_mask(struct domain *domain);
void domain_set_cpu_mask(struct cpu_mask *mask, struct domain *domain);
bool domain_idle(struct domain *domain);
numa_node_t numa_node_for_cpu(cpu_id_t cpu);
domain_id_t domain_for_cpu(cpu_id_t cpu);
void domain_init_after_smp();
void domain_caller_verify(enum topology_caller caller);
void domain_dump(void);

static inline struct domain *domain_local(enum topology_caller c) {
    domain_caller_verify(c);

    /* TOPOC_NONE is set here because technically it's not
     * a big deal if we read the 'wrong CPU' since it's
     * guaranteed that we're in a domain */
    return smp_read(TOPC_NONE, domain);
}

static inline domain_id_t domain_local_id(enum topology_caller c) {
    return domain_local(c)->id;
}

#define domain_for_each_domain(dom_)                                           \
    for (domain_id_t __dom_idx = 0;                                            \
         __dom_idx < global.domain_count &&                                    \
         (((dom_) = global.domains[__dom_idx]), true);                         \
         __dom_idx++)

#define domain_for_each_domain_id(id_)                                         \
    for (domain_id_t __dom_id_idx = 0;                                         \
         __dom_id_idx < global.domain_count &&                                 \
         (((id_) = global.domains[__dom_id_idx]->id), true);                   \
         __dom_id_idx++)

#define domain_for_each_core(pos_, dom_)                                       \
    for (domain_id_t __core_idx = 0;                                           \
         __core_idx < (dom_)->num_cores &&                                     \
         (((pos_) = (dom_)->cores[__core_idx]), true);                         \
         __core_idx++)

#define domain_for_each_core_id(pos_, dom_)                                    \
    for (domain_id_t __core_id_idx = 0;                                        \
         __core_id_idx < (dom_)->num_cores &&                                  \
         (((pos_) = (dom_)->cores[__core_id_idx]->id), true);                  \
         __core_id_idx++)

#define domain_for_each_core_local(clr_, pos_)                                 \
    domain_for_each_core(pos_, smp_core(clr_)->domain)
