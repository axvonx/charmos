#include "mem/tests/test_internal.h"
#include <mem/address_range.h>

ADDRESS_RANGE_DECLARE(vas_test_map, .flags = ADDRESS_RANGE_DYNAMIC,
                      .size = PAGE_2MB, .align = PAGE_SIZE);

TEST_DECLARE_SMOKE(vas, reserve_query_free) {
    const vaddr_t base = 0x700000000000ULL;
    struct vas *vas = vas_create(base, base + PAGE_1GB);
    TEST_ASSERT_NONNULL(vas);

    size_t sizes[] = {PAGE_SIZE, 64 * 1024, PAGE_2MB};
    vaddr_t addresses[TEST_ARRAY_LEN(sizes)];
    for (size_t i = 0; i < TEST_ARRAY_LEN(sizes); i++) {
        addresses[i] = vas_alloc(vas, sizes[i], PAGE_SIZE);
        TEST_ASSERT_NE(addresses[i], 0);
        TEST_ASSERT(vas_vaddr_is_allocated(vas, addresses[i]));
        TEST_ASSERT(vas_vaddr_is_allocated(vas, addresses[i] + sizes[i] - 1));
    }

    for (size_t i = 0; i < TEST_ARRAY_LEN(sizes); i++) {
        vas_free(vas, addresses[i], sizes[i]);
        TEST_ASSERT_FALSE(vas_vaddr_is_allocated(vas, addresses[i]));
    }

    TEST_ASSERT(vas_destroy(vas));
    return TEST_SUCCESS;
}

TEST_DECLARE_SMOKE(vas, map_unaligned_physical_address) {
    struct vas *vas = vas_from(&ADDRESS_RANGE(vas_test_map));
    TEST_ASSERT_NONNULL(vas);
    paddr_t phys = pmm_alloc_pages(2);
    TEST_ASSERT_NE(phys, 0);
    void *mapped =
        vas_map(vas, phys + 37, PAGE_SIZE, PAGE_WRITE, VMM_FLAG_NONE);

    TEST_ASSERT_NONNULL(mapped);
    vaddr_t base = (vaddr_t) mapped - 37;

    TEST_ASSERT_EQ(base & (PAGE_SIZE - 1), 0);
    TEST_ASSERT_EQ(vmm_get_phys((vaddr_t) mapped, VMM_FLAG_NONE), phys + 37);
    TEST_ASSERT(vas_vaddr_is_allocated(vas, base + 2 * PAGE_SIZE - 1));

    ((uint8_t *) mapped)[PAGE_SIZE - 1] = 0x57;
    TEST_ASSERT_EQ(((uint8_t *) hhdm_paddr_to_ptr(phys))[PAGE_SIZE + 36], 0x57);
    vaddr_t next = vas_alloc(vas, PAGE_SIZE, PAGE_SIZE);

    TEST_ASSERT_EQ(next, base + 2 * PAGE_SIZE);
    vas_unmap(vas, mapped, PAGE_SIZE);

    TEST_ASSERT_FALSE(vas_vaddr_is_allocated(vas, base));
    TEST_ASSERT_EQ(vmm_get_phys(base, VMM_FLAG_NONE), PADDR_MAX);
    TEST_ASSERT_EQ(vmm_get_phys(base + PAGE_SIZE, VMM_FLAG_NONE), PADDR_MAX);
    vas_free(vas, next, PAGE_SIZE);
    pmm_free_pages(phys, 2);
    TEST_ASSERT(vas_destroy(vas));

    return TEST_SUCCESS;
}
