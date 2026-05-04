/* @title: Address Ranges */
#pragma once
#include <compiler.h>
#include <linker/symbols.h>
#include <stddef.h>
#include <stdint.h>
#include <structures/rbt.h>
#include <types/types.h>

/* TODO: To different architectures, change this (also, for 32 bit) */
#define ADDRESS_RANGE_KERNEL_START 0xFFFF800000000000
#define ADDRESS_RANGE_KERNEL_END 0xFFFFFFFFFFFFFFFF

enum address_range_flags {
    ADDRESS_RANGE_DYNAMIC = 1 << 0,
    ADDRESS_RANGE_STATIC = 0,
};

struct address_range {
    const char *name;
    vaddr_t base;
    size_t size;
    size_t align;
    enum address_range_flags flags;
    struct rbt_node rbt_node_internal;
} __linker_aligned;

#define ADDRESS_RANGE_DECLARE(sym, ...)                                        \
    static struct address_range __address_range_##sym                          \
        __attribute__((section(".kernel_address_ranges"), used)) = {           \
            __VA_ARGS__, .rbt_node_internal = RBT_NODE_INIT}

#define ADDRESS_RANGE(sym) (__address_range_##sym)

LINKER_SECTION_DEFINE(address_ranges, struct address_range);

void address_ranges_init();
void address_ranges_print();
struct address_range *address_range_for_addr(vaddr_t vaddr);
