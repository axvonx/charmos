#include <asm.h>
#include <console/printf.h>
#include <drivers/iommu/vt_d.h>
#include <drivers/mmio.h>
#include <log.h>
#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <string.h>

#include "internal.h"

static uint64_t *vtd_sl_pte_get_or_alloc(uint64_t *table, size_t idx) {
    uint64_t pte = table[idx];

    if (pte & SL_PTE_READ) {
        paddr_t next_phys = SL_PTE_ADDR(pte);
        return mmio_map(next_phys, PAGE_SIZE);
    }

    paddr_t phys = pmm_alloc_page();
    if (!phys)
        return NULL;

    void *virt = mmio_map(phys, PAGE_SIZE);
    memset(virt, 0, PAGE_SIZE);

    table[idx] = SL_TABLE_ENTRY(phys);
    return virt;
}

static enum iommu_error vtd_sl_map_page(uint64_t *sl_pgd, uint64_t iova,
                                        paddr_t pa, uint32_t perm) {
    uint64_t *pdp =
        vtd_sl_pte_get_or_alloc(sl_pgd, SL_PML4_INDEX(iova));
    if (!pdp)
        return IOMMU_ERR_NO_MEM;

    uint64_t *pd = vtd_sl_pte_get_or_alloc(pdp, SL_PDPT_INDEX(iova));
    if (!pd)
        return IOMMU_ERR_NO_MEM;

    uint64_t *pt = vtd_sl_pte_get_or_alloc(pd, SL_PD_INDEX(iova));
    if (!pt)
        return IOMMU_ERR_NO_MEM;

    pt[SL_PT_INDEX(iova)] = SL_PAGE_ENTRY(pa, perm);
    return IOMMU_ERR_OK;
}

static bool vtd_sl_unmap_page(uint64_t *sl_pgd, uint64_t iova) {
    uint64_t pte;

    pte = sl_pgd[SL_PML4_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return false;
    uint64_t *pdp = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    pte = pdp[SL_PDPT_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return false;
    uint64_t *pd = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    pte = pd[SL_PD_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return false;
    uint64_t *pt = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    if (!(pt[SL_PT_INDEX(iova)] & SL_PTE_READ))
        return false;
    pt[SL_PT_INDEX(iova)] = 0;
    return true;
}

static paddr_t vtd_sl_translate(uint64_t *sl_pgd, uint64_t iova) {
    uint64_t pte;

    pte = sl_pgd[SL_PML4_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return 0;
    uint64_t *pdp = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    pte = pdp[SL_PDPT_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return 0;
    uint64_t *pd = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    pte = pd[SL_PD_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return 0;
    uint64_t *pt = mmio_map(SL_PTE_ADDR(pte), PAGE_SIZE);

    pte = pt[SL_PT_INDEX(iova)];
    if (!(pte & SL_PTE_READ))
        return 0;

    return SL_PTE_ADDR(pte) | SL_PAGE_OFFSET(iova);
}

static enum iommu_error vtd_map(struct iommu_domain *domain, iova_t iova,
                                paddr_t pa, size_t size, uint32_t perm) {
    struct vtd_unit *u = domain->unit->private;
    struct vtd_domain *vd = domain->priv;

    /* size and addresses must be page-aligned                           */
    if (!IS_PAGE_ALIGNED(iova) || !IS_PAGE_ALIGNED(pa) ||
        !IS_PAGE_ALIGNED(size))
        return IOMMU_ERR_INVALID;

    for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
        enum iommu_error err =
            vtd_sl_map_page(vd->sl_pgd, iova + offset, pa + offset, perm);
        if (err != IOMMU_ERR_OK)
            return err;
    }

    /* one range invalidation covering the whole mapping                 */
    vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(vd->domain_id));
    vtd_iq_flush(u);

    return IOMMU_ERR_OK;
}

static void vtd_unmap(struct iommu_domain *domain, iova_t iova, size_t size) {
    struct vtd_unit *u = domain->unit->private;
    struct vtd_domain *vd = domain->priv;

    for (size_t offset = 0; offset < size; offset += PAGE_SIZE)
        vtd_sl_unmap_page(vd->sl_pgd, iova + offset);

    vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(vd->domain_id));
    vtd_iq_flush(u);
}

static void vtd_flush_iotlb_domain(struct iommu_domain *domain) {
    struct vtd_unit *u = domain->unit->private;
    struct vtd_domain *vd = domain->priv;

    vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(vd->domain_id));
    vtd_iq_flush(u);
}

static void vtd_flush_iotlb_range(struct iommu_domain *domain, iova_t iova,
                                  size_t size) {
    struct vtd_unit *u = domain->unit->private;
    struct vtd_domain *vd = domain->priv;

    if (CAP_PAGE_SELECTIVE_INVALIDATION(u->cap) && size <= 32 * PAGE_SIZE) {
        for (size_t off = 0; off < size; off += PAGE_SIZE)
            vtd_iq_submit(
                u, IOTLB_INVAL_DESC_PAGE(vd->domain_id, iova + off, 0));
    } else {
        vtd_iq_submit(u, IOTLB_INVAL_DESC_DOMAIN(vd->domain_id));
    }

    vtd_iq_flush(u);
}
