/* @title: Memory Arena Provider API */
#include <math/bit.h>
#include <mem/arena.h>
#include <structures/bitmap.h>

#define ARENA_MAX_SEG 64
#define ARENA_MAX_EXT_FN 64
/* These arena segments are the indices for segments[] in struct arena, so that
 * various subsystems can extend the structure and get extra storage space.
 *
 * Note that these names are NOT the *only* uses of their respective
 * indices. Index 0 can very well be whatever the arena provider wants,
 * however, this enum is more of providing "norms" for using the segments
 */
enum arena_seg_offset {
    ARENA_SEG_BASE = 0,
    ARENA_SEG_EXT = 1,
    ARENA_SEG_DEBUG = 2,
    /* 3+ are available for use by various arenas */
};

/* NOTE: certain capabilities do not necessarily need to be implemented
 * by the arena. For instance, if a zeroed allocation is requested but the
 * arena has no zero allocation support, then we simply memset before
 * giving the memory back */
struct arena_strategy_capabilities {
    struct alloc_capabilities alloc_caps;
    enum arena_flags supported_flags;
};

struct arena_ops {
    void *(*alloc)(struct arena *a, size_t size, arena_tag_t tag,
                   struct alloc_params params);

    void *(*realloc)(struct arena *a, void *ptr, size_t size, arena_tag_t tag,
                     struct alloc_params params);

    /* Just in case the arena wants to have extra functionality
     * on allocation. Not an ext_fn since it's got so many arguments */
    void *(*alloc_special)(struct arena *a, struct arena_params *p,
                           arena_tag_t tag, struct alloc_params params);

    /* If an arena cannot implement a free operation without other
     * information, it should be using an extended function */
    enum err (*free)(struct arena *a, void *p, enum alloc_behavior bh);

    arena_ext_fn_t ext_fns[ARENA_MAX_EXT_FN];
};

struct arena_desc {
    struct arena_strategy_capabilities caps;
    struct arena_ops *ops;
};

struct arena_desc_linker_record {
    struct arena_desc *desc;
};

struct arena_seg_base {
    const char *name;
    enum arena_flags flags;
};

struct arena_seg {
    cc_align_as(max_align_t) uint8_t storage[];
};

/* We keep arenas intentionally minimal: some arenas
 * that might just be wrappers around things have no reason to
 * take up any more than a qword. We do keep a *desc instead of, say,
 * an enum arena_policy because descriptors can be ad-hoc created */
struct arena {
    struct arena_desc *desc;

    /* For segment presence, bitmap */
#ifdef DEBUG_ARENA
    BITMAP_DECLARE(seg_map, ARENA_MAX_SEG);
#endif

    /*
     * This trailing array contains:
     *
     * 1. uint16_t offsets[segment_count]
     * 2. raw bytes for all segments, packed
     */
    cc_align_as(max_align_t) uint8_t payload;
};
