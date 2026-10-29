/* @title: Condition Variable Types */
#pragma once
#include <thread/wait_types.h>

struct condvar {
    struct thread_wait_header waiters;
};
