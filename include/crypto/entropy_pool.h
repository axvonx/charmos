/* @title: Entropy Pool */
#pragma once
#include <math/units.h>
#include <stdint.h>
#include <sync/spinlock.h>

#define ENTROPY_POOL_SIZE 64 // 512 bits
#define ENTROPY_MAX_BITS to_bits(ENTROPY_POOL_SIZE)

struct entropy_pool {
    uint8_t buffer[ENTROPY_POOL_SIZE]; // Raw pool data
    uint64_t write_pos;                // Circular buffer write position
    uint64_t entropy_bits;             // Estimated entropy in bits
    struct spinlock lock;              // To protect concurrent access
};

void entropy_pool_init(struct entropy_pool *pool);
void entropy_pool_add(struct entropy_pool *pool, const uint8_t *data,
                      uint64_t len, uint64_t entropy_bits);
void entropy_pool_decrease(struct entropy_pool *pool, uint64_t bits);
void entropy_pool_seed(struct entropy_pool *pool);
void entropy_pool_reseed(struct entropy_pool *pool);
