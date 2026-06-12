#include <math/kb_mb_gb_tb.h>
#include <mem/address_range.h>
#include <mem/hhdm.h>
#include <mem/page.h>
#include <mem/page_alloc.h>
#include <mem/pmm.h>
#include <mem/vas.h>

static struct vas *page_alloc_vas = NULL;
ADDRESS_RANGE_DECLARE(page_alloc, .align = PAGE_SIZE,
                      .flags = ADDRESS_RANGE_DYNAMIC, .size = TB(8));

void page_alloc_init() {
    page_alloc_vas = vas_bootstrap_from(&ADDRESS_RANGE(page_alloc));
}

void *page_alloc_internal(size_t n_pages, enum alloc_flags flags) {
    if (n_pages == 1 || flags & ALLOC_FLAG_CONTIGUOUS) {
        paddr_t phys = pmm_alloc_pages(n_pages);
        if (!phys)
            return NULL;

        return hhdm_paddr_to_ptr(phys);
    } else {
        vaddr_t virt =
            vas_alloc(page_alloc_vas, n_pages * PAGE_SIZE, PAGE_SIZE);
        uintptr_t phys_pages[n_pages];
        uint64_t allocated = 0;

        page_flags_t page_flags = PAGE_PRESENT | PAGE_WRITE | PAGE_XD;
        if (flags & ALLOC_FLAG_PAGEABLE)
            page_flags |= PAGE_PAGEABLE;

        for (uint64_t i = 0; i < n_pages; i++) {
            uintptr_t phys = pmm_alloc_page(flags);
            if (!phys) {
                for (uint64_t j = 0; j < allocated; j++)
                    pmm_free_page(phys_pages[j]);
                return NULL;
            }

            enum errno e = vmm_map_page(virt + i * PAGE_SIZE, phys, page_flags,
                                        VMM_FLAG_NONE);
            if (e < 0) {
                pmm_free_page(phys);
                for (uint64_t j = 0; j < allocated; j++)
                    pmm_free_page(phys_pages[j]);

                return NULL;
            }

            phys_pages[allocated++] = phys;
        }

        return (void *) virt;
    }
}

void page_free_internal(void *ptr, size_t n_pages, enum alloc_behavior b) {
    if (hhdm_ptr_in_range(ptr)) {
        pmm_free_pages(hhdm_ptr_to_paddr(ptr), n_pages);
    } else {
        vaddr_t virt = (vaddr_t) ptr;
        for (uint32_t i = 0; i < n_pages; i++) {
            uintptr_t vaddr = virt + i * PAGE_SIZE;
            paddr_t phys = (paddr_t) vmm_get_phys(vaddr, VMM_FLAG_NONE);
            vmm_unmap_page(vaddr, VMM_FLAG_NONE);
            pmm_free_page(phys);
        }

        vas_free(page_alloc_vas, virt, n_pages * PAGE_SIZE);
    }
}
