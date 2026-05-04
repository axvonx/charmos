#include <drivers/mmio.h>
#include <mem/address_range.h>
#include <mem/page.h>
#include <mem/vaddr_alloc.h>
#include <mem/vmm.h>

static struct vas_space *mmio_vas_space = NULL;
ADDRESS_RANGE_DECLARE(mmio, .name = "mmio", .align = PAGE_1GB,
                      .flags = ADDRESS_RANGE_DYNAMIC, .size = MMIO_RANGE_SIZE);

void mmio_init() {
    if (!(mmio_vas_space = vas_space_from_address_range(&ADDRESS_RANGE(mmio))))
        panic("OOM\n");
}

void *mmio_map(paddr_t phys, size_t size) {
    kassert(mmio_vas_space);
    return vas_map(mmio_vas_space, phys, size, PAGE_WRITE | PAGE_UNCACHABLE,
                   VMM_FLAG_NONE);
}

void mmio_unmap(void *vaddr, size_t size) {
    kassert(mmio_vas_space);
    vas_unmap(mmio_vas_space, vaddr, size);
}
