#include "mem/tests/test_internal.h"

TEST_DEFINE_UNIT(mem, tlb_shootdown_single_cpu, .min_ram_mib = 8) {
    paddr_t p1 = pmm_alloc_page();
    paddr_t p2 = pmm_alloc_page();
    TEST_ASSERT(p1 && p2);

    void *va = vmm_map_bump(p1, PAGE_SIZE);
    TEST_ASSERT_NONNULL(va);

    *(volatile uint64_t *) va = 0x11111111;

    vmm_unmap(va, PAGE_SIZE);
    va = vmm_map_bump(p2, PAGE_SIZE);

    tlb_shootdown(.payload.addr = (uintptr_t) va);

    *(volatile uint64_t *) va = 0x22222222;
    TEST_ASSERT_EQ(*(volatile uint64_t *) va, 0x22222222);

    return TEST_SUCCESS;
}
