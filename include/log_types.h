/* @title: Logging Types */
#pragma once
#include <atomic.h>
#include <bootstage.h>
#include <linker/symbols.h>
#include <sch/irql.h>
#include <structures/list.h>
#include <types/types.h>

struct log_record;
struct log_site;

#define LOG_NARGS 10

enum log_handle_flags : uint32_t {
    LOG_HANDLE_PRINT = 1 << 0,     /* emit to console immediately */
    LOG_HANDLE_IMPORTANT = 1 << 1, /* never drop / elevated visibility */
    LOG_HANDLE_RATELIMIT = 1 << 2, /* suppress floods */
    LOG_HANDLE_ONCE = 1 << 3,      /* print only first occurrence */
    LOG_HANDLE_PANIC = 1 << 4,     /* fatal if level >= ERROR */
    LOG_HANDLE_NO_NEWLINE = 1 << 5,
    LOG_HANDLE_FLAGS_DEFAULT = 0,
};

enum log_level : uint8_t {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

enum log_site_flags : uint32_t {
    LOG_SITE_PRINT = 1 << 0,    /* print all logs in site */
    LOG_SITE_DROP_OLD = 1 << 1, /* overwrite oldest on overflow */
    LOG_SITE_NO_IRQ = 1 << 2,   /* suppress in IRQ context */
    LOG_SITE_PANIC_VISIBLE = 1 << 3,

    LOG_SITE_NDJSON = 1 << 4, /* emit records directly to NDJSON wire */
    LOG_SITE_NONE = 0,
    LOG_SITE_DEFAULT = LOG_SITE_DROP_OLD,
};

enum log_record_flags : uint16_t {
    LOG_REC_FROM_IRQ = 1 << 0,
    LOG_REC_TRUNCATED = 1 << 1,
};

/* Governs what to print */
struct log_dump_options {
    uint8_t min_level;
    bool show_cpu : 1;
    bool show_tid : 1;
    bool show_irql : 1;
    bool show_caller : 1;
    bool resolve_symbols : 1;
    bool clear_after_dump : 1;
};

struct log_handle {
    void (*print)(const struct log_site *site, const struct log_record *rec,
                  void (*print)(const char *fmt, ...) cc_printf_like(1, 2));
    enum log_handle_flags flags;
    atomic_uint32_t seen_internal;
    atomic_uint64_t last_ts_internal;
};

struct log_record {
    time_ms_t timestamp;
    cpu_id_t cpu;
    uint32_t tid;

    const struct log_handle *handle;
    enum log_level level;
    enum bootstage boostage;

    uint16_t msg_len;

    char *fmt; /* Becomes a pointer to the shadow_buf in cases where
                * LOG_SITE_DUP_MESSAGES is set */
    uint8_t nargs;
    uint64_t args[LOG_NARGS];

    char *caller_fn;
    char *caller_file;
    int32_t caller_line;
    uintptr_t caller_pc;
    enum log_record_flags flags;
    enum irql logged_at_irql;
};

struct log_ring_slot {
    atomic_uint64_t seq;
    struct log_record rec;
    char *shadow_buf; /* With a len of msg_max_len */
};

struct log_ringbuf {
    struct log_ring_slot *slots;

    atomic_uint64_t head;
    atomic_uint64_t tail;
};

struct log_site {
    struct list_node list;
    char *name;
    struct log_ringbuf rb;
    uint32_t enabled_mask; /* Governs what actually goes in the site */
    size_t capacity;

    /* Only relevant for dynamic log sites */
    refcount_t refcount;

    size_t msg_max_len;
    uint32_t dropped; /* Accumulation of all missed logs */
    enum log_site_flags flags;
    struct log_dump_options dump_opts; /* If LOG_SITE_PRINT, use this */
};
