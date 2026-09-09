#include "mem/tests/test_internal.h"

TEST_DECLARE_SMOKE(mem, demand_alloc_smoke) {
    void *ptr = page_alloc_demand(8, ALLOC_FLAGS_ZERO);
    memset(ptr, 67, PAGE_SIZE);
    test_info("successfully demand allocated and memsetted memory");
    return TEST_SUCCESS;
}
