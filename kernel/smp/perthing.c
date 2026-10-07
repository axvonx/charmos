#include <console/panic.h>
#include <global.h>
#include <linker/symbols.h>
#include <mem/alloc.h>
#include <mem/must.h>
#include <smp/core.h>
#include <smp/domain.h>
#include <smp/percpu.h>
#include <smp/perdomain.h>
#include <smp/pertopo.h>

void percpu_obj_init(void) {
    struct percpu_descriptor *d;
    linker_section_for_each_object(d, percpu_desc) {
        d->percpu_ptrs = must_kmalloc(sizeof(void *) * global.core_count);

        size_t cpu;
        for_each_cpu_id(cpu) {
            d->percpu_ptrs[cpu] =
                must(kmalloc_aligned(d->size, d->align, ALLOC_ZERO));

            if (d->constructor)
                d->constructor(d->percpu_ptrs[cpu], cpu);
        }
    }
}

void perdomain_obj_init(void) {
    struct perdomain_descriptor *d;
    linker_section_for_each_object(d, perdomain_desc) {
        d->perdomain_ptrs = must_kmalloc(sizeof(void *) * global.domain_count);

        size_t id;
        domain_for_each_domain_id(id) {
            d->perdomain_ptrs[id] =
                must(kmalloc_aligned(d->size, d->align, ALLOC_ZERO));

            if (d->constructor)
                d->constructor(d->perdomain_ptrs[id], id);
        }
    }
}

void pertopo_obj_init(void) {
    struct pertopo_descriptor *d;
    linker_section_for_each_object(d, pertopo_desc) {
        d->pertopo_ptrs =
            must_kmalloc(sizeof(void *) * global.topology.count[d->level]);
        size_t n;
        topology_for_each_id(n, d->level) {
            d->pertopo_ptrs[n] =
                must(kmalloc_aligned(d->size, d->align, ALLOC_ZERO));

            if (d->constructor)
                d->constructor(d->pertopo_ptrs[n], n);
        }
    }
}

size_t pertopo_node_local(struct pertopo_descriptor *desc,
                          enum topology_caller c) {
    enum topology_level l = desc->level;
    kassert(topology_contract_verify(
        (struct topology_contract){.caller = c, .scope = l}));

    struct core *self = smp_core_raw();
    for (struct topology_node *n = self->topo_node; n; n = n->parent_node) {
        if (n->level == l)
            return n->id;
    }

    return SIZE_MAX;
}
