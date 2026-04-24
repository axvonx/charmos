/* @title: DMAR */
#pragma once
#include <console/panic.h>
#include <console/printf.h>
#include <log.h>
#include <math/bit.h>
#include <uacpi/tables.h>

#include "uacpi/acpi.h"

LOG_HANDLE_EXTERN(dmar);
LOG_SITE_EXTERN(dmar);
#define dmar_log(lvl, fmt, ...)                                                \
    log(LOG_SITE(dmar), LOG_HANDLE(dmar), lvl, fmt, ##__VA_ARGS__)

#define dmar_err(fmt, ...) dmar_log(LOG_ERROR, fmt, ##__VA_ARGS__)
#define dmar_warn(fmt, ...) dmar_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define dmar_info(fmt, ...) dmar_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define dmar_debug(fmt, ...) dmar_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define dmar_trace(fmt, ...) dmar_log(LOG_TRACE, fmt, ##__VA_ARGS__)

struct dmar_header {
    struct acpi_sdt_hdr header;
    uint8_t host_address_width;
    uint8_t flags;
    uint8_t reserved[10];
    /* remapping structures follow immediately */
};

struct dmar_remapping_struct {
    uint16_t type;
    uint16_t length;
};

#define DMAR_TYPE_DRHD 0 /* DMA Remapping Hardware Unit Definition */
#define DMAR_TYPE_RMRR 1 /* Reserved Memory Region Reporting       */
#define DMAR_TYPE_ATSR 2 /* ATS (Address Translation Services) cap */
#define DMAR_TYPE_RHSA 3 /* Remapping Hardware Status Affinity     */
#define DMAR_TYPE_ANDD 4 /* ACPI Namespace Device Declaration      */

#define DRHD_FLAG_INCLUDE_ALL (1u << 0)

struct dmar_device_scope {
    uint8_t type;   /* 1=PCI endpoint, 2=PCI sub-hier, 3=IOAPIC, 4=HPET */
    uint8_t length; /* includes path entries at the end */
    uint16_t reserved;
    uint8_t enum_id;   /* IOAPIC/HPET ID, or 0 for PCI devices */
    uint8_t start_bus; /* bus number where path starts          */
    /* followed by (length - 6) / 2 path entries: { uint8_t dev, uint8_t fn } */
};

/* device scope type values */
#define SCOPE_TYPE_PCI_ENDPOINT 1
#define SCOPE_TYPE_PCI_BRIDGE 2
#define SCOPE_TYPE_IOAPIC 3
#define SCOPE_TYPE_HPET 4

struct dmar_drhd { /* DRHD: DMA Remapping Hardware Unit Definition */
    uint16_t type; /* DMAR_TYPE_DRHD = 0 */
    uint16_t length;
    uint8_t flags;      /* bit 0 = INCLUDE_ALL */
    uint8_t size;       /* register set size: 2^(size+12) bytes  */
    uint16_t segment;   /* PCI segment number                    */
    uint64_t base_addr; /* MMIO base of this IOMMU unit          */
    /* followed by device scope entries */
};

struct dmar_rmrr { /* RMRR: Reserved Memory Region Reporting */
    uint16_t type; /* DMAR_TYPE_RMRR = 1 */
    uint16_t length;
    uint16_t reserved;
    uint16_t segment;
    uint64_t base_addr;  /* region base (page-aligned)            */
    uint64_t limit_addr; /* region limit (last byte, inclusive)   */
    /* followed by device scope entries */
};

struct dmar_atsr { /* ATSR: ATS (Address Translation Services) cap */
    uint16_t type; /* DMAR_TYPE_ATSR = 2 */
    uint16_t length;
    uint8_t flags; /* bit 0 = ALL_PORTS */
    uint8_t reserved;
    uint16_t segment;
    /* followed by device scope entries (only if ALL_PORTS not set) */
};

static inline const char *scope_type_name(uint8_t t) {
    switch (t) {
    case SCOPE_TYPE_PCI_ENDPOINT: return "PCI endpoint";
    case SCOPE_TYPE_PCI_BRIDGE: return "PCI bridge";
    case SCOPE_TYPE_IOAPIC: return "IOAPIC";
    case SCOPE_TYPE_HPET: return "HPET";
    default: return "unknown";
    }
}
