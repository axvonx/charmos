#pragma once
#include <structures/list.h>
#include <sync/spinlock.h>
#include <thread/thread_types.h>
#include <thread/wait_types.h>

#define THREAD_WAIT_BLOCK_SYNC 0
#define THREAD_WAIT_KEY_NONE UINT16_MAX

#define THREAD_WAIT_HEADER_INIT(name_)                                         \
    {.lock = SPINLOCK_INIT, .waiters = LIST_HEAD_INIT((name_).waiters)}

ct_assert_struct_size_eq(thread_wait_block, 48);

struct thread_wait_object {
    struct thread_wait_header *header;
    void *object;

    /* Optional function. This runs under header->lock right before
     * we link the wait block, and if it returns true,
     * the wait is satisfied at registration time and any
     * remaining objects never get linked */
    bool (*try_acquire)(void *object);
};

struct thread_wait_result {
    enum thread_wait_status status;
    uint16_t key;
    enum thread_resume_reason reason;
};

void thread_wait_header_init(struct thread_wait_header *header);
void thread_wait_init(struct thread *thread);

/* If storage == NULL, we use the thread's embedded blocks. No allocation
 * ever happens in here, however, the caller remains runnable */
void thread_wait_prepare(const struct thread_wait_object *objects, size_t count,
                         struct thread_wait_block *storage,
                         enum thread_wait_type type,
                         enum thread_block_reason reason);

void thread_wait_prepare_one(struct thread_wait_header *header, void *object,
                             enum thread_wait_type type,
                             enum thread_block_reason reason);

void thread_wait_prepare_to_sleep(struct thread_wait_header *header,
                                  void *object, enum thread_wait_type type);

/* Satisfy unlinks ALL blocks before the waiter becomes runnable.
 * The callback runs just for the winner */
bool thread_wait_satisfy(struct thread_wait_block *block,
                         enum thread_resume_reason reason,
                         void (*callback)(struct thread *));

/* For callbacks that took a snapshot of thread->wait_epoch */
bool thread_wait_satisfy_epoch(struct thread_wait_block *block, uint64_t epoch,
                               enum thread_resume_reason reason,
                               void (*cb)(struct thread *));

struct thread *thread_wait_header_satisfy(struct thread_wait_header *hdr,
                                          enum thread_resume_reason reason,
                                          void (*cb)(struct thread *));

/* Like thread_wait_header_satisfy, but if nothing remains
 * to satisfy, empty_locked runs under hdr->lock */
struct thread *thread_wait_header_satisfy_one(struct thread_wait_header *hdr,
                                              enum thread_resume_reason reason,
                                              void (*cb)(struct thread *),
                                              void (*empty_locked)(void *),
                                              void *arg);

/* Satisfies the waiters linked when before_locked runs, under hdr->lock */
size_t thread_wait_header_satisfy_all(struct thread_wait_header *hdr,
                                      enum thread_resume_reason reason,
                                      void (*callback)(struct thread *),
                                      void (*before_locked)(void *), void *arg);

/* Alert is a "level-triggered" singular event. We only end interruptible
 * waits with alerts, but otherwise, the alert will merely pend until
 * the next interruptible wait/park happens */
void thread_alert(struct thread *cc_nonnull_ptr thread);
void thread_park(void);

/* The actual "go wait" action, yield and whatnot */
struct thread_wait_result thread_wait_complete(void);

struct thread_wait_result thread_wait_complete_timeout(time_ms_t timeout_ms);
void thread_wait_cancel(void);
