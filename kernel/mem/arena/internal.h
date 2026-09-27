#include <mem/arena.h>
#include <sync/rcu.h>

struct arena_layout_desc {
    uint8_t total_header_size;
    int8_t offset_tag;
    int8_t offset_depot;
    int8_t offset_canary;
};

struct arena_desc_table {
    struct arena_desc **descs;
    size_t n_descs;
    struct rcu_cb rcu_cb;
};

struct arena_globals {
    struct arena_desc_table cc_mem_rcu *desc_table;
    struct spinlock tbl_lock;
};

extern struct arena_globals arena_global;
