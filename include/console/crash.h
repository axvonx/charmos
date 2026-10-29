/* @title: Crash Engine */
#pragma once
#include <asm.h>
#include <compiler/core.h>
#include <linker/symbols.h>
#include <sch/irql.h>
#include <types/types.h>

struct crash_facility;
struct spinlock;
struct qspinlock;
struct rwlock;
struct mutex;
struct thread;
struct irq_registers;
struct irq_context;

#define CRASH_REG_COUNT 20

enum qemu_exit_codes {
    QEMU_EXIT_OK = 0,
    QEMU_EXIT_FAIL = 1,
    QEMU_EXIT_PANIC = 2,
};

struct crash_regs {
    union {
        struct {
            uint64_t rip, rflags, cr2, cr3;
            uint64_t rax, rbx, rcx, rdx, rbp, rdi, rsi;
            uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
            uint64_t rsp;
        };

        uint64_t regs[CRASH_REG_COUNT];
    };
};

ct_assert_struct_size_eq(crash_regs, CRASH_REG_COUNT * 8);

/* NULL crashes are the generic, code-less crash */
struct crash_code {
    const char *name;
    const char *desc;
    const struct crash_facility *facility;
};

/* Codes that don't have a facility, like enforce(p, INVALID_ARGUMENT) */
#define CRASH_CODES_global                                                     \
    (INVALID_ARGUMENT, "Argument was NULL or out of range"),                   \
        (INVALID_STATE, "Object was in a state this operation forbids"),       \
        (INVALID_CONTEXT,                                                      \
         "Operation ran at a forbidden IRQL, IRQ or boot stage"),              \
        (OUT_OF_MEMORY, "Allocation that must not fail did fail"),             \
        (UNSUPPORTED, "Hardware or firmware lacks a required feature"),        \
        (CORRUPTION, "Data structure or allocator invariant was broken"),      \
        (CAPACITY_EXCEEDED, "A fixed-size limit was exceeded"),                \
        (UNREACHABLE, "Control reached a path that must not execute"),         \
        (DUPLICATE, "Object was registered, inserted, or defined twice"),      \
        (OVERFLOW, "Counter or arithmetic result exceeded its range"),         \
        (UNDERFLOW, "Counter was decremented below zero"),                     \
        (STALE_REFERENCE, "Reference to an object that is dying or dead"),     \
        (BAD_FREE, "Freed memory was already free or never allocated"),        \
        (UNINITIALIZED, "Object was used before initialization"),              \
        (INIT_FAILED, "A required initialization step failed"),                \
        (NOT_FOUND, "Lookup that must succeed found nothing")

enum crash_source {
    CRASH_SOURCE_PANIC = 0,     /* panic() call */
    CRASH_SOURCE_ENFORCE,       /* enforce() failure */
    CRASH_SOURCE_KASAN,         /* ASAN check failure */
    CRASH_SOURCE_UBSAN,         /* UBSAN failure */
    CRASH_SOURCE_NMI_WATCHDOG,  /* Watchdog / Liveness monitor hard stall */
    CRASH_SOURCE_CPU_EXCEPTION, /* Hardware CPU fault */
    CRASH_SOURCE_NIGHTMARE,     /* Nightmare test harness failure */
    CRASH_SOURCE_LOCK_CHK, /* Lock validator order / dependency violation */
};

enum crash_format_flags {
    CRASH_FMT_RAW_SERIAL = 1 << 0,   /* Minimal serial printf */
    CRASH_FMT_VISUAL_PANES = 1 << 1, /* Dual pane ANSI console report */
    CRASH_FMT_NDJSON = 1 << 2,       /* NDJSON stream */
    CRASH_FMT_DUMP_LOGS = 1 << 3,    /* circular log buffer dump */
    CRASH_FMT_PEER_CPUS = 1 << 4,    /* Quiesce and render peer CPU frames */

    /* Default formatting */
    CRASH_FMT_DEFAULT = CRASH_FMT_RAW_SERIAL | CRASH_FMT_VISUAL_PANES |
                        CRASH_FMT_NDJSON | CRASH_FMT_DUMP_LOGS |
                        CRASH_FMT_PEER_CPUS,

    /* Minimal formatting for early boot or nested crashes */
    CRASH_FMT_MINIMAL = CRASH_FMT_RAW_SERIAL | CRASH_FMT_NDJSON,
};

enum crash_hook_flags {
    CRASH_HOOK_DEFAULT = 0,
    CRASH_HOOK_FACILITY = 1 << 0, /* Facility granularity: match
                                   * any code whose ->facility is the
                                   * code's facility
                                   */

    CRASH_HOOK_NO_UNWIND = 1 << 1, /* By default, the crash handler will make
                                    * a best-effort attempt to unwind: it'll
                                    * try to safely drop locks, exit RCU
                                    * read-side critical sections, although
                                    * memory may still leak (this is dependent
                                    * on the facility implementation).
                                    *
                                    * NO_UNWIND allows this behavior to be
                                    * skipped. This is because in certain cases,
                                    * the unwinding cannot happen (e.g. lock
                                    * checking is disabled), and also because
                                    * the stale state not unwinding
                                    * leaves can be used as a postmortem
                                    * for state verification.
                                    */

};

enum crash_unwind_type {
    CRASH_UNWIND_NONE, /* should not be reachable */
    CRASH_UNWIND_RCU,
    CRASH_UNWIND_MUTEX,
    CRASH_UNWIND_RWLOCK,
    CRASH_UNWIND_SPINLOCK,
    CRASH_UNWIND_QSPINLOCK,

    CRASH_UNWIND_MAX,
};

struct crash_payload {
    const struct crash_code *code;
    void *data;
    uintptr_t params[4];
};

struct report_target;
struct crash_facility {
    const char *name;
    const char *desc;
    const struct crash_code *const *codes; /* list order */
    size_t nr_codes;
    void (*const dump)(struct crash_payload pl);
};

/* The idea here:
 *
 * Each thread has a fixed pool of unwind nodes, with a list that stores
 * the currently active nodes, which are unwound in LIFO order.
 *
 * The reason why it's a pool and not a stack is because you can have code
 * such as
 *
 * mutex_lock(&lock);
 * rcu_read_lock();
 *
 * mutex_unlock(&lock);
 * rcu_read_unlock();
 *
 * and with a naive stack, the semantics break here/become tricky because
 * the thread would have to reorganize the stack, whereas a list_head
 * can just be yanked out of the middle.
 *
 * Thus, the unwind node enqueue path is:
 *
 * (1) try to reserve one
 * (2) list_add_tail it
 *
 * And dequeue becomes:
 *
 * (1) find the pointer to the current thing in question
 * (2) dequeue it
 *
 * RCU is even simpler: the first rcu_read_lock gets a node,
 * and every subsequent lock bumps a counter, dec'ing the counter
 * upon unlock, and the crash path simply rcu_read_unlock's it
 * that many times.
 */

struct crash_unwind_node_data {
    uintptr_t arg;
    union {
        void *ptr;
        size_t rcu_lock_times;
        enum irql irql;
        uintptr_t raw;
    };
};

struct crash_context {
    struct crash_payload payload;
    enum crash_source source;
    enum crash_format_flags formats;
    const char *stmt;
    const char *file;
    int line;
    const char *func;
    const char *msg;
    const struct crash_regs *regs; /* NULL = capture caller's frame via asm */
    void *source_data;
};

#define CRASH_WAIT_US MS_TO_US(500) /* Quiesce timeout per peer CPU */
#define CRASH_SPIN_ONE_US 5
#define CRASH_MAX_DEPTH 2 /* Max recursive fault depth */
#define CRASH_MSG_MAX 256

#define CRASH_PAYLOAD(c, d) ((struct crash_payload) {.code = c, .data = d})
#define CRASH_PARAMS(c, p0, p1, p2, p3)                                        \
    ((struct crash_payload) {.code = (c),                                      \
                             .params = {(uintptr_t) (p0), (uintptr_t) (p1),    \
                                        (uintptr_t) (p2), (uintptr_t) (p3)}})

#define CRASH_CODE_TO_PAYLOAD(c) ((struct crash_payload) {.code = c})

/*
 * Use like this:
 *   #define CRASH_CODES_asan                                \
 *       (USE_AFTER_FREE, "Use after free"),                 \
 *       (PARTIAL_GRANULE)
 *
 *   CRASH_CODE(asan, USE_AFTER_FREE)
 */
#define CRASH_FACILITY(f) __crash_facility_##f

#define CRASH_CODE(f, n) _crash_code_sym(f, n)
#define CRASH_CODE_NTH(f, i) __crash_codes_##f[(i)]

#define CRASH_CODES_ENUM(f, prefix)                                            \
    PP_FOREACH_CTX(_crash_code_enumerator, prefix, CRASH_CODES_##f)
#define crash_code_in(c, f) crash_code_in_facility((c), &CRASH_FACILITY(f))

#define CRASH_CODES_DECLARE(f)                                                 \
    extern const struct crash_facility CRASH_FACILITY(f);                      \
    extern const struct crash_code *const __crash_codes_##f[];                 \
    PP_FOREACH_CTX(_crash_code_extern, f, CRASH_CODES_##f)

#define CRASH_FACILITY_DEFINE(f, d, ...)                                       \
    CRASH_CODES_DECLARE(f);                                                    \
    PP_FOREACH_CTX(_crash_code_def, f, CRASH_CODES_##f)                        \
    const struct crash_code *const __crash_codes_##f[] = {                     \
        PP_FOREACH_CTX(_crash_code_ptr, f, CRASH_CODES_##f)};                  \
    const struct crash_facility CRASH_FACILITY(f) = {                          \
        .name = #f,                                                            \
        .desc = d,                                                             \
        .codes = __crash_codes_##f,                                            \
        .nr_codes = ct_array_size(__crash_codes_##f),                          \
        __VA_ARGS__}

#define _crash_code_sym(f, n) __crash_code_##f##_##n
#define _crash_code_name(t) PP_GET_1 t
#define _crash_code_desc(t) _crash_code_desc_(PP_UNPAREN(t), NULL, ~)
#define _crash_code_desc_(...) _crash_code_desc_pick(__VA_ARGS__)
#define _crash_code_desc_pick(n, d, ...) d

#define _crash_code_extern(f, t) _crash_code_extern_(f, _crash_code_name(t))
#define _crash_code_extern_(f, n)                                              \
    extern const struct crash_code _crash_code_sym(f, n)[1];
#define _crash_code_def(f, t)                                                  \
    _crash_code_def_(f, _crash_code_name(t), _crash_code_desc(t))
#define _crash_code_def_(f, n, d)                                              \
    const struct crash_code _crash_code_sym(f, n)[1] = {                       \
        {PP_STRINGIZE(n), d, &CRASH_FACILITY(f)}};
#define _crash_code_ptr(f, t) _crash_code_ptr_(f, _crash_code_name(t))
#define _crash_code_ptr_(f, n) _crash_code_sym(f, n),
#define _crash_code_enumerator(p, t) PP_CONCAT(p, _crash_code_name(t)),

CRASH_CODES_DECLARE(global);

#define CRASH_CODE_ARG(a) PP_IIF(PP_IS_PAREN(a))(_crash_tuple_code(a), a)

#define _crash_tuple_code(t)                                                   \
    PP_CONCAT(_crash_tuple_code_, _crash_narg(PP_UNPAREN(t))) t
#define _crash_narg(...) PP_NARG(__VA_ARGS__)
#define _crash_tuple_code_1(n) CRASH_CODE(global, n)
#define _crash_tuple_code_2(f, n) CRASH_CODE(f, n)

/* Wraps stmt in the bare-name scope unless code was a tuple */
#define CRASH_CODE_SCOPED(a, ...)                                              \
    PP_CONCAT(_crash_code_scoped_, PP_IS_PAREN(a))(__VA_ARGS__)
#define _crash_code_scoped_1(...) __VA_ARGS__
#define _crash_code_scoped_0(...)                                              \
    ({                                                                         \
        _CRASH_CODE_SCOPE                                                      \
        __VA_ARGS__;                                                           \
    })

#define _crash_code_alias(f, t) _crash_code_alias_(f, _crash_code_name(t))
#define _crash_code_alias_(f, n)                                               \
    cc_unused extern const struct crash_code n[1] asm(PP_STRINGIZE(__USER_LABEL_PREFIX__) PP_STRINGIZE(_crash_code_sym(f, n)));

#define _CRASH_CODE_SCOPE                                                      \
    PP_FOREACH_CTX(_crash_code_alias, global, CRASH_CODES_global)

cc_noreturn void enforce_impl_msg(const char *file, int line, const char *func,
                                  const char *prefix, const char *expr,
                                  const char *fmt, ...) cc_printf_like(6, 7);
/* enforce_impl_msg() with INVALID_ARGUMENT for enforce_precond() */
cc_noreturn void enforce_impl_precond_msg(const char *file, int line,
                                          const char *func, const char *prefix,
                                          const char *expr, const char *fmt,
                                          ...) cc_printf_like(6, 7);
cc_noreturn void enforce_impl_code(const char *file, int line, const char *func,
                                   const char *prefix, const char *expr,
                                   const struct crash_code *code,
                                   const char *fmt, ...) cc_printf_like(7, 8);
cc_noreturn void enforce_impl_code_only(const char *file, int line,
                                        const char *func, const char *prefix,
                                        const char *expr,
                                        const struct crash_code *code);

cc_noreturn void crash_full(const struct crash_context *ctx);

bool crash_cpu_is_owner(uint64_t id);
void crash_broadcast_nmi(void);
size_t crash_code_name(const struct crash_code *code, char *buf, size_t len);
void crash_nmi_handoff(void *p, struct irq_registers *irqc);
void debug_print_stack(void);
void crash_facility_printf(const char *fmt, ...) cc_printf_like(1, 2);
void crash_facility_line(const char *fmt, ...) cc_printf_like(1, 2);

static inline bool crash_code_in_facility(const struct crash_code *code,
                                          const struct crash_facility *f) {
    return code && code->facility == f;
}

#ifdef DEBUG_CRASH

void crash_thread_init(struct thread *t);
void crash_unwind(void);

/* Enter/exit pairs */
void crash_unwind_enter_rcu(void);
void crash_unwind_exit_rcu(void);
void crash_unwind_enter_mutex(struct mutex *m);
void crash_unwind_exit_mutex(struct mutex *m);
void crash_unwind_enter_rwlock(struct rwlock *r);
void crash_unwind_exit_rwlock(struct rwlock *r);
void crash_unwind_enter_spinlock(struct spinlock *s, enum irql old);
void crash_unwind_exit_spinlock(struct spinlock *s);
void crash_unwind_enter_qspinlock(struct qspinlock *q, enum irql old);
void crash_unwind_exit_qspinlock(struct qspinlock *q);

#else
/* Empty static inline versions that cc_ignore the parameters */

static inline void crash_thread_init(struct thread *t) {
    cc_ignore(t);
}

static inline void crash_unwind(void) {}

static inline void crash_unwind_enter_rcu(void) {}
static inline void crash_unwind_exit_rcu(void) {}

static inline void crash_unwind_enter_mutex(struct mutex *m) {
    cc_ignore(m);
}

static inline void crash_unwind_exit_mutex(struct mutex *m) {
    cc_ignore(m);
}

static inline void crash_unwind_enter_rwlock(struct rwlock *r) {
    cc_ignore(r);
}

static inline void crash_unwind_exit_rwlock(struct rwlock *r) {
    cc_ignore(r);
}

static inline void crash_unwind_enter_spinlock(struct spinlock *s,
                                               enum irql old) {
    cc_ignore(s, old);
}

static inline void crash_unwind_exit_spinlock(struct spinlock *s) {
    cc_ignore(s);
}

static inline void crash_unwind_enter_qspinlock(struct qspinlock *q,
                                                enum irql old) {
    cc_ignore(q, old);
}

static inline void crash_unwind_exit_qspinlock(struct qspinlock *q) {
    cc_ignore(q);
}

#endif

static inline void qemu_exit(int code) {
    outb(0xf4, (uint8_t) code);
}
