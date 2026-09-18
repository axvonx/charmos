/* @title: Bitmap allocator */
#pragma once
#include <math/bit.h>
#include <mem/alloc.h>
#include <mem/page.h>
#include <mem/vmm.h>
#include <stdbool.h>
#include <stdint.h>
#include <types/types.h>

#define BOOT_BITMAP_SIZE (MB(128) / PAGE_SIZE / 8)

extern uint8_t boot_bitmap[BOOT_BITMAP_SIZE];
extern uint8_t *bitmap;
extern uint64_t bitmap_size;

paddr_t bitmap_alloc_pages(uint64_t count, enum alloc_flags f);
void bitmap_free_pages(paddr_t addr, uint64_t count);

static inline cc_no_sanitize_address void set_bit(uint64_t index) {
    uint64_t byte = index / 8;
    uint8_t mask = 0;
    mask = BIT_SET(mask, index % 8);
    if (byte >= BOOT_BITMAP_SIZE)
        return;

    __atomic_fetch_or(&bitmap[byte], mask, __ATOMIC_SEQ_CST);
}

static inline cc_no_sanitize_address void clear_bit(uint64_t index) {
    uint64_t byte = index / 8;
    uint8_t mask = UINT8_MAX;
    mask = BIT_CLEAR(mask, index % 8);
    if (byte >= BOOT_BITMAP_SIZE)
        return;

    __atomic_fetch_and(&bitmap[byte], mask, __ATOMIC_SEQ_CST);
}

static inline cc_no_sanitize_address bool test_bit(uint64_t index) {
    uint64_t byte = index / 8;
    uint8_t value;
    if (byte >= BOOT_BITMAP_SIZE)
        return false;

    __atomic_load(&bitmap[byte], &value, __ATOMIC_SEQ_CST);
    return BIT_TEST(value, index % 8);
}
