/* @title: Type aliases */
#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int8_t cpu_perf_t;
typedef uint64_t time_t;
typedef uint32_t inode_t;
typedef uint16_t mode_t;
typedef uint32_t gid_t;
typedef uint32_t uid_t;
typedef _Atomic uint32_t refcount_t;
typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;
typedef uintptr_t iova_t;
typedef size_t cpu_id_t;
typedef size_t numa_node_t;
typedef uint64_t pte_t;
typedef uint64_t page_flags_t;
typedef uint32_t thread_prio_t;
typedef int64_t fx32_32_t;
typedef int32_t nice_t;

#define CPU_PERF_MAX INT8_MAX
#define CPU_PERF_MIN INT8_MIN

#define TIME_MAX UINT64_MAX
#define TIME_MIN 0

#define INODE_MAX UINT32_MAX
#define INODE_MIN 0

#define MODE_MAX UINT16_MAX
#define MODE_MIN 0

#define GID_MAX UINT32_MAX
#define GID_MIN 0

#define UID_MAX UINT32_MAX
#define UID_MIN 0

#define REFCOUNT_MAX UINT32_MAX
#define REFCOUNT_MIN 0

#define PADDR_MAX UINTPTR_MAX
#define PADDR_MIN 0

#define VADDR_MAX UINTPTR_MAX
#define VADDR_MIN 0

#define IOVA_MAX UINTPTR_MAX
#define IOVA_MIN 0

#define CPU_ID_MAX SIZE_MAX
#define CPU_ID_MIN 0

#define NUMA_NODE_MAX SIZE_MAX
#define NUMA_NODE_MIN 0

#define PTE_MAX UINT64_MAX
#define PTE_MIN 0

#define PAGE_FLAGS_MAX UINT64_MAX
#define PAGE_FLAGS_MIN 0

#define THREAD_PRIO_MAX UINT32_MAX
#define THREAD_PRIO_MIN 0

#define FX32_32_MAX INT64_MAX
#define FX32_32_MIN INT64_MIN

#define NICE_MAX INT32_MAX
#define NICE_MIN INT32_MIN
