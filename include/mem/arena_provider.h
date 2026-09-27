/* @title: Memory Arena Provider API */
#include <math/bit.h>
#include <mem/arena.h>
#include <structures/bitmap.h>
#include <structures/hlist.h>
#include <structures/mpmc_list.h>
#include <structures/rbt.h>
#include <sync/mutex.h>
#include <sync/rwlock.h>

/* TODO: Arena unregistration */

struct log_site;
struct arena_budget;
struct arena_dumpster;

/* For arena_fn_desc, which is the default set of
 * functions. This primarily serves to do a little
 * bit of debugging sanitization/warning, such as
 * identifying overloaded functions and properly
 * checking if the provided *p is stack allocated */
#define ARENA_MAX_FN 32

#define ARENA_SEG_MAX_SIZE 4096
#define ARENA_MAX_SEG 16 /* 4 bits are used to store segment IDs */
#define ARENA_MAX_EXT_FN 64
#define ARENA_SEG_CUSTOM(s) ((s) + ARENA_SEG_MAX)

#define ARENA_BIN_NOT_HEAD_MAGIC 0xB0D1ED100B0D1ED1 /* "BODIED!  BODIED!" */

/* These arena segments are the IDs for segments[] in struct arena, so that
 * various subsystems can extend the structure and get extra storage space.
 *
 * Note that these names are NOT the *only* uses of their respective
 * IDs. ID 0 can very well be whatever the arena provider wants,
 * however, this enum is more of providing "norms" for using the segments
 */
enum arena_seg_id : uint16_t {
    ARENA_SEG_PROPS = 0,
    ARENA_SEG_SYNC = 1,
    ARENA_SEG_LOG = 2,
    ARENA_SEG_TRACKING = 3,
    ARENA_SEG_BUDGET = 4,
    ARENA_SEG_DEBUG = 5,
    ARENA_SEG_MAX, /* Maximum for the default segment types */
};
static_assert(ARENA_SEG_MAX <= ARENA_MAX_SEG);

/* PARAMS indicates that *p is actually
 * meant to be a stack allocated set of parameters */
enum arena_fn_type { ARENA_FN_PTR, ARENA_FN_PARAMS };

enum arena_desc_flags {
    /* All arenas of this descriptor support garbage collection.
     * Destruction will enqueue it to GC */
    ARENA_DESC_GC = 1,
};

/* Notes on prefixes, since this op table gets busy
 *
 * qry_* is the per-object/ptr "getter", set_* for the "setters", and
 * get_* is more of an "extract this from the structure"
 *
 * on_* is a hook for when something happens
 *
 * As for arena_params, we only use this whenever no *p exists. In other cases,
 * *p is intended to be used as the arena allocator as an overloadable
 * field that can potentially point to a struct arena_params in some
 * implementations.
 */
struct arena_ops {
    struct arena *(*create)(struct arena_params *params);
    void *(*alloc)(struct arena *a, size_t size, struct alloc_params params);

    void *(*realloc)(struct arena *a, void *ptr, size_t size,
                     struct alloc_params params);

    /* Just in case the arena wants to have extra functionality
     * on allocation. Not an ext_fn since it's got so many arguments,
     * and exists at all because alloc does not take in a *p. */
    void *(*alloc_special)(struct arena *a, struct arena_params *params,
                           struct alloc_params a_params);

    /* If an arena cannot implement a free operation without other
     * information, it should be using an extended function */
    enum err (*free)(struct arena *a, void *p, enum alloc_behavior bh);

    /* Responsible for clearing out all memory allocations
     * inside of the arena. It does not necessarily reset its
     * internal structures */
    enum err (*clear)(struct arena *a, struct arena_params *params);

    /*
     * Does NOT ever free *a itself. This is the raw internal
     * free function, and should consider assertion that the
     * refcount == 0 if reference counting is being used
     */
    void (*destroy)(struct arena *a, struct arena_params *params);

    /* Handles actually resetting internal tracking data structures to
     * a point where it is effectively identical to creation time.
     *
     * Does NOT clear(), and can panic
     * if called without a cleared arena */
    void (*reset)(struct arena *a, struct arena_params *params);

    enum err (*set_tag)(struct arena *a, void *p, arena_tag_t tag);

    enum err (*set_budget)(struct arena *a, struct arena_budget b);

    size_t (*qry_size)(struct arena *a, void *p);

    arena_tag_t (*qry_tag)(struct arena *a, void *p);

    stack_handle_t (*qry_trace)(struct arena *a, void *p);

    bool (*qry_owns)(struct arena *a, void *p);

    /* *params is for the outside world */
    struct log_site *(*get_log_site)(struct arena *a,
                                     struct arena_params *params);

    struct arena_budget (*get_budget)(struct arena *a,
                                      struct arena_params *params);

    struct arena_dumpster *(*get_dumpster)(struct arena *a,
                                           struct arena_params *params);

    arena_identity_t (*get_identity)(struct arena *a,
                                     struct arena_params *params);

    bool (*identity_eq)(struct arena *a, struct arena *b);

    /* We do not have a on_gc_destruction. GC destruction
     * calls destroy(), and that is our hook */
    enum err (*on_gc_recycle)(struct arena *a);

    enum err (*on_gc_entry)(struct arena *a);

    /* These merely handle the internal reference counting, notably
     * NOT destroying or recycling arenas */
    bool (*get)(struct arena *a);

    void (*put)(struct arena *a);

    arena_ext_fn_t ext_fns[ARENA_MAX_EXT_FN];
};
static_assert((sizeof(struct arena_ops) -
               sizeof(arena_ext_fn_t) * ARENA_MAX_EXT_FN) <
              sizeof(arena_ext_fn_t) * ARENA_MAX_FN);

/* There's a 3-level hierarchy here:
 *
 * Bins store lockless singly linked lists of same-ksize arenas, and can be
 * created on demand for rapid arena reuse
 *
 * Bins must point to dumpsters, which are caches of various sized bins,
 * and control the "destroy" operation through a single detach operation
 *
 * Dumpsters can also be created on demand, and all dumpsters are tracked
 * with a landfill. For a given type of arena, there can only be a single
 * landfill globally, as its job is primarily profiling and tracking
 * dumpsters so that on, say, an OOM, dumpsters and bins can be reached.
 *
 *             ┌──────────────────────────────────────┐
 *             │           Arena Descriptor           │
 *             └──────────────────────────────────────┘
 *                                │
 *                                ▼
 *             ┌──────────────────────────────────────┐
 *             │       Arena Landfill (only 1)        │
 *             └──────────────────────────────────────┘
 *                       │                  │
 *                       ▼                  ▼
 *             ┌──────────────────┐┌──────────────────┐
 *             │ Arena Dumpster 1 ││ Arena Dumpster 2 │
 *             └──────────────────┘└──────────────────┘
 *                       │
 *                   ┌───┴───────────────────────┐
 *                   ▼                           ▼
 *             ┌──────────┐                ┌──────────┐
 *             │ Bucket 1 │   ●  ●  ●  ●   │ Bucket N │
 *             └──────────┘                └──────────┘
 *                   │
 *                   │
 *             ┌─────┘
 *             │  ┌───────────────┐   ┌───────────────┐
 *             └─▶│ Bin ID A Head │──▶│ Bin ID B Head │
 *                └───────────────┘   └───────────────┘
 *                        │
 *                        │
 *             ┌──────────┘
 *             │  ┌──────────────┐     ┌──────────────┐
 *             └─▶│  Bin ID A 1  │────▶│  Bin ID A 2  │
 *                └──────────────┘     └──────────────┘
 *                        │
 *                        │
 *             ┌──────────┘
 *             │  ┌─────────┐  ┌─────────┐  ┌─────────┐
 *             └─▶│ Arena 1 │─▶│ Arena 2 │─▶│ Arena 3 │
 *                └─────────┘  └─────────┘  └─────────┘
 */
struct arena_bin {
    /* Only one arena bin identity exists at the dumpster scope.
     * Subsequent bins attach their identities to the head */
    struct hlist_node bucket_node;

    union {
        struct hlist_head identical_bins;

        /* If this bin is not the head, this will contain some
         * ARENA_BIN_NOT_HEAD_MAGIC number.
         *
         * The reason we can't just say "if identical_bins == NULL"
         * is because a head that has no identical bins will
         * report that, whereas it could never contain ARENA_BIN_HEAD_MAGIC */
        uintptr_t bin_head_magic;
    };

    size_t arena_size;
    arena_identity_t identity;
    refcount_t refcount;

    struct mpmc_slist arenas;
    struct arena_dumpster *dumpster;
};

struct arena_bucket {
    struct hlist_head bins;
    struct spinlock lock;
};

struct arena_dumpster {
    struct arena_landfill *landfill;

    refcount_t refcount;
    size_t n_buckets;
    struct arena_bucket *buckets;
};

struct arena_landfill {
    struct list_head dumpster_list;
};

/* NOTE: certain capabilities do not necessarily need to be implemented
 * by the arena. For instance, if a zeroed allocation is requested but the
 * arena has no zero allocation support, then we simply memset before
 * giving the memory back */
struct arena_strategy_capabilities {
    struct alloc_capabilities alloc_caps;
    enum arena_flags supported_flags;
};

struct arena_fn_desc {
    void *fn;
    enum arena_fn_type type;
};

struct arena_budget {
    ALLOC_PRIORITY_BITMAP_DECLARE(allowed_prios);
    sz_b_t used_bytes;
    sz_b_t capacity_bytes;
    void *data;
};

struct arena_seg_log {
    struct log_site *site;
    struct log_handle handle;
};

struct arena_seg_sync {
    union {
        struct spinlock spinlock;
        struct mutex mutex;
        struct rwlock rwlock;
    };

    refcount_t refcount;
};

struct arena_seg_props {
    const char *name;
    enum arena_flags flags;
    size_t capacity;
};

struct arena_seg {
    cc_align_as(max_align_t) uint8_t storage[];
};

/* Describes segments. Used in places like arena creation hooks.
 *
 * Since some operations expect 64 of these, let's try to cram
 * the data into a single dword to reduce memory usage */
struct arena_seg_desc {
    uint16_t present : 1; /* Is it here at all? */
    uint16_t id : BITS_NEEDED(ARENA_MAX_SEG - 1);
    uint16_t type : BITS_NEEDED(ARENA_SEG_MAX - 1);
    uint16_t size;
};

/* inmem_desc is for what actually resides in payload[], and it's
 * a shrunken down, internal representation of segments that only
 * holds extra data when debugging
 *
 * NOTE: Segment *indices* are completely undefined.
 * What remains stable is the ID of the segment (0 <= id < 16).
 * Segment lookups key based off the ID,
 * and existence is only enforced
 * in debug builds to keep the arena payload[] slim
 */
struct arena_seg_inmem_desc {
    uint16_t id : 4;

    /* Where is this segment, in the payload[]? */
    uint16_t offset : BITS_NEEDED(ARENA_SEG_MAX_SIZE - 1);

#ifdef DEBUG_ARENA
    uint16_t type : BITS_NEEDED(ARENA_SEG_MAX - 1);
    uint16_t present : 1;
#endif
};

#ifndef DEBUG_ARENA
ct_assert_struct_size_eq(arena_seg_inmem_desc, 2);
#endif

/* This is the parent structure of the operations and other
 * per-arena implementation data */
struct arena_desc {
    struct list_head list_node;
    const char *name;
    const char *description;

    enum arena_strategy strategy;
    struct arena_strategy_capabilities caps;
    struct arena_ops *ops;
    struct arena_seg_desc seg_descs[ARENA_MAX_SEG];
    struct arena_fn_desc fn_descs[ARENA_MAX_FN];
    struct arena_dumpster *dumpster;
};

struct arena_desc_linker_record {
    struct arena_desc *desc;
};

/* We keep arenas minimal: some arenas
 * that might just be wrappers around things have no reason to
 * take up any more than a qword. We do keep a *desc instead of, say,
 * an enum arena_policy because descriptors can be ad-hoc created */
struct arena {
    union {
        struct arena_desc *desc;

        /* We reinterpret this qword as a list_node when the arena
         * is being recycled and resides inside a bin. The reason
         * this is safe is because the GC structures explicitly track
         * their arena_desc objects, so we can easily get the
         * *desc pointer back for this arena */
        struct mpmc_slist_node list_node;
    };

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
     * │  segment ID  │ │ data offset  │ │ other tracking  │
     * │    4 bits    │ │ in payload[] │ │    metadata     │
     * └──────────────┘ └──────────────┘ └─────────────────┘
     */
    cc_align_as(max_align_t) uint8_t payload;
};

/* A wrapper around e^(x - n) where n is scale, and x is used */
enum err arena_budget_prio_scale(sz_b_t used, int scale,
                                 ALLOC_PRIORITY_BITMAP_DECLARE(prio_map_out));

/* Arena strategies call into this with their fully formed descriptors */
struct arena *arena_create_full(struct arena_seg_desc seg_descs[ARENA_MAX_SEG]);
struct arena_seg *arena_seg_for(struct arena *a, uint16_t seg_id);
enum err arena_desc_register(struct arena_desc *d);
struct arena_desc *arena_desc_for(enum arena_strategy strat);
