/* @title: Per-CPU structure */
#pragma once
#include <compiler.h>
#include <console/panic.h>
#include <math/bit.h>
#include <sch/irql.h>
#include <smp/topology.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <thread/dpc.h>
#include <types/types.h>

#define CPU_FEAT_SSE2 BIT(0)
#define CPU_FEAT_AVX BIT(1)
#define CPU_FEAT_AVX2 BIT(2)
#define CPU_FEAT_AVX512F BIT(3)

enum cpu_class {
    CPU_CLASS_UNKNOWN,
    CPU_CLASS_PERFORMANCE,
    CPU_CLASS_EFFICIENCY,
};

enum {
    UARCH_UNKNOWN,
    UARCH_GOLDEN_COVE,
    UARCH_GRACEMONT,
    UARCH_SKYLAKE,
};

struct cpu_capability {
    enum cpu_class class;

    uint32_t uarch_id; /* e.g. golden cove, gracemont */

    uint32_t issue_width;
    uint32_t retire_width;

    cpu_perf_t perf_score; /* Relative to everyone else on a 0-255
                            * scale, how performant are we? The
                            * higher this number is, the more "performant"
                            * this CPU currently is, and the more likely
                            * the scheduler will decide to migrate a
                            * thread that needs such perf scores onto here. */

    uint32_t energy_score; /* lower is better */

    uint64_t feature_bits; /* ISA features, vector width, etc */
};

/* Let's put commonly accessed fields up here
 * to make the cache a bit happier */
struct core {
    struct core *self;
    cpu_id_t id;
    struct thread *current_thread;
    struct cpu_capability cap;

    size_t domain_cpu_id; /* what CPU in the domain? */

    /* array [domain_levels_enabled] -> domain reference */
    struct scheduler_domain *domains[TOPOLOGY_LEVEL_MAX];

    /* index within each domain's groups */
    int32_t group_index[TOPOLOGY_LEVEL_MAX];

    atomic_bool executing_dpcs;
    atomic_bool idle;

    /* Execution context in one word, using SMP_CTX_* below */
    uint32_t ctx;

    enum irql current_irql;

    atomic_bool needs_run_dpcs; /* Set before sending IRQ_NOP, which is then
                                 * checked in the isr_common_entry */

    atomic_bool needs_resched;
    atomic_bool in_resched; /* in scheduler_yield() */

    struct domain *domain;
    struct domain_arena *domain_arena;
    size_t rr_current_domain;

    struct tss *tss;

    freq_khz_t lapic_khz;

    struct topology_node *topo_node;
    struct topology_cache_info llc;

    numa_node_t numa_node;
    uint32_t package_id;
    uint32_t smt_mask;
    uint32_t smt_id;
    uint32_t core_id;

    freq_hz_t tsc_hz;
    time_us_t last_us;
    uint64_t last_tsc; /* For time.c */

    _Atomic uint64_t pt_seen_epoch;
    bool reclaiming_page_tables;
};

void smp_caller_verify(enum topology_caller caller);
#define smp_read8(off)                                                         \
    ({                                                                         \
        uint8_t __v;                                                           \
        asm volatile("movb %%gs:%c1, %b0" : "=q"(__v) : "i"(off) : "memory");  \
        __v;                                                                   \
    })

#define smp_read16(off)                                                        \
    ({                                                                         \
        uint16_t __v;                                                          \
        asm volatile("movw %%gs:%c1, %w0" : "=r"(__v) : "i"(off) : "memory");  \
        __v;                                                                   \
    })

#define smp_read32(off)                                                        \
    ({                                                                         \
        uint32_t __v;                                                          \
        asm volatile("movl %%gs:%c1, %k0" : "=r"(__v) : "i"(off) : "memory");  \
        __v;                                                                   \
    })

#define smp_read64(off)                                                        \
    ({                                                                         \
        uint64_t __v;                                                          \
        asm volatile("movq %%gs:%c1, %0" : "=r"(__v) : "i"(off) : "memory");   \
        __v;                                                                   \
    })

#define smp_member_size(member) sizeof(typeof(((struct core *) 0)->member))

#define smp_read(cond, member)                                                 \
    ({                                                                         \
        if (cond != TOPC_NONE)                                                 \
            smp_caller_verify(cond);                                           \
        static_assert(                                                         \
            smp_member_size(member) == 1 || smp_member_size(member) == 2 ||    \
                smp_member_size(member) == 4 || smp_member_size(member) == 8,  \
            "smp_core_read: unsupported member size");                         \
        uint64_t _raw;                                                         \
        switch (smp_member_size(member)) {                                     \
        case 1: _raw = smp_read8(offsetof(struct core, member)); break;        \
        case 2: _raw = smp_read16(offsetof(struct core, member)); break;       \
        case 4: _raw = smp_read32(offsetof(struct core, member)); break;       \
        case 8: _raw = smp_read64(offsetof(struct core, member)); break;       \
        default: __builtin_unreachable();                                      \
        }                                                                      \
        (typeof(ct_decay(((struct core *) 0)->member))) (uintptr_t) _raw;      \
    })

#define smp_write8(off, v)                                                     \
    ({                                                                         \
        uint8_t __v = (v);                                                     \
        asm volatile("movb %b0, %%gs:%c1" : : "q"(__v), "i"(off) : "memory");  \
    })

#define smp_write16(off, v)                                                    \
    ({                                                                         \
        uint16_t __v = (v);                                                    \
        asm volatile("movw %w0, %%gs:%c1" : : "r"(__v), "i"(off) : "memory");  \
    })

#define smp_write32(off, v)                                                    \
    ({                                                                         \
        uint32_t __v = (v);                                                    \
        asm volatile("movl %0, %%gs:%c1" : : "r"(__v), "i"(off) : "memory");   \
    })

#define smp_write64(off, v)                                                    \
    ({                                                                         \
        uint64_t __v = (v);                                                    \
        asm volatile("movq %0, %%gs:%c1" : : "r"(__v), "i"(off) : "memory");   \
    })

#define smp_write(cond, member, val)                                           \
    do {                                                                       \
        if (cond != TOPC_NONE)                                                 \
            smp_caller_verify(cond);                                           \
        static_assert(                                                         \
            smp_member_size(member) == 1 || smp_member_size(member) == 2 ||    \
                smp_member_size(member) == 4 || smp_member_size(member) == 8,  \
            "smp_core_write: unsupported member size");                        \
        typeof(((struct core *) 0)->member) _val = (val);                      \
        uint64_t _raw = (uint64_t) (uintptr_t) _val;                           \
        switch (smp_member_size(member)) {                                     \
        case 1:                                                                \
            smp_write8(offsetof(struct core, member), (uint8_t) _raw);         \
            break;                                                             \
        case 2:                                                                \
            smp_write16(offsetof(struct core, member), (uint16_t) _raw);       \
            break;                                                             \
        case 4:                                                                \
            smp_write32(offsetof(struct core, member), (uint32_t) _raw);       \
            break;                                                             \
        case 8: smp_write64(offsetof(struct core, member), _raw); break;       \
        default: __builtin_unreachable();                                      \
        }                                                                      \
    } while (0)

static inline cpu_id_t smp_id(enum topology_caller cond) {
    return smp_read(cond, id);
}

static inline cpu_id_t smp_id_raw(void) {
    return smp_read(TOPC_NONE, id);
}

static inline struct core *smp_core(enum topology_caller cond) {
    return smp_read(cond, self);
}

static inline struct core *smp_core_raw(void) {
    return smp_read(TOPC_NONE, self);
}

/* The idea with this (TODO: Consider an enum) is that if a migration happens
 * across reads of two different words, the whole result becomes invalid,
 * and squishing it all into one word guarantees that such behavior
 * is not possible. We also use counters here, as opposed to flags,
 * so reentrancy bugs can be identified from the get-go.
 *
 * NOTE: needs_resched is not tracked here because that's for other
 * CPUs to write, and this is purely local
 */
#define SMP_CTX_PREEMPT_SHIFT 0
#define SMP_CTX_PREEMPT_BITS 8
#define SMP_CTX_IRQ_SHIFT 8
#define SMP_CTX_IRQ_BITS 8
#define SMP_CTX_NMI_SHIFT 16
#define SMP_CTX_NMI_BITS 4

#define SMP_CTX_FIELD_MAX(bits) ((1u << (bits)) - 1u)

#define SMP_CTX_PREEMPT_MASK                                                   \
    (SMP_CTX_FIELD_MAX(SMP_CTX_PREEMPT_BITS) << SMP_CTX_PREEMPT_SHIFT)
#define SMP_CTX_IRQ_MASK                                                       \
    (SMP_CTX_FIELD_MAX(SMP_CTX_IRQ_BITS) << SMP_CTX_IRQ_SHIFT)
#define SMP_CTX_NMI_MASK                                                       \
    (SMP_CTX_FIELD_MAX(SMP_CTX_NMI_BITS) << SMP_CTX_NMI_SHIFT)

#define SMP_CTX_PREEMPT_ONE (1u << SMP_CTX_PREEMPT_SHIFT)
#define SMP_CTX_IRQ_ONE (1u << SMP_CTX_IRQ_SHIFT)
#define SMP_CTX_NMI_ONE (1u << SMP_CTX_NMI_SHIFT)

/* "not plain thread context" */
#define SMP_CTX_IN_INTERRUPT_MASK (SMP_CTX_IRQ_MASK | SMP_CTX_NMI_MASK)

struct core *smp_bsp(void);
static inline uint32_t smp_ctx(enum topology_caller c) {
    return smp_read(c, ctx);
}

static inline uint32_t smp_ctx_preempt_count(uint32_t smp_ctx) {
    return (smp_ctx & SMP_CTX_PREEMPT_MASK) >> SMP_CTX_PREEMPT_SHIFT;
}

static inline uint32_t smp_ctx_irq_count(uint32_t smp_ctx) {
    return (smp_ctx & SMP_CTX_IRQ_MASK) >> SMP_CTX_IRQ_SHIFT;
}

static inline uint32_t smp_ctx_nmi_count(uint32_t smp_ctx) {
    return (smp_ctx & SMP_CTX_NMI_MASK) >> SMP_CTX_NMI_SHIFT;
}

/* Add a SMP_CTX_*_ONE to its field */
static inline uint32_t smp_ctx_add(enum topology_caller c, uint32_t one,
                                   uint32_t mask) {
    struct core *cpu = smp_core(c);
    if (cc_unlikely((cpu->ctx & mask) == mask))
        panic("smp_ctx field overflow, mask %#x, smp_ctx %#x", mask, cpu->ctx);

    cpu->ctx += one;
    return cpu->ctx;
}

static inline uint32_t smp_ctx_sub(enum topology_caller c, uint32_t one,
                                   uint32_t mask) {
    struct core *cpu = smp_core(c);
    if (cc_unlikely((cpu->ctx & mask) == 0))
        panic("smp_ctx field underflow, mask %#x, smp_ctx %#x", mask, cpu->ctx);

    cpu->ctx -= one;
    return cpu->ctx;
}

#define for_each_cpu_struct(__iter)                                            \
    for (size_t __id = 0;                                                      \
         ((__iter = global.cores[__id]), __id < global.core_count); __id++)

#define for_each_cpu_id(__id) for (__id = 0; __id < global.core_count; __id++)
