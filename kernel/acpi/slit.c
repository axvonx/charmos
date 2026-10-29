#include <acpi/acpi.h>
#include <log.h>
#include <mem/numa.h>
#include <topo/global.h>
#include <uacpi/tables.h>

#include "uacpi/acpi.h"
#include "uacpi/status.h"

LOG_HANDLE_DEFINE_PRINT(slit);

void slit_init(void) {
    struct uacpi_table slit_table;
    size_t nodes = topo_global.numa_node_count;

    if (uacpi_table_find_by_signature("SLIT", &slit_table) != UACPI_STATUS_OK) {
        log_warn_global(LOG_HANDLE(slit),
                        "SLIT table not found, assuming uniform distances");

        /* distances_cnt is 0 with no SRAT, and `distance` is NULL there */
        for (size_t i = 0; i < nodes; i++) {
            struct numa_node *node = &topo_global.numa_nodes[i];
            for (size_t j = 0; j < node->distances_cnt; j++)
                node->distance[j] = (i == j) ? 10 : 20;
        }
    } else {
        struct acpi_slit *slit = slit_table.ptr;
        size_t localities = slit->num_localities;

        if (localities != nodes) {
            log_warn_global(LOG_HANDLE(slit),
                            "Mismatch in SLIT nodes vs detected NUMA nodes");
        }

        /* The matrix is localities x localities, only detected nodes get rows
         */
        uint8_t *entry = slit->matrix;

        for (size_t i = 0; i < nodes && i < localities; i++) {
            struct numa_node *node = &topo_global.numa_nodes[i];
            for (size_t j = 0; j < node->distances_cnt && j < localities; j++)
                node->distance[j] = entry[i * localities + j];
        }
    }

    /* Every consumer of rel_dists treats it as present,
     * and missing SLIT can happen */
    for (size_t i = 0; i < nodes; i++) {
        if (topo_global.numa_nodes[i].distances_cnt)
            numa_construct_relative_distances(&topo_global.numa_nodes[i]);
    }

    numa_dump();
}
