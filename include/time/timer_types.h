/* @title: Timer Types */
#pragma once
#include <structures/hlist.h>
#include <types/types.h>

/* timer_flags: 32 bit bitflags
 *
 *      ┌───────────────────────────────────────────────────────┐
 * Bits │ 31..28 27..24 23..20 19..16 15..12  11..8  7..4  3..0 │
 * Use  │  %%%%   %%%%   %%Pd   ID##   ####    ####  ####  #### │
 *      └───────────────────────────────────────────────────────┘
 *
 * I - IRQ timer - not executed in DPC
 * D - Deferrable (Only activates when CPU non-idle)
 * d - DPC queued
 * P - Pinned
 *
 * %%% - Bucket idx (if present, up to 1023)
 * ### - CPU target for this timer
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum timer_flags {
    TIMER_FLAG_NONE = 0,
    TIMER_FLAG_DEFERRABLE = 1 << 18,
    TIMER_FLAG_IRQ = 1 << 19,
    TIMER_FLAG_DPC_QUEUED = 1 << 20,
    TIMER_FLAG_PINNED = 1 << 21,
};

struct timer {
    struct hlist_node hlist_node;
    time_us_t expiration_us;
    void (*func)(struct timer *timer);
    enum timer_flags flags;
};

#define TIMER_FLAG_CPU_MASK 0x0003FFFF
#define TIMER_FLAG_CPU(c) ((c) & TIMER_FLAG_CPU_MASK)

#define TIMER_FLAG_BUCKET_MASK 0x3FFu
#define TIMER_FLAG_BUCKET_SHIFT 22
#define TIMER_FLAG_BUCKET(b)                                                   \
    (((b) & TIMER_FLAG_BUCKET_MASK) << TIMER_FLAG_BUCKET_SHIFT)

#define TIMER_DEFINE(n, fn, ...)                                               \
    struct timer n = (struct timer) {                                          \
        .func = fn                                                             \
    }
