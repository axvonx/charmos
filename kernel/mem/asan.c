#include <compiler/intrinsic.h>
#include <console/crash.h>
#include <console/term.h>
#include <dbg.h>
#include <enforce.h>
#include <global.h>
#include <math/align.h>
#include <math/min_max.h>
#include <mem/asan.h>
#include <mem/hhdm.h>
#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vmm.h>
#include <ndjson.h>
#include <stack_depot.h>
#include <stdint.h>
#include <string.h>
#include <sync/spinlock.h>

#include "mem/slab/internal.h"

#ifdef DEBUG_ASAN
LOG_SITE_DEFINE_PRINT(asan);
LOG_HANDLE_DEFINE_PRINT(asan);

/* If we widen this, ASAN_SHADOW_OFFSET must be recomputed, because right now
 * it maps exactly this window into the shadow region */
#define ASAN_COVERED_START SLAB_HEAP_START
#define ASAN_COVERED_END SLAB_HEAP_END

static bool asan_ready = false;

static cc_noreturn void asan_runtime_panic(const char *what, const void *addr,
                                           size_t size, const char *file,
                                           int line, const char *func);

#define kasan_panic(what, addr, size)                                          \
    asan_runtime_panic((what), (addr), (size), __FILE__, __LINE__, __func__)

/* Two pages that everything aliases to for pages that are fully OK/not OK to
 * read/modify */
static paddr_t asan_zero_shadow_phys;  /* never had a chunk: reads accessible */
static paddr_t asan_freed_shadow_phys; /* had one, gave it back: reads freed */

static inline bool asan_addr_covered(uintptr_t a, size_t size) {
    return a >= ASAN_COVERED_START && (a + size) <= ASAN_COVERED_END;
}

static inline uint8_t *asan_shadow_for_internal(const void *addr) {
    return (uint8_t *) ASAN_SHADOW_ADDR(addr);
}

/* Number of shadow bytes covering [addr, addr + size) */
static inline size_t asan_shadow_span(const void *addr, size_t size) {
    const uint8_t *end = (const uint8_t *) addr + size - 1;
    return (size_t) (asan_shadow_for_internal(end) -
                     asan_shadow_for_internal(addr)) +
           1;
}

static void asan_mark_valid(void *addr, size_t size) {
    enforce(IS_ALIGNED((uintptr_t) addr, ASAN_GRANULE));

    uint8_t *shadow = asan_shadow_for_internal(addr);
    size_t full = size >> ASAN_SHADOW_SCALE;
    size_t tail = size & (ASAN_GRANULE - 1);

    memset(shadow, 0, full);
    if (tail)
        shadow[full] = (uint8_t) tail;
}

static void asan_mark_poisoned(void *addr, size_t size, uint8_t value) {
    enforce(IS_ALIGNED((uintptr_t) addr, ASAN_GRANULE));

    memset(asan_shadow_for_internal(addr), value, asan_shadow_span(addr, size));
}

void asan_alloc(void *addr, size_t requested, size_t slot) {
    ASAN_ABORT_IF_NOT_READY();

    if (!slot || !asan_addr_covered((uintptr_t) addr, slot))
        return;

    enforce(requested && requested <= slot);

    asan_mark_valid(addr, requested);

    size_t redzone = ALIGN_UP(requested, ASAN_GRANULE);
    if (redzone < slot)
        asan_mark_poisoned((uint8_t *) addr + redzone, slot - redzone,
                           ASAN_POISON_HEAP_REDZONE);
}

void asan_free(void *addr, size_t slot) {
    ASAN_ABORT_IF_NOT_READY();

    if (!slot || !asan_addr_covered((uintptr_t) addr, slot))
        return;

    asan_mark_poisoned(addr, slot, ASAN_POISON_HEAP_FREED);
}

void asan_poison(void *addr, size_t size) {
    ASAN_ABORT_IF_NOT_READY();

    if (!size || !asan_addr_covered((uintptr_t) addr, size))
        return;

    asan_mark_poisoned(addr, size, ASAN_POISON_VALUE);
}

void asan_unpoison(void *addr, size_t size) {
    ASAN_ABORT_IF_NOT_READY();

    if (!size || !asan_addr_covered((uintptr_t) addr, size))
        return;

    asan_mark_valid(addr, size);
}

static inline bool asan_shadow_is_shared(vaddr_t shadow_page) {
    paddr_t p = vmm_get_phys(shadow_page);
    return p == asan_zero_shadow_phys || p == asan_freed_shadow_phys;
}

static bool asan_page_is_uniform(const void *addr, uint8_t value) {
    const uint64_t *p = addr;
    uint64_t want = value * 0x0101010101010101ULL;

    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++) {
        if (p[i] != want)
            return false;
    }

    return true;
}

static inline vaddr_t asan_shadow_page_first(vaddr_t base) {
    return ALIGN_DOWN(ASAN_SHADOW_ADDR(base), PAGE_SIZE);
}

static inline vaddr_t asan_shadow_page_limit(vaddr_t base, size_t len) {
    return ALIGN_UP(ASAN_SHADOW_ADDR(base + len - 1) + 1, PAGE_SIZE);
}

enum err asan_shadow_install(vaddr_t base, size_t len) {
    if (!asan_ready)
        return ERR_OK;

    if (!len || !asan_addr_covered(base, len))
        return ERR_OK;

    vaddr_t limit = asan_shadow_page_limit(base, len);

    for (vaddr_t v = asan_shadow_page_first(base); v < limit; v += PAGE_SIZE) {
        if (!asan_shadow_is_shared(v))
            continue;

        /* Per page rather than once up front, so a range of any length works:
         * the walk is cheap and only happens for a page we are about to back */
        enum err err = vmm_unshare_path(v);
        if (err < 0)
            return err;

        paddr_t phys = pmm_alloc_page();
        if (!phys)
            return ERR_NO_MEM;

        err = vmm_map_page(v, phys,
                           .page_flags = PAGE_PRESENT | PAGE_WRITE | PAGE_XD,
                           .vmm_flags = VMM_FLAG_MODIFY_LEAF);
        if (err < 0) {
            pmm_free_page(phys);
            return err;
        }

        memset((void *) v, ASAN_POISON_VALUE, PAGE_SIZE);
    }

    return ERR_OK;
}

void asan_shadow_release(vaddr_t base, size_t len) {
    if (!asan_ready)
        return;

    if (!len || !asan_addr_covered(base, len))
        return;

    vaddr_t limit = asan_shadow_page_limit(base, len);

    for (vaddr_t v = asan_shadow_page_first(base); v < limit; v += PAGE_SIZE) {
        if (asan_shadow_is_shared(v))
            continue;

        if (!asan_page_is_uniform((void *) v, ASAN_POISON_HEAP_FREED))
            continue;

        paddr_t phys = vmm_get_phys(v);

        if (vmm_map_page(v, asan_freed_shadow_phys,
                         .page_flags = PAGE_PRESENT | PAGE_XD,
                         .vmm_flags = VMM_FLAG_MODIFY_LEAF) < 0)
            continue;

        pmm_free_page(phys);
    }
}

static void asan_map_early_shadow(void) {
    vaddr_t start = ASAN_SHADOW_ADDR(ASAN_COVERED_START);
    vaddr_t end = ASAN_SHADOW_ADDR(ASAN_COVERED_END);

    asan_zero_shadow_phys = pmm_alloc_page(ALLOC_FLAGS_ZERO);
    if (!asan_zero_shadow_phys)
        kasan_panic("could not allocate the shared shadow page", NULL, 0);

    asan_freed_shadow_phys = pmm_alloc_page();
    if (!asan_freed_shadow_phys)
        kasan_panic("could not allocate the shared freed shadow page", NULL, 0);

    memset(hhdm_paddr_to_ptr(asan_freed_shadow_phys), ASAN_POISON_HEAP_FREED,
           PAGE_SIZE);

    /* Deliberately RO and aliased */
    enum err err = vmm_map_aliased(start, end - start, asan_zero_shadow_phys,
                                   .page_flags = PAGE_PRESENT | PAGE_XD);
    if (err < 0)
        kasan_panic("could not map the shared shadow window",
                    (const void *) start, end - start);

    asan_info("shared shadow: [%lx, %lx), %zu GiB of window on one zero page "
              "at %lx\n",
              start, end, (size_t) ((end - start) / GIB(1)),
              asan_zero_shadow_phys);
}

void asan_init(void) {
    asan_info("bringing ASAN up...");

    asan_map_early_shadow();

    asan_ready = true;
}

NDJSON_DEFINE(asan_fault, NDJSON_SECTION_ASAN, NDJSON_KIND_FAULT, 1,
              NDJSON_STR(what), NDJSON_HEX(addr), NDJSON_U64(size),
              NDJSON_STR(access));

NDJSON_DEFINE(asan_frame, NDJSON_SECTION_ASAN, NDJSON_KIND_FRAME, 1,
              NDJSON_U64(idx), NDJSON_HEX(addr), NDJSON_STR(sym),
              NDJSON_U64(off));

#define CRASH_CODES_asan                                                       \
    (USE_AFTER_FREE, "Use after free"),                                        \
        (HEAP_REDZONE, "Heap out of bounds (redzone)"),                        \
        (UNALLOCATED, "Access to unallocated memory"),                         \
        (PARTIAL_GRANULE, "Heap overflow (partial granule)"),                  \
        (POISONED, "Access to poisoned memory"),                               \
        (UNCLASSIFIED, "Bad memory access"), (RUNTIME, "ASAN runtime failure")
CRASH_CODES_DECLARE(asan);

#define ASAN_REPORT_SHADOW_ROWS 3
#define ASAN_REPORT_SHADOW_COLS 8 /* fits the facility box at 80 columns */
#define ASAN_REPORT_NOTE_MAX 128

struct asan_report {
    const void *addr;
    size_t size;
    bool is_write;
    bool has_shadow;
    uint8_t shadow[ASAN_REPORT_SHADOW_ROWS][ASAN_REPORT_SHADOW_COLS];
    char owner[ASAN_REPORT_NOTE_MAX]; /* empty = not a slab object */
    stack_handle_t owner_trace;
    struct slab *owner_slab; /* NULL until the object is pinned down */
    size_t owner_idx;
};

static const struct crash_code *asan_poison_code(uint8_t shadow) {
    switch (shadow) {
    case ASAN_POISON_HEAP_FREED: return CRASH_CODE(asan, USE_AFTER_FREE);
    case ASAN_POISON_HEAP_REDZONE: return CRASH_CODE(asan, HEAP_REDZONE);
    case ASAN_POISON_VALUE: return CRASH_CODE(asan, UNALLOCATED);
    default:
        return asan_shadow_is_poison(shadow)
                   ? CRASH_CODE(asan, POISONED)
                   : CRASH_CODE(asan, PARTIAL_GRANULE);
    }
}

static void asan_crash_dump(struct crash_payload pl);

CRASH_FACILITY_DEFINE(asan, "Kernel address sanitizer",
                      .dump = asan_crash_dump);

static void asan_dump_shadow(const struct asan_report *r) {
    char row[256]; /* room for a color escape around every byte */

    crash_facility_line("%sshadow bytes%s", term_style(TERM_SEV_LABEL),
                        term_style_reset());

    for (int i = 0; i < ASAN_REPORT_SHADOW_ROWS; i++) {
        int rel = i - ASAN_REPORT_SHADOW_ROWS / 2;
        size_t n = 0;

        for (int j = 0; j < ASAN_REPORT_SHADOW_COLS; j++) {
            bool fault = rel == 0 && j == 0;
            uint8_t b = r->shadow[i][j];
            const char *style = fault ? term_style(TERM_SEV_CRIT)
                                : b   ? term_style(TERM_SEV_WARN)
                                      : term_style(TERM_SEV_DIM);

            n += (size_t) snprintf(row + n, (int) (sizeof(row) - n),
                                   "%s%02x%s ", style, b, term_style_reset());
            if (n >= sizeof(row))
                break;
        }

        char off[16];
        snprintf(off, (int) sizeof(off), "%s%d", rel > 0 ? "+" : "",
                 rel * ASAN_REPORT_SHADOW_COLS * (int) ASAN_GRANULE);
        crash_facility_line("%s%6s%s  %s", term_style(TERM_SEV_DIM), off,
                            term_style_reset(), row);
    }
}

static void asan_dump_trace(stack_handle_t h) {
    struct stack_depot_record *rec = stack_depot_get_record(h);

    if (!rec) {
        crash_facility_line("  %s<trace no longer in the depot>%s",
                            term_style(TERM_SEV_DIM), term_style_reset());
        return;
    }

    for (size_t i = 0; i < rec->num_entries; i++) {
        uint64_t off = 0;
        const char *sym = debug_symbolize(rec->entries[i], &off);

        if (sym)
            crash_facility_line("  %s#%-2zu%s %s+0x%lx",
                                term_style(TERM_SEV_DIM), i, term_style_reset(),
                                sym, off);
        else
            crash_facility_line("  %s#%-2zu%s %016lx", term_style(TERM_SEV_DIM),
                                i, term_style_reset(), rec->entries[i]);
    }
}

static void asan_crash_dump(struct crash_payload pl) {
    const struct asan_report *r = pl.data;

    if (!r)
        return;

    crash_facility_line("%saddress%s %p", term_style(TERM_SEV_LABEL),
                        term_style_reset(), r->addr);

    if (pl.code != CRASH_CODE(asan, RUNTIME))
        crash_facility_line("%saccess%s  %zu-byte %s",
                            term_style(TERM_SEV_LABEL), term_style_reset(),
                            r->size, r->is_write ? "store" : "load");
    else if (r->size)
        crash_facility_line("%ssize%s    0x%zx", term_style(TERM_SEV_LABEL),
                            term_style_reset(), r->size);

    if (r->has_shadow) {
        crash_facility_line("");
        asan_dump_shadow(r);
    }

    if (r->owner[0]) {
        crash_facility_line("");
        crash_facility_printf("%sowner%s %s", term_style(TERM_SEV_LABEL),
                              term_style_reset(), r->owner);
        if (r->owner_trace)
            asan_dump_trace(r->owner_trace);
    }
}

static void asan_collect_shadow(struct asan_report *r) {
    if (!asan_ready || !asan_addr_covered((uintptr_t) r->addr, 1))
        return;

    const uint8_t *sh = asan_shadow_for_internal(r->addr);
    int mid = ASAN_REPORT_SHADOW_ROWS / 2;

    for (int i = 0; i < ASAN_REPORT_SHADOW_ROWS; i++)
        for (int j = 0; j < ASAN_REPORT_SHADOW_COLS; j++)
            r->shadow[i][j] = sh[(i - mid) * ASAN_REPORT_SHADOW_COLS + j];

    r->has_shadow = true;
}

static void asan_emit_trace(stack_handle_t h) {
    struct stack_depot_record *rec = stack_depot_get_record(h);

    if (!rec)
        return;

    for (size_t i = 0; i < rec->num_entries; i++) {
        uint64_t off = 0;
        const char *sym = debug_symbolize(rec->entries[i], &off);

        ndjson_emit(asan_frame, .idx = i, .addr = rec->entries[i], .sym = sym,
                    .off = off);
    }
}

#define asan_owner_note(r, ...)                                                \
    snprintf((r)->owner, sizeof((r)->owner), __VA_ARGS__)

/* Slabs keep a stack depot handle per object when DEBUG_SLAB_DEEP, so
 * we must tread carefully when retrieving the backtrace, lest we
 * dereference an unmapped page or cause other problems */
static void asan_collect_owner(struct asan_report *r) {
#ifdef DEBUG_SLAB_DEEP
    static bool reporting;
    void *addr = (void *) r->addr;

    if (reporting || !slab_ptr_in_slab(addr))
        return;
    reporting = true;

    if (slab_order_map_get((vaddr_t) addr) == SLAB_POW2_ORDER_EMPTY) {
        asan_owner_note(r, "in the slab heap but no chunk is mapped there");
        goto out;
    }

    struct slab *s = slab_for_ptr(addr);
    struct slab_cache *cache = s->parent_cache;

    /* Metadata may be what went wrong, so sanity check before trusting it */
    if ((uintptr_t) cache < SLAB_HEAP_START || !cache->obj_stride ||
        !cache->objs_per_slab || s->mem < (vaddr_t) s ||
        (vaddr_t) addr < s->mem) {
        asan_owner_note(r, "slab metadata at %p is not usable (cache=%p)",
                        (void *) s, (void *) cache);
        goto out;
    }

    size_t idx = slab_allocation_index(s, addr);
    if (idx >= cache->objs_per_slab) {
        asan_owner_note(r, "object index %zu out of range for slab %p", idx,
                        (void *) s);
        goto out;
    }

    stack_handle_t h = s->traces[idx];
    if (!h) {
        asan_owner_note(r, "object %zu of slab %p has no owner", idx,
                        (void *) s);
        goto out;
    }

    asan_owner_note(r, "object %zu of slab %p, last allocated at:", idx,
                    (void *) s);
    r->owner_slab = s;
    r->owner_idx = idx;
    r->owner_trace = h;
    asan_emit_trace(h);

out:
    reporting = false;
#else
    cc_ignore(r);
#endif
}

static cc_noreturn void asan_crash(const struct crash_code *code,
                                   const char *msg, struct asan_report *r,
                                   const char *file, int line,
                                   const char *func) {
    crash_full(&(struct crash_context){
        .payload = CRASH_PAYLOAD(code, r),
        .source = CRASH_SOURCE_KASAN,
        .formats = CRASH_FMT_DEFAULT,
        .file = file,
        .line = line,
        .func = func,
        .msg = msg,
    });
}

static cc_noreturn void asan_runtime_panic(const char *what, const void *addr,
                                           size_t size, const char *file,
                                           int line, const char *func) {
    struct asan_report r = {.addr = addr, .size = size};

    ndjson_emit(asan_fault, .what = what, .addr = (uint64_t) (uintptr_t) addr,
                .size = size, .access = "");
    asan_crash(CRASH_CODE(asan, RUNTIME), what, &r, file, line, func);
}

/* First shadow byte that makes [addr, addr + size) bad, 0 if all of it is OK */
static inline uint8_t asan_bad_shadow(const void *addr, size_t size) {
    if (size == 0 || !asan_addr_covered((uintptr_t) addr, size))
        return 0;

    uintptr_t start = (uintptr_t) addr;
    uintptr_t last = start + (size - 1);

    for (uintptr_t g = ALIGN_DOWN(start, ASAN_GRANULE); g <= last;
         g += ASAN_GRANULE) {
        uint8_t s = *asan_shadow_for_internal((const void *) g);
        if (s == 0)
            continue;

        /* Bytes of this granule the access actually touches: [lo, hi) */
        uintptr_t hi = MIN(last - g + 1, ASAN_GRANULE);

        /* A partial granule keeps its first `s` bytes accessible */
        if (!asan_shadow_is_poison(s) && hi <= s)
            continue;

        return s;
    }

    return 0;
}

static cc_noreturn void asan_report_access(const void *addr, size_t size,
                                           bool is_write, void *pc) {
    uint8_t s = asan_ready ? asan_bad_shadow(addr, size) : 0;
    const struct crash_code *code =
        s ? asan_poison_code(s) : CRASH_CODE(asan, UNCLASSIFIED);
    const char *access = is_write ? "store" : "load";
    struct asan_report r = {.addr = addr, .size = size, .is_write = is_write};
    char msg[CRASH_MSG_MAX];

    ndjson_emit(asan_fault, .what = code->desc,
                .addr = (uint64_t) (uintptr_t) addr, .size = size,
                .access = access);

    asan_collect_shadow(&r);
    asan_collect_owner(&r);

    int n = snprintf(msg, sizeof(msg), "%zu-byte %s at %p", size, access, addr);
    if (r.owner_slab && n > 0 && (size_t) n < sizeof(msg))
        snprintf(msg + n, sizeof(msg) - (size_t) n, " (object %zu of slab %p)",
                 r.owner_idx, (void *) r.owner_slab);

    uint64_t site = (uint64_t) (uintptr_t) pc - 1;
    uint64_t off = 0;
    uint32_t line = 0;
    const char *func = debug_symbolize(site, &off);
    const char *file = debug_line_for(site, &line);

    asan_crash(code, msg, &r, file, (int) line, func);
}

static inline void asan_check_access_core(const void *addr, size_t size,
                                          bool is_write, void *pc) {
    ASAN_ABORT_IF_NOT_READY();

    if (asan_bad_shadow(addr, size))
        asan_report_access(addr, size, is_write, pc);
}

void __asan_load1(const void *addr) {
    asan_check_access_core(addr, 1, false, ci_return_address(0));
}
void __asan_load2(const void *addr) {
    asan_check_access_core(addr, 2, false, ci_return_address(0));
}
void __asan_load4(const void *addr) {
    asan_check_access_core(addr, 4, false, ci_return_address(0));
}
void __asan_load8(const void *addr) {
    asan_check_access_core(addr, 8, false, ci_return_address(0));
}
void __asan_load16(const void *addr) {
    asan_check_access_core(addr, 16, false, ci_return_address(0));
} /* some compilers */

void __asan_store1(const void *addr) {
    asan_check_access_core(addr, 1, true, ci_return_address(0));
}
void __asan_store2(const void *addr) {
    asan_check_access_core(addr, 2, true, ci_return_address(0));
}
void __asan_store4(const void *addr) {
    asan_check_access_core(addr, 4, true, ci_return_address(0));
}
void __asan_store8(const void *addr) {
    asan_check_access_core(addr, 8, true, ci_return_address(0));
}
void __asan_store16(const void *addr) {
    asan_check_access_core(addr, 16, true, ci_return_address(0));
}

/* Generic wrappers the compiler sometimes uses */
void __asan_loadN(const void *addr, size_t size) {
    asan_check_access_core(addr, size, false, ci_return_address(0));
}
void __asan_storeN(const void *addr, size_t size) {
    asan_check_access_core(addr, size, true, ci_return_address(0));
}

/* The compiler's spelling of the same two operations */
void __asan_poison_memory_region(void *addr, size_t size) {
    asan_poison(addr, size);
}

void __asan_unpoison_memory_region(void *addr, size_t size) {
    asan_unpoison(addr, size);
}

struct __asan_global {
    void *addr;
    size_t size;
    const char *name;
    /* some targets include more fields; we ignore them */
};

void __asan_register_globals(struct __asan_global *globals, size_t n) {
    ASAN_ABORT_IF_NOT_READY();
    const size_t redzone = 16;
    for (size_t i = 0; i < n; i++) {
        void *addr = globals[i].addr;
        size_t size = globals[i].size;
        if (!addr || size == 0)
            continue;

        /* poison left redzone (if address is valid) */
        if ((uintptr_t) addr >= redzone) /* simple check */
            __asan_poison_memory_region((uint8_t *) addr - redzone, redzone);
        __asan_unpoison_memory_region(addr, size);
        __asan_poison_memory_region((uint8_t *) addr + size, redzone);
    }
}

void __asan_unregister_globals(void *globals, size_t n) {
    cc_ignore(globals, n);
}

#define ASAN_MAX_STACK_RECORDS 1024
struct stack_record {
    void *addr;
    size_t size;
};

static struct stack_record stack_records[ASAN_MAX_STACK_RECORDS];
static size_t stack_records_count = 0;

void __asan_stack_malloc(void *addr, size_t size) {
    ASAN_ABORT_IF_NOT_READY();
    if (!addr || size == 0)
        return;
    const size_t rz = 16;
    __asan_poison_memory_region((uint8_t *) addr - rz, size + rz * 2);
    __asan_unpoison_memory_region(addr, size);

    if (stack_records_count < ASAN_MAX_STACK_RECORDS) {
        stack_records[stack_records_count].addr = addr;
        stack_records[stack_records_count].size = size;
        stack_records_count++;
    }
}

void __asan_stack_free(void *addr) {
    ASAN_ABORT_IF_NOT_READY();
    for (size_t i = 0; i < stack_records_count; i++) {
        if (stack_records[i].addr == addr) {
            size_t size = stack_records[i].size;
            const size_t rz = 16;
            __asan_poison_memory_region((uint8_t *) addr - rz, size + rz * 2);
            /* shift tail down */
            stack_records[i] = stack_records[stack_records_count - 1];
            stack_records_count--;
            return;
        }
    }
}

void __asan_report_load1(void *addr) {
    asan_report_access(addr, 1, false, ci_return_address(0));
}
void __asan_report_load2(void *addr) {
    asan_report_access(addr, 2, false, ci_return_address(0));
}
void __asan_report_load4(void *addr) {
    asan_report_access(addr, 4, false, ci_return_address(0));
}
void __asan_report_load8(void *addr) {
    asan_report_access(addr, 8, false, ci_return_address(0));
}
void __asan_report_load16(void *addr) {
    asan_report_access(addr, 16, false, ci_return_address(0));
}

void __asan_report_load32(void *addr) {
    asan_report_access(addr, 32, false, ci_return_address(0));
}

void __asan_report_load64(void *addr) {
    asan_report_access(addr, 64, false, ci_return_address(0));
}

void __asan_report_load_n(void *addr, size_t size) {
    asan_report_access(addr, size, false, ci_return_address(0));
}

void __asan_report_store1(void *addr) {
    asan_report_access(addr, 1, true, ci_return_address(0));
}
void __asan_report_store2(void *addr) {
    asan_report_access(addr, 2, true, ci_return_address(0));
}
void __asan_report_store4(void *addr) {
    asan_report_access(addr, 4, true, ci_return_address(0));
}
void __asan_report_store8(void *addr) {
    asan_report_access(addr, 8, true, ci_return_address(0));
}
void __asan_report_store16(void *addr) {
    asan_report_access(addr, 16, true, ci_return_address(0));
}

void __asan_report_store32(void *addr) {
    asan_report_access(addr, 32, true, ci_return_address(0));
}

void __asan_report_store64(void *addr) {
    asan_report_access(addr, 64, true, ci_return_address(0));
}

void __asan_report_store_n(void *addr, size_t size) {
    asan_report_access(addr, size, true, ci_return_address(0));
}

int __asan_option_detect_stack_use_after_return = 0;

void *__asan_stack_malloc_0(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_1(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_2(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_3(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_4(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_5(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_6(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_7(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_8(size_t size) {
    cc_ignore(size);
    return NULL;
}
void *__asan_stack_malloc_9(size_t size) {
    cc_ignore(size);
    return NULL;
}

void __asan_stack_free_0(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_1(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_2(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_3(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_4(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_5(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_6(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_7(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_8(void *p, size_t size) {
    cc_ignore(p, size);
}
void __asan_stack_free_9(void *p, size_t size) {
    cc_ignore(p, size);
}

void *__asan_malloc(size_t size) {
    /* Not relevant for kernel: just panic if somehow called */
    kasan_panic("__asan_malloc called", NULL, size);
}

void __asan_free(void *p) {
    kasan_panic("__asan_free called", p, 0);
}

void __asan_malloc_hook(void *ptr, size_t size) {
    cc_ignore(ptr, size);
}
void __asan_free_hook(void *ptr) {
    cc_ignore(ptr);
}

void __asan_init(void) {
    asan_info("__asan_init runtime stub called");
}

void __asan_before_dynamic_init(const char *module_name) {
    cc_ignore(module_name);
}
void __asan_after_dynamic_init(void) {}

/* Compiler sometimes emits these for non-instrumented copies. */
void *__asan_memcpy(void *dst, const void *src, size_t n) {
    return memcpy(dst, src, n);
}
void *__asan_memmove(void *dst, const void *src, size_t n) {
    return memmove(dst, src, n);
}
void *__asan_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

/* Some compilers also expect these “weak” entrypoints */
void __asan_handle_no_return(void) {}
void __asan_after_load(void) {}
void __asan_after_store(void) {}
void __asan_before_memory_access(void) {}
void __asan_after_memory_access(void) {}

/* Optional runtime interface for global poisoning/unpoisoning */
void __asan_set_shadow_00_to_0x00(void) {}
void __asan_set_shadow_f8_to_0x00(void) {}

void __asan_alloca_poison(void *addr, size_t size) {
    ASAN_ABORT_IF_NOT_READY();
    if (!addr || size == 0)
        return;
    __asan_poison_memory_region(addr, size);
}

void __asan_allocas_unpoison(void *addr, size_t size) {
    ASAN_ABORT_IF_NOT_READY();
    if (!addr || size == 0)
        return;
    __asan_unpoison_memory_region(addr, size);
}

cc_weak void __asan_alloca_poison_0(void *addr, size_t size) {
    __asan_alloca_poison(addr, size);
}

cc_weak void __asan_allocas_unpoison_0(void *addr, size_t size) {
    __asan_allocas_unpoison(addr, size);
}

#define ASAN_ALIAS(name, target) cc_alias(target) void name

/* Outline callbacks (forced via -asan-instrumentation-with-call-threshold=0).
 */
ASAN_ALIAS(__asan_load1_noabort, __asan_load1)(const void *addr);
ASAN_ALIAS(__asan_load2_noabort, __asan_load2)(const void *addr);
ASAN_ALIAS(__asan_load4_noabort, __asan_load4)(const void *addr);
ASAN_ALIAS(__asan_load8_noabort, __asan_load8)(const void *addr);
ASAN_ALIAS(__asan_load16_noabort, __asan_load16)(const void *addr);
ASAN_ALIAS(__asan_store1_noabort, __asan_store1)(const void *addr);
ASAN_ALIAS(__asan_store2_noabort, __asan_store2)(const void *addr);
ASAN_ALIAS(__asan_store4_noabort, __asan_store4)(const void *addr);
ASAN_ALIAS(__asan_store8_noabort, __asan_store8)(const void *addr);
ASAN_ALIAS(__asan_store16_noabort, __asan_store16)(const void *addr);
ASAN_ALIAS(__asan_loadN_noabort, __asan_loadN)(const void *addr, size_t size);
ASAN_ALIAS(__asan_storeN_noabort, __asan_storeN)(const void *addr, size_t size);

ASAN_ALIAS(__asan_report_load1_noabort, __asan_report_load1)(void *addr);
ASAN_ALIAS(__asan_report_load2_noabort, __asan_report_load2)(void *addr);
ASAN_ALIAS(__asan_report_load4_noabort, __asan_report_load4)(void *addr);
ASAN_ALIAS(__asan_report_load8_noabort, __asan_report_load8)(void *addr);
ASAN_ALIAS(__asan_report_load16_noabort, __asan_report_load16)(void *addr);
ASAN_ALIAS(__asan_report_store1_noabort, __asan_report_store1)(void *addr);
ASAN_ALIAS(__asan_report_store2_noabort, __asan_report_store2)(void *addr);
ASAN_ALIAS(__asan_report_store4_noabort, __asan_report_store4)(void *addr);
ASAN_ALIAS(__asan_report_store8_noabort, __asan_report_store8)(void *addr);
ASAN_ALIAS(__asan_report_store16_noabort, __asan_report_store16)(void *addr);
ASAN_ALIAS(__asan_report_load_n_noabort, __asan_report_load_n)(void *addr,
                                                               size_t size);
ASAN_ALIAS(__asan_report_store_n_noabort, __asan_report_store_n)(void *addr,
                                                                 size_t size);
#endif
