/* @title: Type aliases */
#pragma once
#include <compiler.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Time units */
ct_strong_int(time_ns, TIME_NS, uint64_t, UINT64_MAX);
ct_strong_int(time_us, TIME_US, uint64_t, UINT64_MAX);
ct_strong_int(time_ms, TIME_MS, uint64_t, UINT64_MAX);
ct_strong_int(time_s, TIME_S, uint64_t, UINT64_MAX);
typedef uint64_t timestamp_t;

/* Frequency */
ct_strong_int(freq_hz, FREQ_HZ, uint64_t, UINT64_MAX);
ct_strong_int(freq_khz, FREQ_KHZ, uint64_t, UINT64_MAX);
ct_strong_int(freq_mhz, FREQ_MHZ, uint64_t, UINT64_MAX);
ct_strong_int(freq_ghz, FREQ_GHZ, uint64_t, UINT64_MAX);

/* Aliases over UNIX identifiers */
ct_strong_int(inode, INODE, uint32_t, UINT32_MAX);
typedef uint16_t mode_t;
typedef uint32_t gid_t;
typedef uint32_t uid_t;

/* For stack depot: someday will become uint32_t */
typedef struct stack_depot_record *stack_handle_t;

/* Refcount */
typedef _Atomic uint32_t refcount_t;
typedef _Atomic uint32_t mapcount_t;

/* Addresses and memory */
ct_strong_int(paddr, PADDR, uintptr_t, UINTPTR_MAX);
ct_strong_int(vaddr, VADDR, uintptr_t, UINTPTR_MAX);
ct_strong_int(pfn, PFN, uintptr_t, UINTPTR_MAX);
ct_strong_int(pgoff, PGOFF, uintptr_t, UINTPTR_MAX);
ct_strong_int(iova, IOVA, uintptr_t, UINTPTR_MAX);
ct_strong_int(pte, PTE, uint64_t, UINT64_MAX);
typedef uint64_t page_flags_t;

/* Processor topology and threading */
typedef uint8_t irq_t;
typedef int8_t cpu_perf_t;
ct_strong_int(cpu_id, CPU_ID, size_t, SIZE_MAX);
ct_strong_int(domain_id, DOMAIN_ID, size_t, SIZE_MAX);
ct_strong_int(numa_node, NUMA_NODE, size_t, SIZE_MAX);
ct_strong_int(thread_id, THREAD_ID, size_t, SIZE_MAX);
typedef int32_t nice_t;
typedef uint32_t thread_prio_t;

/* Other types */
typedef int64_t fx32_32_t;
typedef ptrdiff_t ssize_t;

/* Larger types */
typedef __int128_t int128_t;
typedef __uint128_t uint128_t;

#define CPU_PERF_MAX INT8_MAX
#define CPU_PERF_MIN INT8_MIN

#define MODE_MAX UINT16_MAX
#define MODE_MIN 0

#define GID_MAX UINT32_MAX
#define GID_MIN 0

#define UID_MAX UINT32_MAX
#define UID_MIN 0

#define STACK_HANDLE_MAX UINT32_MAX
#define STACK_HANDLE_MIN 0

#define REFCOUNT_MAX UINT32_MAX
#define REFCOUNT_MIN 0

#define CPU_ID_NONE CPU_ID_MAX

#define DOMAIN_ID_NONE DOMAIN_ID_MAX

#define THREAD_ID_NONE THREAD_ID_MAX

#define PAGE_FLAGS_MAX UINT64_MAX
#define PAGE_FLAGS_MIN 0

#define THREAD_PRIO_MAX UINT32_MAX
#define THREAD_PRIO_MIN 0

#define FX32_32_MAX INT64_MAX
#define FX32_32_MIN INT64_MIN

#define NICE_MAX 19
#define NICE_MIN -20

#define SSIZE_MAX ((ssize_t) (SIZE_MAX >> 1))
#define SSIZE_MIN ((ssize_t) (-SSIZE_MAX - 1))

/* Shift has to come after the complement, because complementing the
 * shifted zero yields all ones, turning it into `-1` */
#define INT128_MAX ((int128_t) (~((uint128_t) 0ULL) >> 1))
#define INT128_MIN ((int128_t) (-INT128_MAX - 1))

#define UINT128_MAX ((uint128_t) (~((uint128_t) 0ULL)))
#define UINT128_MIN ((uint128_t) 0ULL)
