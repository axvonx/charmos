/* @title: Events */
#pragma once
#include <atomic.h>
#include <thread/wait.h>
#include <time/time.h>
#include <tsa.h>

/* This event model is somewhat similar to KEVENT in Windows. Notification
 * events satisfy every single wait and remain "latched" until they get
 * explicitly cleared. Synchronization events (auto resets) satisfy
 * one wait and clear themselves.
 *
 * If waiters exist, the signal is handed directly and the event doesn't
 * get perceived as signalled.
 *
 * Event may be freed ONLY on event_wait returns
 */
enum event_type : uint8_t {
    EVENT_NOTIFICATION,
    EVENT_SYNCHRONIZATION,
};

struct event {
    struct thread_wait_header header;
    enum event_type type;

    /* Set under header->lock, but clears can be lockless */
    atomic_bool signaled;
};

#define EVENT_INIT(name_, type_, signaled_)                                    \
    {.header = THREAD_WAIT_HEADER_INIT((name_).header),                        \
     .type = (type_),                                                          \
     .signaled = (signaled_)}

void event_init(struct event *e, enum event_type type, bool signaled);

/* Return previous state */
bool event_set(struct event *e);
bool event_set_full(struct event *e, enum thread_resume_reason reason,
                    void (*callback)(struct thread *));
bool event_reset(struct event *e);
void event_clear(struct event *e);

bool event_read_state(struct event *e);

/* This function does not block. It consumes signal for a
 * synchronization event on success */
bool event_try_wait(struct event *e);

/* thread_wait_prepare() with several objects */
struct thread_wait_object event_wait_object(struct event *e);

struct thread_wait_result
event_wait(struct event *e, enum thread_wait_type type,
           enum thread_block_reason reason) TSA_MAY_SLEEP;

struct thread_wait_result
event_wait_timeout(struct event *e, enum thread_wait_type type,
                   enum thread_block_reason reason,
                   time_ms_t timeout_ms) TSA_MAY_SLEEP;
