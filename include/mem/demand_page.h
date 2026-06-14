/* @title: Demand Page */
#pragma once
#include <types/types.h>
enum demand_page_flags : page_flags_t {
    /* We purposely leave zero unused to make
     * sure that this was intentionally a demand
     * page and not something odd happening */
    DEMAND_PAGE_FLAG_NONE = 1,
    DEMAND_PAGE_FLAG_ZERO_MEMORY = 2,
    DEMAND_PAGE_FLAG_WRITABLE = 1 << 3,
    DEMAND_PAGE_FLAG_XD = 1 << 4,
};
