/* @title: Memory Arena Provider API */
#include <math/bit.h>
#include <mem/arena.h>
#include <structures/bitmap.h>

struct log_site;

/* All indices are uint16_t */
#define ARENA_SEG_MAX_SIZE UINT16_MAX
#define ARENA_MAX_SEG 64
#define ARENA_MAX_EXT_FN 64

/* These arena segments are the indices for segments[] in struct arena, so that
 * various subsystems can extend the structure and get extra storage space.
 *
 * Note that these names are NOT the *only* uses of their respective
 * indices. Index 0 can very well be whatever the arena provider wants,
 * however, this enum is more of providing "norms" for using the segments
 */
enum arena_seg_type : uint16_t {
    ARENA_SEG_BASE = 0,
    ARENA_SEG_EXT = 1,
    ARENA_SEG_DEBUG = 2,
    ARENA_SEG_MAX, /* Maximum for the default segment types */
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

/* Notes on prefixes, since this op table gets busy
 *
 * qry_* is the per-object/ptr 'getter', set_* for the 'setters', and
 * get_* is more of an 'extract this from the structure, generally"
 */
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

    enum err (*set_tag)(struct arena *a, void *p, arena_tag_t tag);

    size_t (*qry_size)(const struct arena *a, void *p);

    arena_tag_t (*qry_tag)(const struct arena *a, void *p);

    stack_handle_t (*qry_trace)(const struct arena *a, void *p);

    arena_ext_fn_t ext_fns[ARENA_MAX_EXT_FN];
};

/* This is a *type* of segment, not necessarily at index 0 */
struct arena_seg_base {
    const char *name;
    enum arena_flags flags;
};

/* The generic version */
struct arena_seg {
    cc_align_as(max_align_t) uint8_t storage[];
};

/* Describes segments. Used in places like arena creation hooks.
 *
 * Since some operations expect 64 of these, let's try to cram
 * the data into a single dword to reduce memory usage */
struct arena_seg_desc {
    uint16_t present : 1; /* Is it here at all? */
    uint16_t idx : BITS_NEEDED(ARENA_MAX_SEG - 1);
    uint16_t type : BITS_NEEDED(ARENA_SEG_MAX - 1);
    uint16_t size;
};

/* inmem_desc is for what actually resides in payload[], and it's
 * a shrunken down, internal representation of segments that only
 * holds extra data when debugging for safety */
struct arena_seg_inmem_desc {
    uint16_t offset; /* Where is this segment, in the payload[]? */

#ifdef DEBUG_ARENA
    uint16_t type : BITS_NEEDED(ARENA_SEG_MAX - 1);
    uint16_t present : 1;
#endif
};

/* This is the parent structure of the operations and other
 * per-arena implementation data */
struct arena_desc {
    struct arena_strategy_capabilities caps;
    struct arena_ops *ops;
    struct arena_seg_desc seg_descs[ARENA_MAX_SEG];
};

struct arena_desc_linker_record {
    struct arena_desc *desc;
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

    /* ┌────────────────────────────────────────────────────────────┐
     * │                  struct arena's payload[]                  │
     * └────────────────────────────────────────────────────────────┘
     *
     *  │                                                          │
     *  │                                                          │
     *  │                                                          │
     *  │                                                          │
     *  ▼                                                          ▼
     *
     * ┌──────────────┐ ┌──────────────┐         ┌──────────────────┐
     * │ inmem_desc 0 │ │ inmem_desc 1 │ ●  ●  ● │descriptor storage│
     * └──────────────┘ └──────────────┘         └──────────────────┘
     *        ││
     *        │└────────────────┬─────────────────┐
     *        │                 │                 │
     *        ▼                 ▼                 ▼
     * ┌──────────────┐ ┌──────────────┐ ┌─────────────────┐
     * │ data offset  │ │     type     │ │ other tracking  │
     * │ in payload[] │ │  (optional)  │ │    metadata     │
     * └──────────────┘ └──────────────┘ └─────────────────┘
     */
    cc_align_as(max_align_t) uint8_t payload;
};

/* Arena strategies call into this with their fully formed descriptors */
struct arena *arena_create_full(struct arena_seg_desc seg_descs[ARENA_MAX_SEG]);
struct arena_seg *arena_seg_ptr(struct arena *a, uint16_t seg_idx);
