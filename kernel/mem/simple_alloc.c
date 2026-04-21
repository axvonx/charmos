#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vaddr_alloc.h>
#include <mem/vmm.h>
#include <string.h>
/* simple alloc used for bootstrapping systems */
void *simple_alloc(struct vas_space *space, size_t size) {
    size_t pages = PAGES_NEEDED_FOR(size);
    vaddr_t virt_base = vas_alloc(space, size, PAGE_SIZE);

    for (size_t i = 0; i < pages; i++) {
        vaddr_t virt = virt_base + (i * PAGE_SIZE);
        paddr_t phys = pmm_alloc_page();
        kassert(phys);
        vmm_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE, VMM_FLAG_NONE);
    }

    memset((void *) virt_base, 0, size);
    return (void *) virt_base;
}

void simple_free(struct vas_space *space, void *ptr, size_t size) {
    vaddr_t real_virt = (vaddr_t) ptr;
    size_t pages = PAGES_NEEDED_FOR(size);
    for (size_t i = 0; i < pages; i++) {
        vaddr_t virt = (vaddr_t) real_virt + i * PAGE_SIZE;
        paddr_t phys = vmm_get_phys(virt, VMM_FLAG_NONE);
        kassert(phys != (paddr_t) -1);
        vmm_unmap_page(virt, VMM_FLAG_NONE);
        pmm_free_page(phys);
    }
    vas_free(space, real_virt, size);
}
