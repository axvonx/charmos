/* @title: Timer Types */
#pragma once
#include <structures/hlist.h>
#include <types/types.h>

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

    void *data;
};
