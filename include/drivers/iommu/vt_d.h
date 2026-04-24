/* @title: VT-d */
#pragma once
#include <console/panic.h>
#include <console/printf.h>
#include <drivers/iommu/iommu.h>
#include <log.h>
#include <math/bit.h>

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
    volatile struct vtd_regs *regs;
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
 * CAP_REG - Capability Register (64-bit, offset 0x008)
 * Intel VT-d spec 11.4.2
 */
#define CAP_ND(cap) BIT_RANGE((cap), 0, 2)
#define CAP_RWBF(cap) BIT_RANGE((cap), 4, 4)
#define CAP_PLMR(cap) BIT_RANGE((cap), 5, 5)
#define CAP_PHMR(cap) BIT_RANGE((cap), 6, 6)
#define CAP_CM(cap) BIT_RANGE((cap), 7, 7)
#define CAP_SAGAW(cap) BIT_RANGE((cap), 8, 12)
#define CAP_MGAW(cap) BIT_RANGE((cap), 16, 21)
#define CAP_ZLR(cap) BIT_RANGE((cap), 22, 22)
#define CAP_FRO(cap) BIT_RANGE((cap), 24, 33)
#define CAP_SLLPS(cap) BIT_RANGE((cap), 34, 37)
#define CAP_PSI(cap) BIT_RANGE((cap), 39, 39)
#define CAP_NFR(cap) BIT_RANGE((cap), 40, 47)
#define CAP_MAMV(cap) BIT_RANGE((cap), 48, 53)
#define CAP_DWD(cap) BIT_RANGE((cap), 54, 54)
#define CAP_DRD(cap) BIT_RANGE((cap), 55, 55)
#define CAP_FL1GP(cap) BIT_RANGE((cap), 56, 56)

/*
 * ECAP_REG - Extended Capability Register (64-bit, offset 0x010)
 * Intel VT-d spec 11.4.3
 */
#define ECAP_C(ecap) BIT_RANGE((ecap), 0, 0)
#define ECAP_QI(ecap) BIT_RANGE((ecap), 1, 1)
#define ECAP_DT(ecap) BIT_RANGE((ecap), 2, 2)
#define ECAP_IR(ecap) BIT_RANGE((ecap), 3, 3)
#define ECAP_EIM(ecap) BIT_RANGE((ecap), 4, 4)
#define ECAP_PT(ecap) BIT_RANGE((ecap), 6, 6)
#define ECAP_SC(ecap) BIT_RANGE((ecap), 7, 7)
#define ECAP_IRO(ecap) BIT_RANGE((ecap), 8, 17)
#define ECAP_MHMV(ecap) BIT_RANGE((ecap), 20, 23)
#define ECAP_MTS(ecap) BIT_RANGE((ecap), 25, 25)
#define ECAP_NEST(ecap) BIT_RANGE((ecap), 26, 26)

/* SAGAW bit positions -> address widths / page-table levels */
#define SAGAW_39BIT BIT(1) /* 3-level page table */
#define SAGAW_48BIT BIT(2) /* 4-level page table */
#define SAGAW_57BIT BIT(3) /* 5-level page table */

/*
 * GCMD_REG - Global Command Register (32-bit, offset 0x018)
 * GSTS_REG - Global Status Register  (32-bit, offset 0x01C)
 * Intel VT-d spec 11.4.4, 5
 */
#define GCMD_TE BIT(31) /* Translation Enable */
#define GCMD_SRTP BIT(30) /* Set Root Table Pointer */
#define GCMD_SFL BIT(29) 
#define GCMD_EAFL BIT(28)
#define GCMD_CFI BIT(27)
#define GCMD_QIE BIT(26)
#define GCMD_IRE BIT(25)
#define GCMD_SIRTP BIT(24)

#define GSTS_TES BIT(31)
#define GSTS_RTPS BIT(30)
#define GSTS_FLS BIT(29)
#define GSTS_AFLS BIT(28)
#define GSTS_CFIS BIT(27)
#define GSTS_QIES BIT(26)
#define GSTS_IRES BIT(25)

/*
 * RTADDR_REG - Root Table Address Register (64-bit, offset 0x020)
 */
#define RTADDR_TTM_LEGACY (0ULL << 10)
#define RTADDR_TTM_SCALABLE (1ULL << 10)

/*
 * FSTS_REG - Fault Status Register (32-bit, offset 0x034)
 */
#define FSTS_PFO BIT(0)
#define FSTS_PPF BIT(1)
#define FSTS_AFO BIT(2)
#define FSTS_APF BIT(3)
#define FSTS_IQE BIT(4)
#define FSTS_ICE BIT(5)
#define FSTS_ITE BIT(6)
#define FSTS_PRO BIT(7)
#define FSTS_FRI(s) BIT_RANGE((s), 8, 15)

/*
 * Root Entry (128-bit / two 64-bit words) - Intel VT-d spec 9.1
 */
#define ROOT_ENTRY_PRESENT BIT(0)
#define ROOT_ENTRY_CTP_SHIFT 12
#define ROOT_ENTRY_CTP_MASK (~0xFFFULL)

#define ROOT_ENTRY_SET_CTP(phys)                                               \
    (((phys) & ROOT_ENTRY_CTP_MASK) | ROOT_ENTRY_PRESENT)

struct vtd_root_entry {
    uint64_t lo;
    uint64_t hi; /* must be zero */
} __packed;

_Static_assert(sizeof(struct vtd_root_entry) == 16, "root entry must be 16B");

/*
 * Context Entry (128-bit / two 64-bit words) - Intel VT-d spec §9.3
 */
#define CTX_ENTRY_PRESENT BIT(0)
#define CTX_ENTRY_FPD BIT(1)
#define CTX_ENTRY_TT_SHIFT 2
#define CTX_ENTRY_TT_MASK BIT_MASK(2, 3)
#define CTX_ENTRY_TT_UNTRANSLATED (0ULL << 2)
#define CTX_ENTRY_TT_PASSTHROUGH (2ULL << 2)
#define CTX_ENTRY_SLPTPTR_SHIFT 12
#define CTX_ENTRY_SLPTPTR_MASK (~0xFFFULL)

#define CTX_ENTRY_AW_SHIFT 0
#define CTX_ENTRY_AW_39BIT 1
#define CTX_ENTRY_AW_48BIT 2
#define CTX_ENTRY_AW_57BIT 3

#define CTX_ENTRY_DID_SHIFT 8
#define CTX_ENTRY_DID_MASK BIT_MASK(8, 23)

#define CTX_ENTRY_SET_LO(slptptr, tt)                                          \
    (((slptptr) & CTX_ENTRY_SLPTPTR_MASK) | (tt) | CTX_ENTRY_PRESENT)
#define CTX_ENTRY_SET_HI(domain_id, aw)                                        \
    (((uint64_t) (domain_id) << CTX_ENTRY_DID_SHIFT) | (aw))

/* Extract fields from a context entry */
#define CTX_ENTRY_TT(lo) BIT_RANGE((lo), 2, 3)
#define CTX_ENTRY_DID(hi) BIT_RANGE((hi), 8, 23)
#define CTX_ENTRY_AW(hi) BIT_RANGE((hi), 0, 2)

struct vtd_context_entry {
    uint64_t lo;
    uint64_t hi;
} __packed;

_Static_assert(sizeof(struct vtd_context_entry) == 16, "ctx entry must be 16B");

/*
 * Second-Level Page Table Entries - Intel VT-d spec 9.5
 */
#define SL_PTE_READ BIT(0)
#define SL_PTE_WRITE BIT(1)
#define SL_PTE_PRESENT (SL_PTE_READ | SL_PTE_WRITE)
#define SL_PTE_USER BIT(2)
#define SL_PTE_EMT_SHIFT 3
#define SL_PTE_EMT_WB (6ULL << 3)
#define SL_PTE_EMT_UC (0ULL << 3)
#define SL_PTE_IPAT BIT(6)
#define SL_PTE_PS BIT(7)
#define SL_PTE_SNP BIT(11)
#define SL_PTE_ADDR_MASK (~0xFFFULL & ((1ULL << 52) - 1))
#define SL_PTE_TM BIT(62)

#define SL_PTE_ADDR(pte) ((pte) & SL_PTE_ADDR_MASK)
#define SL_PTE_EMT(pte) BIT_RANGE((pte), 3, 5)
#define SL_PTE_IS_LARGE(pte) BIT_RANGE((pte), 7, 7)

#define SL_TABLE_ENTRY(phys) (((phys) & SL_PTE_ADDR_MASK) | SL_PTE_PRESENT)

#define SL_PAGE_ENTRY(phys, perm)                                              \
    (((phys) & SL_PTE_ADDR_MASK) | SL_PTE_EMT_WB | SL_PTE_IPAT | ((perm) & 0x3))

/* IOVA index helpers, identical breakdown to x86-64 VA */
#define SL_PML4_IDX(iova) (((iova) >> 39) & 0x1FF)
#define SL_PDPT_IDX(iova) (((iova) >> 30) & 0x1FF)
#define SL_PD_IDX(iova) (((iova) >> 21) & 0x1FF)
#define SL_PT_IDX(iova) (((iova) >> 12) & 0x1FF)
#define SL_PAGE_OFFSET(iova) ((iova) & 0xFFF)

/*
 * Invalidation Queue - Intel VT-d spec 6.5
 */
#define IQA_SIZE_SHIFT 0
#define IQA_SIZE_256 0
#define IQA_SIZE_512 1
#define IQA_SIZE_1K 2
#define IQA_SIZE_2K 3
#define IQA_SIZE_4K 4
#define IQA_SIZE_8K 5
#define IQA_SIZE_16K 6
#define IQA_SIZE_32K 7
#define IQA_ADDR_MASK (~0xFFFULL)

#define IQA_REG_BUILD(phys, size_enc) (((phys) & IQA_ADDR_MASK) | (size_enc))
#define IQA_SIZE(iqa) BIT_RANGE((iqa), 0, 2)

#define IQ_ENTRY_SIZE 16
#define IQ_TAIL_SHIFT 4

/* Descriptor type codes */
#define IQ_DESC_TYPE_CONTEXT_CACHE 0x1
#define IQ_DESC_TYPE_IOTLB 0x2
#define IQ_DESC_TYPE_DEVICE_IOTLB 0x3
#define IQ_DESC_TYPE_IEC 0x4
#define IQ_DESC_TYPE_WAIT 0x5

#define IQ_DESC_TYPE(lo) BIT_RANGE((lo), 0, 3)
#define IQ_DESC_GRAN(lo) BIT_RANGE((lo), 4, 5)

/* Context-cache invalidation descriptor */
#define CTX_INV_GRAN_GLOBAL (1ULL << 4)
#define CTX_INV_GRAN_DOMAIN (2ULL << 4)
#define CTX_INV_GRAN_DEVICE (3ULL << 4)
#define CTX_INV_DID_SHIFT 48

#define CTX_INV_DESC_GLOBAL()                                                  \
    ((struct vtd_inv_desc) {                                                   \
        .lo = IQ_DESC_TYPE_CONTEXT_CACHE | CTX_INV_GRAN_GLOBAL, .hi = 0})
#define CTX_INV_DESC_DOMAIN(did)                                               \
    ((struct vtd_inv_desc) {.lo = IQ_DESC_TYPE_CONTEXT_CACHE |                 \
                                  CTX_INV_GRAN_DOMAIN |                        \
                                  ((uint64_t) (did) << CTX_INV_DID_SHIFT),     \
                            .hi = 0})

/* IOTLB invalidation descriptor */
#define IOTLB_INV_GRAN_GLOBAL (1ULL << 4)
#define IOTLB_INV_GRAN_DOMAIN (2ULL << 4)
#define IOTLB_INV_GRAN_PAGE (3ULL << 4)
#define IOTLB_INV_DR BIT(7)
#define IOTLB_INV_DW BIT(6)
#define IOTLB_INV_DID_SHIFT 32
#define IOTLB_INV_IOVA_SHIFT 12
#define IOTLB_INV_AM_MASK 0x3FULL
#define IOTLB_INV_HINT BIT(6)

#define IOTLB_INV_DID(lo) BIT_RANGE((lo), 32, 47)
#define IOTLB_INV_GRAN(lo) BIT_RANGE((lo), 4, 5)

#define IOTLB_INV_DESC_GLOBAL()                                                \
    ((struct vtd_inv_desc) {.lo = IQ_DESC_TYPE_IOTLB | IOTLB_INV_GRAN_GLOBAL | \
                                  IOTLB_INV_DR | IOTLB_INV_DW,                 \
                            .hi = 0})
#define IOTLB_INV_DESC_DOMAIN(did)                                             \
    ((struct vtd_inv_desc) {.lo = IQ_DESC_TYPE_IOTLB | IOTLB_INV_GRAN_DOMAIN | \
                                  IOTLB_INV_DR | IOTLB_INV_DW |                \
                                  ((uint64_t) (did) << IOTLB_INV_DID_SHIFT),   \
                            .hi = 0})
#define IOTLB_INV_DESC_PAGE(did, iova, am)                                     \
    ((struct vtd_inv_desc) {                                                   \
        .lo = IQ_DESC_TYPE_IOTLB | IOTLB_INV_GRAN_PAGE | IOTLB_INV_DR |        \
              IOTLB_INV_DW | ((uint64_t) (did) << IOTLB_INV_DID_SHIFT),        \
        .hi = ((iova) & ~0xFFFULL) | ((am) & IOTLB_INV_AM_MASK)})

/* Wait descriptor */
#define WAIT_DESC_IF BIT(4)
#define WAIT_DESC_SW BIT(5)
#define WAIT_DESC_FN BIT(6)
#define WAIT_DESC_DATA_SHIFT 32

#define WAIT_DESC(status_phys, status_data)                                    \
    ((struct vtd_inv_desc) {                                                   \
        .lo = IQ_DESC_TYPE_WAIT | WAIT_DESC_SW | WAIT_DESC_FN |                \
              ((uint64_t) (status_data) << WAIT_DESC_DATA_SHIFT),              \
        .hi = (status_phys)})

struct vtd_inv_desc {
    uint64_t lo;
    uint64_t hi;
} __packed;

_Static_assert(sizeof(struct vtd_inv_desc) == 16, "inv desc must be 16B");

extern const struct iommu_ops vtd_iommu_ops;

struct iommu_unit *vtd_unit_create(uint64_t base_phys, uint16_t segment,
                                   uint8_t size_field);

/* ND encoding to actual domain count */
static inline uint32_t vtd_cap_domain_count(uint64_t cap) {
    static const uint32_t nd_table[] = {16,   64,    256,   1024,
                                        4096, 16384, 65536, 0};
    return nd_table[CAP_ND(cap)];
}
