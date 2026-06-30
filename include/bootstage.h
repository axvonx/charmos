/* @title: Bootstages */
#include <stdint.h>
#pragma once

enum bootstage : uint8_t {
    BOOTSTAGE_NONE,
    BOOTSTAGE_EARLY_FB, /* Console can be printed to */

    BOOTSTAGE_EARLY_ALLOCATORS, /* Early non-topology aware
                                 * allocators available */

    BOOTSTAGE_EARLY_DEVICES, /* Early devices (ACPI, LAPIC, HPET) brought up */

    BOOTSTAGE_MID_MP, /* APs exit busy-spin and enter idle thread */

    BOOTSTAGE_MID_TOPOLOGY, /* Topology parsed */

    BOOTSTAGE_MID_ALLOCATORS, /* Allocators are topology aware */

    BOOTSTAGE_LATE, /* Rest of kernel is brought up -- filesystems,
                     * drivers, etc. almost all
                     * features are available in APIs */

    BOOTSTAGE_COMPLETE, /* Complete - enter init */

    BOOTSTAGE_COUNT,
};

extern const char *bootstage_str[BOOTSTAGE_COUNT];

/* This is a bit goofy, but we use it when global.bootstage can't be used,
 * often due to header file recursion soup and other happenings */
enum bootstage bootstage_get();
void bootstage_advance(enum bootstage new);
