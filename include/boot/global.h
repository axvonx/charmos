/* @title: Boot Globals */
#pragma once
#include <bootstage.h>

struct boot_globals {
    volatile enum bootstage stage;
};

extern struct boot_globals boot_global;
