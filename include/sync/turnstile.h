#include <container_of.h>
#include <sch/irql.h>
#include <structures/list.h>
#include <structures/pairing_heap.h>
#include <structures/rbt.h>
#include <sync/spinlock_types.h>
#include <thread/thread_types.h>
#include <thread/wait_types.h>

#define TURNSTILE_WRITER_QUEUE 0
#define TURNSTILE_READER_QUEUE 1
#define TURNSTILE_NUM_QUEUES 2

struct wroost_percpu_data;
enum turnstile_state {
    TURNSTILE_STATE_UNUSED, /* we are unused by anything... */

    TURNSTILE_STATE_IN_HASH_TABLE, /* we are the turnstile
                                    * responsible for some lock
                                    * and we are in the hash table */

    TURNSTILE_STATE_IN_FREE_LIST, /* we are sitting on the freelist of
                                   * a turnstile sitting around in the
                                   * hash table of turnstiles */
};

struct turnstile {
    struct thread_wait_header wait;
    struct thread *owner;

    struct thread_class_donation pi;
    struct list_node hash_list;

    union {
        struct list_head freelist;
        struct list_node freelist_node;
    };
    size_t waiters; /* how many goobers are blocking on me? */
    void *lock_obj; /* lock we are for */
    struct rbt queues[TURNSTILE_NUM_QUEUES];
    enum turnstile_state state;

    uint64_t next_wait_seq;
    uint32_t handoffs; /* since the last fairness turn */
};

/* Every TURNSTILE_BYPASS_LIMIT-th handoff wakes the longest waiting
 * waiter instead of the highest wq_prio */
#define TURNSTILE_BYPASS_LIMIT 4

#define turnstile_from_freelist(fl)                                            \
    (container_of(fl, struct turnstile, freelist_node))
#define turnstile_from_hash_list_node(hln)                                     \
    (container_of(hln, struct turnstile, hash_list))

/* chains in hashtable */
struct turnstile_hash_chain {
    struct list_head list;
    struct spinlock lock;
};

#define TURNSTILE_HASH_SIZE 128
#define TURNSTILE_HASH_MASK (TURNSTILE_HASH_SIZE - 1)

#define TURNSTILE_OBJECT_HASH(obj)                                             \
    ((((uintptr_t) (obj) >> 3) * 2654435761u) & TURNSTILE_HASH_MASK)

struct turnstile_hash_table {
    struct turnstile_hash_chain heads[TURNSTILE_HASH_SIZE];
};

void turnstiles_init(void);
struct turnstile *turnstile_create(void);
void turnstile_destroy(struct turnstile *ts);
struct turnstile *turnstile_init(struct turnstile *ts);
struct turnstile *turnstile_block(struct turnstile *ts, size_t queue_num,
                                  void *lock_obj, enum irql lock_irql,
                                  struct thread *owner);
enum irql turnstile_lookup(void *obj, struct turnstile **out_ts);
void turnstile_unlock(void *obj, enum irql irql);
void turnstile_wake(struct turnstile *ts, size_t queue, size_t num_threads,
                    enum irql lock_irql);

/* Returns whether the caller pcd needs to re-check threads
 * because somewhere down the PI chain, someone wanted to
 * bother the caller pcd (also merges kicks into pcd) */
bool turnstile_propagate_wroost(struct wroost_percpu_data *pcd,
                                struct thread *t);
/* Hand off to queue head if `target` is NULL */
void turnstile_wake_handoff(struct turnstile *ts, size_t queue,
                            struct thread *target, enum irql lock_irql);

/* Called once per handoff with chain locked. NULL means we should
 * use the priority ordering of wq_prio */
struct thread *turnstile_fair_pick(struct turnstile *ts, size_t *queue_out);
size_t turnstile_get_waiter_count(void *lock_obj);
int32_t turnstile_thread_priority(struct thread *t);
