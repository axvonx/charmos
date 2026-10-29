#include "mem/slab/tests/test_internal.h"
#include <mem/alloc.h>

TEST_DEFINE_SMOKE(slab, demand_growth) {
    size_t count = 500;
    void **ptrs = vnews(ptrs, count);
    TEST_ASSERT_NONNULL(ptrs);

    for (size_t i = 0; i < count; i++) {
        ptrs[i] = valloc(512);
        TEST_ASSERT_NONNULL(ptrs[i]);
    }

    for (size_t i = 0; i < count; i++) {
        vfree(ptrs[i]);
    }
    vfree(ptrs);

    return TEST_SUCCESS;
}
