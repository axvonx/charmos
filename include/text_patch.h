/* @title: Kernel Text Patching */
#pragma once
#include <asm.h>
#include <math/bit.h>
#include <stdbool.h>
#include <stdint.h>

#define CR0_WP BIT(16)

struct text_patch_window {
    uint64_t cr0;
    bool interrupts;
};

static inline struct text_patch_window
text_patch_begin(void) TSA_ACQUIRES_IRQS {
    struct text_patch_window w = {
        .interrupts = irq_disable_save(),
    };
    w.cr0 = cr0_read();
    cr0_write(w.cr0 & ~CR0_WP);

    return w;
}

static inline void
text_patch_end(struct text_patch_window w) TSA_RELEASES_IRQS {
    cr0_write(w.cr0);
    irq_restore(w.interrupts);
}
