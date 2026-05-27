/* @title: (Counting) Bloom Filter */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

#define COUNTER_BITS 4
#define COUNTER_MAX ((1u << COUNTER_BITS) - 1) /* 15 */
#define COUNTERS_PER_BYTE (8 / COUNTER_BITS)   /* 2  */

struct counting_bloom_filter {
    uint8_t *counters;
    size_t num_counters;
    size_t num_hashes;
    size_t live_elements;
};

enum bloom_remove_result {
    BLOOM_REMOVE_OK = 0,
    BLOOM_REMOVE_NOT_FOUND = -1,
    BLOOM_REMOVE_SATURATED = -2,
};

/*
 * cbf_remove -- remove an element (decrements k counters).
 *
 *   BLOOM_REMOVE_OK        -- counters decremented, element logically removed.
 *   BLOOM_REMOVE_NOT_FOUND -- a counter is already 0, element not present
 *   BLOOM_REMOVE_SATURATED -- a counter is at max, decrementing it would
 *                             corrupt counts for other elements sharing the
 *                             slot, no counters are modified
 *
 * NOTE: only remove elements you actually inserted. Removing something
 * that was never added can corrupt counters shared with other elements,
 * potentially causing false negatives for those elements
 */
enum bloom_remove_result cbf_remove(struct counting_bloom_filter *cbf,
                                    const char *element);
bool cbf_contains(const struct counting_bloom_filter *cbf, const char *element);

/* current theoretical false-positive rate */
fx32_32_t cbf_estimated_fpr(const struct counting_bloom_filter *cbf);
void cbf_add(struct counting_bloom_filter *cbf, const char *element);
void cbf_destroy(struct counting_bloom_filter *cbf);

/*
 * cbf_create -- allocate a counting bloom filter sized for `capacity`
 * simultaneous live elements at the desired `false_positive_rate`.
 *
 * Uses the standard optimal formulas:
 *   m = -n * ln(p) / ln(2)^2     (counter slots)
 *   k = (m/n) * ln(2)            (hash functions)
 */
struct counting_bloom_filter *cbf_create(size_t capacity,
                                         fx32_32_t false_positive_rate);
