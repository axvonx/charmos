/* @title: Writer to Reader Boosting Types */
#pragma once
#include <math/bit.h>
#include <structures/cpu_mask_types.h>
#include <structures/hlist.h>
#include <thread/thread_types.h>

#define WROOST_THREAD_COUNTERS 32

#define WROOST_FINE_ENTRY_COUNT 4

#define WROOST_THREAD_COUNTER_BITS (WROOST_THREAD_COUNTERS * 2)

#define WROOST_THREAD_COUNTER_BYTES (WROOST_THREAD_COUNTER_BITS / BITS_PER_BYTE)

typedef uint16_t wroost_id_t;

typedef uint8_t wroost_counter_t;

/* Writers are a simple state machine.
 *
 * NONE = nothing happening
 * WANT = "I want a lock"
 * WAIT = "I am waiting for a lock"
 * BUMP = "I have yielded and bumped counters"
 *
 * WANT and WAIT are set in the thread's context, and BUMP is set immediately
 * after the WAIT reaches the switch out function.
 *
 * We leave it at BUMP throughout a critical section,*/
enum wroost_writer_state : uint8_t {
    WROOST_WRITER_NONE,
    WROOST_WRITER_WANT,
    WROOST_WRITER_WAIT,
    WROOST_WRITER_BUMP,
};

struct wroost_kicks {
    bool need_kicks;
    struct cpu_mask cpus;
    wroost_id_t lock_id;
};

struct wroost_fine_entry {
    wroost_id_t lock_id; /* Shift this right by WROOST_FINE_INDEX_SHIFT
                          * to get the coarse index of the fine entry */
};

/*
 * There's a key invariant with fine_entries:
 *
 * ====================================================================
 * A counter is either FULLY represented in fine_entries, or completely
 * unrepresented within fine_entries.
 * ====================================================================
 *
 * The reason why:
 *
 * If 3/4 fine_entries are taken up, and a counter wants to allocate
 * two more, then it ends up with a split: one of its contributing
 * numbers in the counter is in the fine_entries, and one isn't.
 *
 * This means that the boost HAS to use the coarse entry, since
 * otherwise, we risk a false negative, which would be completely
 * against the premise of wroost.
 *
 * Thus, that extra fine_entry is wasted on a lock ID that doesn't
 * use it. So, this invariant keeps all fine entries useful.
 */
struct wroost_thread_data {
    struct hlist_node map_node;

    /* Reader */
    wroost_counter_t counters[WROOST_THREAD_COUNTER_BYTES];
    struct wroost_fine_entry fine_entries[WROOST_FINE_ENTRY_COUNT];
    uint8_t read_nesting_depth;

    /* Writer */
    wroost_id_t want_lock_id;

    enum wroost_writer_state writer_state;

    /* Boosts: applies to the readers */

    /* boost_class_ceil is the ceil that the writer->reader boosts contribute.
     * There is only one, and it is the maximum of any of the lock IDs it holds.
     *
     * max_boost serves a completely different role. Every rwlock is
     * instantiated with a fixed maximum on the highest thread priority class
     * that can enter the lock. max_boost is the maximum of all of these.
     * The idea is that if an URGENT thread blocks on a rwlock with a maximum
     * of RT, that code will panic.
     *
     * Thus, if we encounter a thread that seems
     * to hold an ID where the urgent counter is >= 1, if this max_boost
     * is at RT, we actually *don't* boost to urgent, and boost to what's
     * further down (not necessarily RT either). It's essentially a way
     * to cheaply verify that a big boost does NOT belong to us,
     * because if it did, the kernel would've crashed long before.
     */
    enum thread_prio_class max_boost : THREAD_PRIO_CLASS_BIT_COUNT;

    /* Whether this wroost_thread_data has
     * contributed to the thread's class_floors */
    bool contributed_floor : 1; /* Set by scheduler */
    bool in_blocked_ts_list : 1;

    /* TODO: use this to implement the optimization of knowing
     * the readers cannot be of lower priority */
    bool need_bump : 1;

    /* Tracks which of the 4 fine_entries are active so index == 0
     * does not get interpreted being a free slot  */
    uint8_t fine_valid : WROOST_FINE_ENTRY_COUNT;

    enum thread_prio_class floor : THREAD_PRIO_CLASS_BIT_COUNT;
};
