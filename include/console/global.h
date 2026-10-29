/* @title: Crash Globals */
#pragma once
#include <atomic.h>

struct crash_globals {
    atomic_bool panicked;
};

extern struct crash_globals crash_global;
