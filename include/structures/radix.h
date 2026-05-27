/* @title: Radix Tree */
#pragma once
#include <errno.h>
#include <stdint.h>

#define RADIX_BITS 6
#define RADIX_SIZE (1 << RADIX_BITS)
#define RADIX_MASK (RADIX_SIZE - 1)

#define NUM_INSERTS 128
#define NUM_LOOKUPS 32

typedef uint64_t (*radix_key_fn)(const void *item);

struct radix_node {
    struct radix_node *parent;
    void *slots[RADIX_SIZE];
    uint64_t present_mask;
};

struct radix_tree {
    struct radix_node *root;
    uint32_t height;
    radix_key_fn key_fn;
};
