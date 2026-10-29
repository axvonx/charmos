/* @title: Spinlock Types */
#pragma once
#include <atomic.h>
#include <sync/lock_chk.h>
#include <sync/lock_chk_types.h>
#include <tsa.h>

/* For structures that embed locks */
struct TSA_CAPABILITY("spinlock") raw_spinlock {
    atomic_uint8_t state;
};

#define RAW_SPINLOCK_INIT ((struct raw_spinlock) {.state = ATOMIC_VAR_INIT(0)})

struct TSA_CAPABILITY("spinlock") spinlock {
    struct raw_spinlock raw;

#ifdef DEBUG_LOCK_CHK
    struct lock_chk_lock chk;
    atomic(enum lock_op_flags) irq_usage;
#endif /* DEBUG_LOCK_CHK */
};

TSA_DEFINE_ASSUME(struct spinlock)
