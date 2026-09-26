/* @title: Compiler Atomics, Volatile Access & Fences */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ca_read_once(x) (*(const volatile typeof(x) *) &(x))

#define ca_write_once(x, val)                                                  \
    do {                                                                       \
        *(volatile typeof(x) *) &(x) = (val);                                  \
    } while (0)

/* Compiler optimization barrier */
#define ca_barrier() asm volatile("" ::: "memory")

#define ca_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define ca_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define ca_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)

#if defined(__x86_64__) || defined(__i386__)
#define ca_pause() asm volatile("pause" ::: "memory")
#elif defined(__aarch64__)
#define ca_pause() asm volatile("isb" ::: "memory")
#else
#define ca_pause() ca_barrier()
#endif
