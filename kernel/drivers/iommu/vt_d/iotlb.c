#include <drivers/mmio.h>

#include "internal.h"

static uint64_t *vtd_iotlb_reg(struct vtd_unit *u) {
    return (uint64_t *) ((uintptr_t) u->regs + IOTLB_REG_OFFSET(u->ecap));
}

static uint64_t *vtd_iva_reg(struct vtd_unit *u) {
    return (uint64_t *) ((uintptr_t) u->regs + IVA_REG_OFFSET(u->ecap));
}

static void vtd_iotlb_reg_flush(struct vtd_unit *u, uint64_t val) {
    mmio_write_64(vtd_iotlb_reg(u), val | IOTLB_REG_INVALIDATE);

    while (mmio_read_64(vtd_iotlb_reg(u)) & IOTLB_REG_INVALIDATE)
        cpu_relax();
}

void vtd_iotlb_flush_global(struct vtd_unit *u) {
    if (ECAP_QUEUED_INVALIDATION(u->ecap)) {
        vtd_iq_submit(u, IOTLB_INVAL_DESC_GLOBAL);
        vtd_iq_flush(u);
        return;
    }

    vtd_iotlb_reg_flush(u, IOTLB_REG_GLOBAL | IOTLB_REG_DRAIN_READS |
                               IOTLB_REG_DRAIN_WRITES);
}

void vtd_iotlb_flush_domain(struct vtd_unit *u, uint16_t domain_id) {
    if (ECAP_QUEUED_INVALIDATION(u->ecap)) {
        vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(domain_id));
        vtd_iq_flush(u);
        return;
    }

    vtd_iotlb_reg_flush(
        u, IOTLB_REG_DOMAIN | IOTLB_REG_DRAIN_READS | IOTLB_REG_DRAIN_WRITES |
               ((uint64_t) domain_id << IOTLB_REG_DOMAIN_ID_SHIFT));
}

void vtd_iotlb_flush_page(struct vtd_unit *u, uint16_t domain_id, iova_t iova,
                          uint8_t am) {
    if (ECAP_QUEUED_INVALIDATION(u->ecap)) {
        vtd_iq_submit(u, IOTLB_INVAL_DESC_PAGE(domain_id, iova, am));
        vtd_iq_flush(u);
        return;
    }

    mmio_write_64(vtd_iva_reg(u),
                  (iova & IVA_REG_ADDR_MASK) | IVA_REG_HINT | am);

    vtd_iotlb_reg_flush(
        u, IOTLB_REG_PAGE | IOTLB_REG_DRAIN_READS | IOTLB_REG_DRAIN_WRITES |
               ((uint64_t) domain_id << IOTLB_REG_DOMAIN_ID_SHIFT));
}

void vtd_iotlb_flush_range(struct vtd_unit *u, uint16_t domain_id, iova_t iova,
                           size_t size) {
    if (CAP_PAGE_SELECTIVE_INVALIDATION(u->cap) && size <= 32 * PAGE_SIZE) {
        for (size_t off = 0; off < size; off += PAGE_SIZE)
            vtd_iotlb_flush_page(u, domain_id, iova + off, 0);
        return;
    }

    vtd_iotlb_flush_domain(u, domain_id);
}

void vtd_iotlb_flush_range_batched(struct vtd_unit *u, uint16_t domain_id,
                                   iova_t iova, size_t size) {
    if (!ECAP_QUEUED_INVALIDATION(u->ecap)) {
        vtd_iotlb_flush_range(u, domain_id, iova, size);
        return;
    }

    if (CAP_PAGE_SELECTIVE_INVALIDATION(u->cap) && size <= 32 * PAGE_SIZE) {
        for (size_t off = 0; off < size; off += PAGE_SIZE)
            vtd_iq_submit(u, IOTLB_INVAL_DESC_PAGE(domain_id, iova + off, 0));
    } else {
        vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(domain_id));
    }

    vtd_iq_flush(u);
}
