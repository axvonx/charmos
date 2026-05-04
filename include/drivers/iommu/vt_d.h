/* @title: VT-d */
#pragma once
#include <console/panic.h>
#include <console/printf.h>
#include <drivers/iommu/iommu.h>
#include <log.h>
#include <math/bit.h>
LOG_SITE_EXTERN(vtd);
LOG_HANDLE_EXTERN(vtd);

#define vtd_log(lvl, fmt, ...)                                                 \
    log(LOG_SITE(vtd), LOG_HANDLE(vtd), lvl, fmt, ##__VA_ARGS__)

#define vtd_err(fmt, ...) vtd_log(LOG_ERROR, fmt, ##__VA_ARGS__)
#define vtd_warn(fmt, ...) vtd_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define vtd_info(fmt, ...) vtd_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define vtd_debug(fmt, ...) vtd_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define vtd_trace(fmt, ...) vtd_log(LOG_TRACE, fmt, ##__VA_ARGS__)

/*
 * Register-name prefixes used below mirror the names in the Intel VT-d spec
 * so that bit-field macros can be cross-referenced with the spec text.
 *   CAP    - Capability Register
 *   ECAP   - Extended Capability Register
 *   GCMD   - Global Command Register
 *   GSTS   - Global Status Register
 *   RTADDR - Root Table Address Register
 *   FSTS   - Fault Status Register
 *   IQA    - Invalidation Queue Address Register
 */

struct vtd_regs {
    uint32_t version; /* 0x000 - Version                    */
    uint32_t _res0;
    uint64_t capabilities;          /* 0x008 - Capability                 */
    uint64_t extended_capabilities; /* 0x010 - Extended Capability        */
    uint32_t global_command;        /* 0x018 - Global Command             */
    uint32_t global_status;         /* 0x01C - Global Status              */
    uint64_t root_table_addr;       /* 0x020 - Root Table Address         */
    uint64_t context_command;       /* 0x028 - Context Command            */
    uint32_t _res1;
    uint32_t fault_status;           /* 0x034 - Fault Status               */
    uint32_t fault_event_control;    /* 0x038 - Fault Event Control        */
    uint32_t fault_event_data;       /* 0x03C - Fault Event Data           */
    uint32_t fault_event_addr;       /* 0x040 - Fault Event Address        */
    uint32_t fault_event_addr_upper; /* 0x044 - Fault Event Upper Address*/
    uint64_t _res2[2];
    uint64_t advanced_fault_log; /* 0x058 - Advanced Fault Log         */
    uint32_t _res3;
    uint32_t protected_memory_enable;  /* 0x064 - Protected Memory Enable   */
    uint32_t protected_low_mem_base;   /* 0x068 - Protected Low Mem Base    */
    uint32_t protected_low_mem_limit;  /* 0x06C - Protected Low Mem Limit   */
    uint64_t protected_high_mem_base;  /* 0x070 - Protected High Mem Base   */
    uint64_t protected_high_mem_limit; /* 0x078 - Protected High Mem Limit  */
    uint64_t invalidation_queue_head;  /* 0x080 - Invalidation Queue Head   */
    uint64_t invalidation_queue_tail;  /* 0x088 - Invalidation Queue Tail   */
    uint64_t invalidation_queue_addr;  /* 0x090 - Invalidation Queue Addr   */
    uint32_t _res4;
    uint32_t
        invalidation_comp_status; /* 0x09C - Invalidation Completion Status */
    uint32_t invalidation_event_ctrl; /* 0x0A0 - Invalidation Event Control */
    uint32_t invalidation_event_data; /* 0x0A4 - Invalidation Event Data */
    uint32_t invalidation_event_addr; /* 0x0A8 - Invalidation Event Address */
    uint32_t invalidation_event_addr_upper; /* 0x0AC - Invalidation Event Upper
                                               Addr */
    uint64_t invalidation_queue_error_record; /* 0x0B0 - IQ Error Record */
    uint64_t interrupt_remapping_table_addr; /* 0x0B8 - IR Table Address      */
} __packed;

_Static_assert(offsetof(struct vtd_regs, invalidation_queue_head) == 0x080,
               "vtd_regs layout mismatch");
_Static_assert(offsetof(struct vtd_regs, interrupt_remapping_table_addr) ==
                   0x0B8,
               "vtd_regs layout mismatch");
_Static_assert(offsetof(struct vtd_regs, fault_status) == 0x034,
               "vtd_regs layout mismatch");

struct vtd_invl_queue {};

struct vtd_unit {
    struct vtd_regs *regs;
    uint64_t cap;
    uint64_t ecap;
    uint16_t segment;
    uint32_t domain_count;

    uint64_t *root_table;
    paddr_t root_table_phys;

    void *iq_base;
    paddr_t iq_phys;
    uint32_t iq_head;
    uint32_t iq_tail;
    uint32_t iq_size;
};

struct vtd_domain {
    uint16_t domain_id;
    uint64_t *sl_pgd;
    uint64_t sl_pgd_phys;
};

/*
 * CAP - Capability Register (64-bit, offset 0x008)
 * Intel VT-d spec 11.4.2
 */
#define CAP_NUM_DOMAINS(cap)                                                   \
    BIT_RANGE((cap), 0, 2) /* ND     - Number of Domains supported (encoded)   \
                            */
#define CAP_REQUIRES_WRITE_BUF_FLUSH(cap)                                      \
    BIT_RANGE((cap), 4, 4) /* RWBF   - Required Write-Buffer Flushing */
#define CAP_PROTECTED_LOW_MEMORY(cap)                                          \
    BIT_RANGE((cap), 5, 5) /* PLMR   - Protected Low-Memory Region */
#define CAP_PROTECTED_HIGH_MEMORY(cap)                                         \
    BIT_RANGE((cap), 6, 6) /* PHMR   - Protected High-Memory Region */
#define CAP_CACHING_MODE(cap) BIT_RANGE((cap), 7, 7) /* CM */
#define CAP_SUPPORTED_ADDR_WIDTHS(cap)                                         \
    BIT_RANGE((cap), 8,                                                        \
              12) /* SAGAW  - Supported Adjusted Guest Address Widths */
#define CAP_MAX_ADDR_WIDTH(cap)                                                \
    BIT_RANGE((cap), 16, 21) /* MGAW   - Maximum Guest Address Width */
#define CAP_ZERO_LENGTH_READ(cap) BIT_RANGE((cap), 22, 22) /* ZLR */
#define CAP_FAULT_RECORD_OFFSET(cap)                                           \
    BIT_RANGE((cap), 24, 33) /* FRO    - Fault-Recording Register Offset */
#define CAP_LARGE_PAGE_SUPPORT(cap)                                            \
    BIT_RANGE((cap), 34, 37) /* SLLPS  - Second-Level Large Page Support */
#define CAP_PAGE_SELECTIVE_INVALIDATION(cap)                                   \
    BIT_RANGE((cap), 39, 39) /* PSI                                            \
                              */
#define CAP_NUM_FAULT_RECORDS(cap)                                             \
    BIT_RANGE((cap), 40, 47) /* NFR    - Number of Fault-Recording Registers   \
                              */
#define CAP_MAX_ADDR_MASK_VALUE(cap) BIT_RANGE((cap), 48, 53)  /* MAMV */
#define CAP_WRITE_DRAINING(cap) BIT_RANGE((cap), 54, 54)       /* DWD */
#define CAP_READ_DRAINING(cap) BIT_RANGE((cap), 55, 55)        /* DRD */
#define CAP_FIRST_LEVEL_1GB_PAGE(cap) BIT_RANGE((cap), 56, 56) /* FL1GP */
#define CAP_POSTED_INTERRUPTS(cap) BIT_RANGE((cap), 59, 59)    /* PI */
#define CAP_FIRST_STAGE_5LEVEL_PAGING(cap)                                     \
    BIT_RANGE((cap), 60, 60) /* FL5LP / FS5LP */
#define CAP_ENHANCED_COMMAND_SUPPORT(cap) BIT_RANGE((cap), 61, 61) /* ECMDS */
#define CAP_ENHANCED_SET_INTR_REMAP_TABLE_PTR_SUPPORT(cap)                     \
    BIT_RANGE(                                                                 \
        (cap), 62,                                                             \
        62) /* ESIRTPS - Enhanced Set Interrupt Remap Table Pointer Support */
#define CAP_ENHANCED_SET_ROOT_TABLE_PTR_SUPPORT(cap)                           \
    BIT_RANGE((cap), 63,                                                       \
              63) /* ESRTPS  - Enhanced Set Root Table Pointer Support */

/*
 * ECAP - Extended Capability Register (64-bit, offset 0x010)
 * Intel VT-d spec 11.4.3
 */
#define ECAP_COHERENCY(ecap) BIT_RANGE((ecap), 0, 0)               /* C */
#define ECAP_QUEUED_INVALIDATION(ecap) BIT_RANGE((ecap), 1, 1)     /* QI */
#define ECAP_DEVICE_TLB(ecap) BIT_RANGE((ecap), 2, 2)              /* DT */
#define ECAP_INTERRUPT_REMAPPING(ecap) BIT_RANGE((ecap), 3, 3)     /* IR */
#define ECAP_EXTENDED_INTERRUPT_MODE(ecap) BIT_RANGE((ecap), 4, 4) /* EIM */
#define ECAP_PASS_THROUGH(ecap) BIT_RANGE((ecap), 6, 6)            /* PT */
#define ECAP_SNOOP_CONTROL(ecap) BIT_RANGE((ecap), 7, 7)           /* SC */
#define ECAP_IOTLB_REGISTER_OFFSET(ecap) BIT_RANGE((ecap), 8, 17)  /* IRO */
#define ECAP_MAX_HANDLE_MASK_VALUE(ecap) BIT_RANGE((ecap), 20, 23) /* MHMV */
#define ECAP_MEMORY_TYPE_SUPPORT(ecap) BIT_RANGE((ecap), 25, 25)   /* MTS */
#define ECAP_NESTED_TRANSLATION(ecap) BIT_RANGE((ecap), 26, 26)    /* NEST */
#define ECAP_PAGE_REQUEST_SUPPORT(arg) BIT_TEST((arg), 29)         /* PRS */
#define ECAP_EXECUTE_REQUEST_SUPPORT(arg) BIT_TEST((arg), 30)      /* ERS */
#define ECAP_SUPERVISOR_REQUEST_SUPPORT(arg) BIT_TEST((arg), 31)   /* SRS */
#define ECAP_NO_WRITE_FLAG_SUPPORT(arg) BIT_TEST((arg), 33)        /* NWFS */
#define ECAP_EXTENDED_ACCESSED_FLAG_SUPPORT(arg)                               \
    BIT_TEST((arg), 34) /* EAFS                                                \
                         */
#define ECAP_PASID_SIZE_SUPPORTED(arg)                                         \
    BIT_RANGE((arg), 35, 39) /* PSS - Process Address Space ID size */
#define ECAP_PASID_SUPPORT(arg) BIT_TEST((arg), 40) /* PASID */
#define ECAP_DEVICE_TLB_INVAL_THROTTLE(arg)                                    \
    BIT_TEST((arg), 41)                                          /* DIT        \
                                                                  */
#define ECAP_PAGE_REQUEST_DRAIN_SUPPORT(arg) BIT_TEST((arg), 42) /* PDS */
#define ECAP_SCALABLE_MODE_TRANSLATION_SUPPORT(arg)                            \
    BIT_TEST((arg), 43)                                       /* SMTS */
#define ECAP_VIRTUAL_COMMAND_SUPPORT(arg) BIT_TEST((arg), 44) /* VCS */
#define ECAP_SECOND_STAGE_ACCESSED_DIRTY_SUPPORT(arg)                          \
    BIT_TEST((arg), 45) /* SSADS */
#define ECAP_SECOND_STAGE_TRANSLATION_SUPPORT(arg)                             \
    BIT_TEST((arg), 46) /* SSTS */
#define ECAP_FIRST_STAGE_TRANSLATION_SUPPORT(arg)                              \
    BIT_TEST((arg), 47) /* FSTS (do not confuse with Fault Status register) */
#define ECAP_SCALABLE_MODE_PAGE_WALK_COHERENCY_SUPPORT(arg)                    \
    BIT_TEST((arg), 48) /* SMPWCS */
#define ECAP_RID_PASID_SUPPORT(arg)                                            \
    BIT_TEST((arg), 49) /* RPS - Requester-ID/PASID */
#define ECAP_PERFORMANCE_MONITORING_SUPPORT(arg) BIT_TEST((arg), 51) /* PMS */
#define ECAP_ABORT_DMA_MODE_SUPPORT(arg) BIT_TEST((arg), 52)         /* ADMS */
#define ECAP_RID_PRIV_SUPPORT(arg)                                             \
    BIT_TEST((arg), 53) /* RPRIVS - Requester-ID Privilege */
#define ECAP_STOP_MARKER_SUPPORT(arg) BIT_TEST((arg), 58) /* SMS */

/*
 * Bit positions in CAP_SUPPORTED_ADDR_WIDTHS (CAP.SAGAW)
 * Each bit indicates a supported guest address width / page-table depth.
 */
#define ADDR_WIDTH_39BIT BIT(1) /* 3-level page table */
#define ADDR_WIDTH_48BIT BIT(2) /* 4-level page table */
#define ADDR_WIDTH_57BIT BIT(3) /* 5-level page table */

/*
 * GCMD - Global Command Register (32-bit, offset 0x018)
 * GSTS - Global Status Register  (32-bit, offset 0x01C)
 * Intel VT-d spec 11.4.4, 11.4.5
 */
#define GCMD_TRANSLATION_ENABLE BIT(31)  /* TE */
#define GCMD_SET_ROOT_TABLE_PTR BIT(30)  /* SRTP   - Set Root Table Pointer */
#define GCMD_WRITE_BUFFER_FLUSH BIT(27)  /* WBF */
#define GCMD_QUEUED_INVAL_ENABLE BIT(26) /* QIE */
#define GCMD_INTERRUPT_REMAP_ENABLE BIT(25) /* IRE */
#define GCMD_SET_INTR_REMAP_TABLE_PTR                                          \
    BIT(24) /* SIRTP  - Set Interrupt Remap Table Pointer */
#define GCMD_COMPAT_FORMAT_INTERRUPT                                           \
    BIT(23) /* CFI    - Compatibility Format Interrupt */

#define GSTS_TRANSLATION_ENABLED BIT(31)            /* TES */
#define GSTS_ROOT_TABLE_PTR_SET BIT(30)             /* RTPS */
#define GSTS_WRITE_BUFFER_FLUSH_STATUS BIT(27)      /* WBFS */
#define GSTS_QUEUED_INVAL_ENABLED BIT(26)           /* QIES */
#define GSTS_INTERRUPT_REMAP_ENABLED BIT(25)        /* IRES */
#define GSTS_INTR_REMAP_TABLE_PTR_SET BIT(24)       /* IRTPS */
#define GSTS_COMPAT_FORMAT_INTERRUPT_STATUS BIT(23) /* CFIS */

/*
 * RTADDR - Root Table Address Register (64-bit, offset 0x020)
 * TTM = Translation Table Mode (bits 10-11).
 */
#define RTADDR_TRANSLATION_MODE_LEGACY (0ULL << 10)
#define RTADDR_TRANSLATION_MODE_SCALABLE (1ULL << 10)

/*
 * FSTS - Fault Status Register (32-bit, offset 0x034)
 */
#define FSTS_PRIMARY_FAULT_OVERFLOW BIT(0)               /* PFO */
#define FSTS_PRIMARY_PENDING_FAULT BIT(1)                /* PPF */
#define FSTS_ADVANCED_FAULT_OVERFLOW BIT(2)              /* AFO */
#define FSTS_ADVANCED_PENDING_FAULT BIT(3)               /* APF */
#define FSTS_INVAL_QUEUE_ERROR BIT(4)                    /* IQE */
#define FSTS_INVAL_COMPLETION_ERROR BIT(5)               /* ICE */
#define FSTS_INVAL_TIMEOUT_ERROR BIT(6)                  /* ITE */
#define FSTS_PAGE_REQUEST_OVERFLOW BIT(7)                /* PRO */
#define FSTS_FAULT_RECORD_INDEX(s) BIT_RANGE((s), 8, 15) /* FRI */

/*
 * Root Entry (128-bit / two 64-bit words) - Intel VT-d spec 9.1
 * CTP = Context-Table Pointer (bits 12-63 of the lo word).
 */
#define ROOT_ENTRY_PRESENT BIT(0)
#define ROOT_ENTRY_CONTEXT_TABLE_PTR_SHIFT 12
#define ROOT_ENTRY_CONTEXT_TABLE_PTR_MASK (~0xFFFULL)

#define ROOT_ENTRY_SET_CONTEXT_TABLE_PTR(phys)                                 \
    (((phys) & ROOT_ENTRY_CONTEXT_TABLE_PTR_MASK) | ROOT_ENTRY_PRESENT)

struct vtd_root_entry {
    uint64_t lo;
    uint64_t hi; /* must be zero */
} __packed;

_Static_assert(sizeof(struct vtd_root_entry) == 16, "root entry must be 16B");

/*
 * Context Entry (128-bit / two 64-bit words) - Intel VT-d spec 9.3
 * Field acronyms used as suffixes:
 *   FPD     - Fault Processing Disable
 *   TT      - Translation Type
 *   SLPTPTR - Second-Level Page-Table Pointer
 *   AW      - Address Width
 *   DID     - Domain Identifier
 */
#define CTX_ENTRY_PRESENT BIT(0)
#define CTX_ENTRY_FAULT_PROCESSING_DISABLE BIT(1)
#define CTX_ENTRY_TRANSLATION_TYPE_SHIFT 2
#define CTX_ENTRY_TRANSLATION_TYPE_MASK BIT_MASK(2, 3)
#define CTX_ENTRY_TRANSLATION_TYPE_UNTRANSLATED (0ULL << 2)
#define CTX_ENTRY_TRANSLATION_TYPE_PASSTHROUGH (2ULL << 2)
#define CTX_ENTRY_SL_PTR_SHIFT 12
#define CTX_ENTRY_SL_PTR_MASK (~0xFFFULL)

#define CTX_ENTRY_ADDR_WIDTH_SHIFT 0
#define CTX_ENTRY_ADDR_WIDTH_39BIT 1
#define CTX_ENTRY_ADDR_WIDTH_48BIT 2
#define CTX_ENTRY_ADDR_WIDTH_57BIT 3

#define CTX_ENTRY_DOMAIN_ID_SHIFT 8
#define CTX_ENTRY_DOMAIN_ID_MASK BIT_MASK(8, 23)

#define CTX_ENTRY_SET_LO(slptptr, tt)                                          \
    (((slptptr) & CTX_ENTRY_SL_PTR_MASK) | (tt) | CTX_ENTRY_PRESENT)
#define CTX_ENTRY_SET_HI(domain_id, aw)                                        \
    (((uint64_t) (domain_id) << CTX_ENTRY_DOMAIN_ID_SHIFT) | (aw))

/* Extract fields from a context entry */
#define CTX_ENTRY_TRANSLATION_TYPE(lo) BIT_RANGE((lo), 2, 3)
#define CTX_ENTRY_DOMAIN_ID(hi) BIT_RANGE((hi), 8, 23)
#define CTX_ENTRY_ADDR_WIDTH(hi) BIT_RANGE((hi), 0, 2)

struct vtd_context_entry {
    uint64_t lo;
    uint64_t hi;
} __packed;

_Static_assert(sizeof(struct vtd_context_entry) == 16, "ctx entry must be 16B");

/*
 * Second-Level Page-Table Entry (PTE).
 * "SL" prefix abbreviates "Second-Level". Bit-field acronyms inlined below.
 */
#define SL_PTE_READ BIT(0)
#define SL_PTE_WRITE BIT(1)
#define SL_PTE_PRESENT                                               \
    (SL_PTE_READ | SL_PTE_WRITE)
#define SL_PTE_EXECUTE BIT(2)
#define SL_PTE_MEMORY_TYPE_SHIFT 3 /* EMT - Extended Memory Type */
#define SL_PTE_MEMORY_TYPE_WRITE_BACK (6ULL << 3)  /* WB */
#define SL_PTE_MEMORY_TYPE_UNCACHEABLE (0ULL << 3) /* UC */
#define SL_PTE_IGNORE_PAT                                            \
    BIT(6) /* IPAT - Ignore Page Attribute Table */
#define SL_PTE_LARGE_PAGE BIT(7) /* PS - Page Size */
#define SL_PTE_SNOOP BIT(11)     /* SNP */
#define SL_PTE_ADDR_MASK (~0xFFFULL & ((1ULL << 52) - 1))

#define SL_PTE_ADDR(pte) ((pte) & SL_PTE_ADDR_MASK)
#define SL_PTE_MEMORY_TYPE(pte) BIT_RANGE((pte), 3, 5)
#define SL_PTE_IS_LARGE(pte) BIT_RANGE((pte), 7, 7)

#define SL_TABLE_ENTRY(phys)                                         \
    (((phys) & SL_PTE_ADDR_MASK) | SL_PTE_PRESENT)

#define SL_PAGE_ENTRY(phys, perm)                                    \
    (((phys) & SL_PTE_ADDR_MASK) |                                   \
     SL_PTE_MEMORY_TYPE_WRITE_BACK | SL_PTE_IGNORE_PAT |   \
     ((perm) & 0x3))

/*
 * IOVA index helpers, identical breakdown to x86-64 VA.
 * PML4 / PDPT / PD / PT are the standard x86-64 page-table levels.
 */
#define SL_PML4_INDEX(iova) (((iova) >> 39) & 0x1FF)
#define SL_PDPT_INDEX(iova) (((iova) >> 30) & 0x1FF)
#define SL_PD_INDEX(iova) (((iova) >> 21) & 0x1FF)
#define SL_PT_INDEX(iova) (((iova) >> 12) & 0x1FF)
#define SL_PAGE_OFFSET(iova) ((iova) & 0xFFF)

/*
 * IQA - Invalidation Queue Address Register, and queue layout
 * Intel VT-d spec 6.5
 */
#define IQA_SIZE_SHIFT 0
#define IQA_SIZE_256_ENTRIES 0
#define IQA_SIZE_512_ENTRIES 1
#define IQA_SIZE_1K_ENTRIES 2
#define IQA_SIZE_2K_ENTRIES 3
#define IQA_SIZE_4K_ENTRIES 4
#define IQA_SIZE_8K_ENTRIES 5
#define IQA_SIZE_16K_ENTRIES 6
#define IQA_SIZE_32K_ENTRIES 7
#define IQA_ADDR_MASK (~0xFFFULL)

#define IQA_REG_BUILD(phys, size_enc) (((phys) & IQA_ADDR_MASK) | (size_enc))
#define IQA_SIZE(iqa) BIT_RANGE((iqa), 0, 2)

#define INVAL_QUEUE_ENTRY_SIZE 16
#define INVAL_QUEUE_TAIL_SHIFT 4

/* Invalidation descriptor type codes (low 4 bits of descriptor.lo) */
#define INVAL_DESC_TYPE_CONTEXT_CACHE 0x1
#define INVAL_DESC_TYPE_IOTLB                                                  \
    0x2 /* I/O Translation Lookaside Buffer                                    \
         */
#define INVAL_DESC_TYPE_DEVICE_IOTLB 0x3
#define INVAL_DESC_TYPE_INTERRUPT_ENTRY_CACHE 0x4 /* IEC */
#define INVAL_DESC_TYPE_WAIT 0x5

#define INVAL_DESC_TYPE(lo) BIT_RANGE((lo), 0, 3)
#define INVAL_DESC_GRANULARITY(lo) BIT_RANGE((lo), 4, 5)

/* Context-cache invalidation descriptor */
#define CONTEXT_INVAL_GRANULARITY_GLOBAL (1ULL << 4)
#define CONTEXT_INVAL_GRANULARITY_DOMAIN (2ULL << 4)
#define CONTEXT_INVAL_GRANULARITY_DEVICE (3ULL << 4)
#define CONTEXT_INVAL_DOMAIN_ID_SHIFT 48

#define CONTEXT_INVAL_DESC_GLOBAL                                              \
    ((struct vtd_inv_desc) {.lo = INVAL_DESC_TYPE_CONTEXT_CACHE |              \
                                  CONTEXT_INVAL_GRANULARITY_GLOBAL,            \
                            .hi = 0})
#define CONTEXT_INVAL_DESC_DOMAIN(did)                                         \
    ((struct vtd_inv_desc) {                                                   \
        .lo = INVAL_DESC_TYPE_CONTEXT_CACHE |                                  \
              CONTEXT_INVAL_GRANULARITY_DOMAIN |                               \
              ((uint64_t) (did) << CONTEXT_INVAL_DOMAIN_ID_SHIFT),             \
        .hi = 0})

/* IOTLB invalidation descriptor */
#define IOTLB_INVAL_GRANULARITY_GLOBAL (1ULL << 4)
#define IOTLB_INVAL_GRANULARITY_DOMAIN (2ULL << 4)
#define IOTLB_INVAL_GRANULARITY_PAGE (3ULL << 4)
#define IOTLB_INVAL_DRAIN_READS BIT(7)  /* DR */
#define IOTLB_INVAL_DRAIN_WRITES BIT(6) /* DW */
#define IOTLB_INVAL_DOMAIN_ID_SHIFT 32
#define IOTLB_INVAL_IOVA_SHIFT 12
#define IOTLB_INVAL_ADDR_MASK_FIELD_MASK                                       \
    0x3FULL /* AM (Address Mask) field width */
#define IOTLB_INVAL_HINT                                                       \
    BIT(6) /* IH (in hi dword): non-leaf entries unchanged */

#define IOTLB_INVAL_DOMAIN_ID(lo) BIT_RANGE((lo), 32, 47)
#define IOTLB_INVAL_GRANULARITY(lo) BIT_RANGE((lo), 4, 5)

#define IOTLB_INVAL_DESC_GLOBAL                                                \
    ((struct vtd_inv_desc) {                                                   \
        .lo = INVAL_DESC_TYPE_IOTLB | IOTLB_INVAL_GRANULARITY_GLOBAL |         \
              IOTLB_INVAL_DRAIN_READS | IOTLB_INVAL_DRAIN_WRITES,              \
        .hi = 0})
#define IOTLB_INVAL_DESC_DOMAIN(did)                                           \
    ((struct vtd_inv_desc) {                                                   \
        .lo = INVAL_DESC_TYPE_IOTLB | IOTLB_INVAL_GRANULARITY_DOMAIN |         \
              IOTLB_INVAL_DRAIN_READS | IOTLB_INVAL_DRAIN_WRITES |             \
              ((uint64_t) (did) << IOTLB_INVAL_DOMAIN_ID_SHIFT),               \
        .hi = 0})
#define IOTLB_INVAL_DESC_PAGE(did, iova, am)                                   \
    ((struct vtd_inv_desc) {                                                   \
        .lo = INVAL_DESC_TYPE_IOTLB | IOTLB_INVAL_GRANULARITY_PAGE |           \
              IOTLB_INVAL_DRAIN_READS | IOTLB_INVAL_DRAIN_WRITES |             \
              ((uint64_t) (did) << IOTLB_INVAL_DOMAIN_ID_SHIFT),               \
        .hi =                                                                  \
            ((iova) & ~0xFFFULL) | ((am) & IOTLB_INVAL_ADDR_MASK_FIELD_MASK)})

/* Wait descriptor */
#define WAIT_DESC_INTERRUPT_FLAG BIT(4) /* IF */
#define WAIT_DESC_STATUS_WRITE BIT(5)   /* SW */
#define WAIT_DESC_FENCE BIT(6)          /* FN */
#define WAIT_DESC_DATA_SHIFT 32

#define WAIT_DESC(status_phys, status_data)                                    \
    ((struct vtd_inv_desc) {                                                   \
        .lo = INVAL_DESC_TYPE_WAIT | WAIT_DESC_STATUS_WRITE |                  \
              WAIT_DESC_FENCE |                                                \
              ((uint64_t) (status_data) << WAIT_DESC_DATA_SHIFT),              \
        .hi = (status_phys)})

struct vtd_inv_desc {
    uint64_t lo;
    uint64_t hi;
} __packed;

_Static_assert(sizeof(struct vtd_inv_desc) == 16, "inv desc must be 16B");

extern const struct iommu_ops vtd_iommu_ops;

struct iommu *vtd_unit_create(uint64_t base_phys, uint16_t segment,
                              uint8_t size_field);

/* ND encoding to actual domain count */
static inline uint32_t vtd_cap_domain_count(uint64_t cap) {
    static const uint32_t nd_table[] = {16,   64,    256,   1024,
                                        4096, 16384, 65536, 0};
    return nd_table[CAP_NUM_DOMAINS(cap)];
}
