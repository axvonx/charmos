/* @title: Thread Wait Types */
#pragma once
#include <structures/list.h>
#include <sync/spinlock_types.h>

#define THREAD_NUM_WAIT_BLOCKS 4

struct thread_wait_header {
    struct spinlock lock;
    struct list_head waiters;
};

enum thread_wait_block_state {
    THREAD_WAIT_BLOCK_INACTIVE,
    THREAD_WAIT_BLOCK_ACTIVE,
    THREAD_WAIT_BLOCK_SATISFIED,
};

struct thread_wait_block {
    /* TODO: we're using a doubly linked list here because the block list under
     * a thread can grow arbitrarily long. Perhaps someday we can consider
     * something like Windows' MAXIMUM_WAIT_OBJECTS, so we can go for
     * a singularly linked list, save 8 bytes, at the cost of an
     * absolute maximum + maybe minor slowdowns (?) */
    struct list_node obj_node;
    struct thread *thread;
    void *object;
    uint16_t key;
    enum thread_wait_block_state state;
    struct thread_wait_header *header;
};
