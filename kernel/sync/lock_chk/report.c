#ifdef DEBUG_LOCK_CHK

#include <atomic.h>
#include <console/crash.h>
#include <console/printf.h>
#include <console/term.h>
#include <enforce.h>
#include <ndjson.h>
#include <nightmare/nightmare.h>
#include <string.h>

#include "internal.h"

NDJSON_DEFINE(lock_chk_finding, NDJSON_SECTION_LOCK_CHK, NDJSON_KIND_FINDING, 1,
              NDJSON_STR(kind), NDJSON_STR(sig), NDJSON_STR(file),
              NDJSON_U64(line), NDJSON_STR(class), NDJSON_STR(msg),
              NDJSON_STR(mode), NDJSON_U64(cycle_len),
              NDJSON_STR(capacity_pool), NDJSON_U64(capacity_used),
              NDJSON_U64(capacity_limit));

static const char *lock_chk_fail_kind_to_str(enum lock_chk_failure_kind kind) {
    return CRASH_CODE_NTH(lock_chk, kind)->name;
}

static const char *lock_chk_mode_name(enum lock_chk_mode mode) {
    if (mode == LOCK_CHK_MODE_EXCLUSIVE)
        return "exclusive";
    if (mode == LOCK_CHK_MODE_SHARED)
        return "shared";
    return "";
}

static const char *lock_chk_fault_msg(const struct lock_chk_fault *fault) {
    if (fault->report == NULL || fault->report->msg[0] == '\0')
        return "lock_chk failure (report buffer busy)";

    return fault->report->msg;
}

static uint16_t lock_chk_fault_cycle_len(const struct lock_chk_fault *fault) {
    return fault->report != NULL ? fault->report->cycle_len : 0;
}

static uint64_t lock_chk_calc_generic_sig(const struct lock_chk_fault *fault) {
    uint64_t signature = HASH_FNV1A_64_OFFSET_BASIS;
    signature =
        lock_chk_hash_str(signature, lock_chk_fail_kind_to_str(fault->kind));
    const struct lock_chk_class *class = fault->class;
    if (class != NULL) {
        signature = lock_chk_hash_str(signature, class->name);
        signature = lock_chk_hash_str(signature, class->file);
        signature =
            lock_chk_hash_bytes(signature, &class->line, sizeof(class->line));
    }
    if (fault->site != NULL) {
        signature = lock_chk_hash_str(signature, fault->site->file);
        signature = lock_chk_hash_bytes(signature, &fault->site->line,
                                        sizeof(fault->site->line));
    }
    signature = lock_chk_hash_bytes(signature, &fault->subclass,
                                    sizeof(fault->subclass));
    return lock_chk_hash_bytes(signature, &fault->mode, sizeof(fault->mode));
}

static uint64_t lock_chk_fail_sig(const struct lock_chk_fault *fault) {
    const struct lock_chk_report *report = fault->report;
    if (report != NULL) {
        if (report->signature != 0)
            return report->signature;
        if (fault->kind == LOCK_CHK_FAIL_CYCLE && report->cycle_len != 0)
            return lock_chk_calc_sig(report->cycle_hops, report->cycle_len);
    }

    return lock_chk_calc_generic_sig(fault);
}

static void lock_chk_emit_finding(const struct lock_chk_fault *fault,
                                  const char *kind, const char *signature) {
    ndjson_emit(lock_chk_finding, .kind = kind, .sig = signature,
                .file = fault->site ? fault->site->file : "",
                .line = fault->site ? fault->site->line : 0,
                .class = fault->class ? fault->class->name : "",
                .msg = lock_chk_fault_msg(fault),
                .mode = lock_chk_mode_name(fault->mode),
                .cycle_len = lock_chk_fault_cycle_len(fault),
                .capacity_pool =
                    fault->capacity_pool ? fault->capacity_pool : "",
                .capacity_used = fault->capacity_used,
                .capacity_limit = fault->capacity_limit);
}

static void maybe_report_degradation(const struct lock_chk_fault *fault,
                                     uint64_t signature,
                                     const char *signature_text) {
    static atomic_bool emitted = false;
    if (atomic_xchg_rlx(&emitted, true))
        return;

    printf_unlocked("\n*** LOCK_CHK WARNING: Capacity exhausted for pool '%s' "
                    "(%u/%u) because DEGRADED***\n\n",
                    fault->capacity_pool ? fault->capacity_pool : "<unknown>",
                    fault->capacity_used, fault->capacity_limit);
    lock_chk_emit_finding(fault, "CAPACITY_DEGRADED", signature_text);
    nightmare_request_external_fail("lock_chk_degraded", signature, "%s",
                                    lock_chk_fault_msg(fault));
}

static void lock_chk_crash_dump(struct crash_payload pl);

CRASH_FACILITY_DEFINE(lock_chk, "Lock dependency validator",
                      .dump = lock_chk_crash_dump);

#define lock_chk_dump_field(label, fmt, ...)                                   \
    crash_facility_line("%s%-9s%s " fmt, term_style(TERM_SEV_LABEL), label,    \
                        term_style_reset(), ##__VA_ARGS__)

static const char *lock_chk_class_name(const struct lock_chk_class *class) {
    return class ? class->name : "<unknown>";
}

static const char *lock_chk_basename(const char *path) {
    const char *slash = path ? strrchr(path, '/') : NULL;
    return slash ? slash + 1 : (path ? path : "?");
}

static void dump_cycle(const struct lock_chk_report *report) {
    crash_facility_line("");
    crash_facility_line(
        "%scycle%s %s(%u hops%s)%s", term_style(TERM_SEV_LABEL),
        term_style_reset(), term_style(TERM_SEV_DIM), report->cycle_len,
        report->cycle_truncated ? ", truncated" : "", term_style_reset());

    for (uint16_t i = 0; i < report->cycle_len; i++) {
        const struct lock_chk_cycle_hop *hop = &report->cycle_hops[i];

        crash_facility_line(
            "%s[%u]%s '%s'/%u %s%s%s", term_style(TERM_SEV_DIM), i,
            term_style_reset(), lock_chk_class_name(hop->from_class),
            hop->from_subclass, term_style(TERM_SEV_DIM),
            lock_chk_mode_name(hop->from_mode), term_style_reset());
        crash_facility_line(
            "    %s->%s '%s'/%u %s%s%s", term_style(TERM_SEV_WARN),
            term_style_reset(), lock_chk_class_name(hop->to_class),
            hop->to_subclass, term_style(TERM_SEV_DIM),
            lock_chk_mode_name(hop->to_mode), term_style_reset());
        crash_facility_line(
            "       %sat %s:%u%s", term_style(TERM_SEV_DIM),
            lock_chk_basename(hop->site ? hop->site->file : NULL),
            hop->site ? hop->site->line : 0, term_style_reset());
    }
}

static void lock_chk_crash_dump(struct crash_payload pl) {
    const struct lock_chk_fault *fault = pl.data;
    uint64_t signature = pl.params[0];

    if (!fault)
        return;

    lock_chk_dump_field("signature", "%016lx", signature);

    const struct lock_chk_class *class = fault->class;
    if (class) {
        lock_chk_dump_field("class", "'%s'/%u", class->name, fault->subclass);
        lock_chk_dump_field(
            "", "%sdeclared at %s:%u%s", term_style(TERM_SEV_DIM),
            lock_chk_basename(class->file), class->line, term_style_reset());
    }

    if (fault->instance)
        lock_chk_dump_field("instance", "%s %p",
                            lock_chk_type_to_str(fault->type), fault->instance);

    if (fault->mode == LOCK_CHK_MODE_EXCLUSIVE ||
        fault->mode == LOCK_CHK_MODE_SHARED)
        lock_chk_dump_field("mode", "%s", lock_chk_mode_name(fault->mode));

    if (fault->kind == LOCK_CHK_FAIL_CAPACITY)
        lock_chk_dump_field("capacity", "'%s' used %u / limit %u",
                            fault->capacity_pool ? fault->capacity_pool
                                                 : "<unknown>",
                            fault->capacity_used, fault->capacity_limit);

    const struct lock_chk_report *report = fault->report;
    if (fault->kind == LOCK_CHK_FAIL_CYCLE && report && report->cycle_len)
        dump_cycle(report);
}

void lock_chk_report_fail(const struct lock_chk_fault *fault) {
    uint64_t signature = lock_chk_fail_sig(fault);
    char signature_text[24];
    snprintf(signature_text, sizeof(signature_text), "%016lx", signature);

    if (fault->kind == LOCK_CHK_FAIL_CAPACITY &&
        !lock_chk_global.panic_on_exhaustion) {
        maybe_report_degradation(fault, signature, signature_text);
        lock_chk_report_release();
        return;
    }

    lock_chk_emit_finding(fault, lock_chk_fail_kind_to_str(fault->kind),
                          signature_text);
    nightmare_request_external_fail("lock_chk", signature, "%s",
                                    lock_chk_fault_msg(fault));

    crash_full(&(struct crash_context){
        .payload = {.code = CRASH_CODE_NTH(lock_chk, fault->kind),
                    .data = (void *) fault,
                    .params = {signature}},
        .source = CRASH_SOURCE_LOCK_CHK,
        .formats = CRASH_FMT_DEFAULT,
        .file = fault->site ? fault->site->file : __RELFILE__,
        .line = fault->site ? fault->site->line : __LINE__,
        .func = "lock_chk",
        .msg = lock_chk_fault_msg(fault),
        .regs = NULL,
    });
}

#endif /* DEBUG_LOCK_CHK */
