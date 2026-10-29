/* @title: Thread reaper */
#pragma once
#include <structures/locked_list.h>
#include <sync/semaphore.h>
#include <thread/thread.h>

struct reaper_thread {
    struct locked_list list;
    struct semaphore sem;
    struct thread *thread;
};

void reaper_enqueue(struct thread *t);
void reaper_thread_main(void *nothing);
void reaper_init(void);
uint64_t reaper_get_reaped_thread_count(void);
