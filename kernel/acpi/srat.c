#include <acpi/acpi.h>
#include <log.h>
#include <mem/alloc.h>
#include <mem/nofail.h>
#include <mem/numa.h>
#include <string.h>
#include <structures/cpu_mask.h>
#include <topo/core.h>
#include <topo/global.h>
#include <types/types.h>
#include <uacpi/tables.h>

#include "uacpi/acpi.h"
#include "uacpi/status.h"

LOG_HANDLE_DEFINE_PRINT_STATIC(srat);

void srat_init(void) {
    struct uacpi_table srat_table;
    if (uacpi_table_find_by_signature("SRAT", &srat_table) != UACPI_STATUS_OK) {
        log_warn_global(LOG_HANDLE(srat),
                        "SRAT table not found, assuming single NUMA node");

        topo_global.numa_nodes =
            nofail_vnew(topo_global.numa_nodes, ALLOC_ZERO);

        topo_global.numa_nodes[0].topo = NULL;
        topo_global.numa_nodes[0].mem_base = 0;
        topo_global.numa_nodes[0].mem_size = 0;
        topo_global.numa_nodes[0].distances_cnt = 0;
        topo_global.numa_nodes[0].distance = NULL;
        return;
    }

    struct acpi_srat *srat = (struct acpi_srat *) srat_table.ptr;
    uint8_t *ptr = (uint8_t *) srat + sizeof(struct acpi_srat);
    uint64_t remaining = srat->hdr.length - sizeof(struct acpi_srat);

    uint64_t max_prox_domain = 0;

    while (remaining >= sizeof(struct acpi_entry_hdr)) {
        struct acpi_entry_hdr *entry = (void *) ptr;
        if (entry->type == ACPI_SRAT_ENTRY_TYPE_PROCESSOR_AFFINITY) {
            struct acpi_srat_processor_affinity *cpu = (void *) ptr;
            if (cpu->flags & 1) {
                uint64_t prox_domain =
                    (uint64_t) cpu->proximity_domain_low |
                    (uint64_t) cpu->proximity_domain_high[0] << 8 |
                    (uint64_t) cpu->proximity_domain_high[1] << 16 |
                    (uint64_t) cpu->proximity_domain_high[2] << 24;

                if (prox_domain > max_prox_domain)
                    max_prox_domain = prox_domain;
            }
        } else if (entry->type == ACPI_SRAT_ENTRY_TYPE_MEMORY_AFFINITY) {
            struct acpi_srat_memory_affinity *mem = (void *) ptr;
            if (mem->flags & 1 && mem->proximity_domain > max_prox_domain)
                max_prox_domain = mem->proximity_domain;
        }

        ptr += entry->length;
        remaining -= entry->length;
    }

    topo_global.numa_node_count = max_prox_domain + 1;
    size_t numa_node_count = topo_global.numa_node_count;
    topo_global.numa_nodes =
        nofail_vnews(topo_global.numa_nodes, numa_node_count, ALLOC_ZERO);

    for (size_t i = 0; i < numa_node_count; i++) {
        topo_global.numa_nodes[i].topo = NULL;
        topo_global.numa_nodes[i].mem_base = 0;
        topo_global.numa_nodes[i].mem_size = 0;
        topo_global.numa_nodes[i].distances_cnt = numa_node_count;
        topo_global.numa_nodes[i].distance = nofail_vnews(
            topo_global.numa_nodes[i].distance, numa_node_count, ALLOC_ZERO);

        cpu_mask_init(&topo_global.numa_nodes[i].cpus, topo_global.cpu_count);
    }

    ptr = (uint8_t *) srat + sizeof(struct acpi_srat);
    remaining = srat->hdr.length - sizeof(struct acpi_srat);

    while (remaining >= sizeof(struct acpi_entry_hdr)) {
        struct acpi_entry_hdr *entry = (void *) ptr;
        switch (entry->type) {

        case ACPI_SRAT_ENTRY_TYPE_PROCESSOR_AFFINITY: {
            struct acpi_srat_processor_affinity *cpu = (void *) ptr;
            if (cpu->flags & 1) {
                uint64_t prox_domain =
                    (uint64_t) cpu->proximity_domain_low |
                    (uint64_t) cpu->proximity_domain_high[0] << 8 |
                    (uint64_t) cpu->proximity_domain_high[1] << 16 |
                    (uint64_t) cpu->proximity_domain_high[2] << 24;

                cpu_id_t cpu_id = smp_cpu_from_apic_id(cpu->id);
                if (cpu_id != CPU_ID_NONE) {
                    /* the first pass sized numa_nodes from this same field */
                    assume(prox_domain < topo_global.numa_node_count);
                    /* NOLINTNEXTLINE(*ArrayBound) */
                    cpu_mask_set(&topo_global.numa_nodes[prox_domain].cpus,
                                 cpu_id);
                }
            }
            break;
        }

        case ACPI_SRAT_ENTRY_TYPE_MEMORY_AFFINITY: {

            struct acpi_srat_memory_affinity *mem = (void *) ptr;
            if (!(mem->flags & 1))
                break;

            uint64_t node_id = mem->proximity_domain;
            if (node_id >= numa_node_count)
                break;

            struct numa_node *n = &topo_global.numa_nodes[node_id];
            n->mem_base = mem->address;
            n->mem_size = mem->length;
            break;
        }

        default: break;
        }

        ptr += entry->length;
        remaining -= entry->length;
    }
}
