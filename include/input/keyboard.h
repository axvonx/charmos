#pragma once
#include <types/types.h>

struct generic_keyboard;

typedef void (*kbd_emit_fn)(struct generic_keyboard *, uint32_t keycode,
                            bool pressed);

struct generic_keyboard {
    void *priv;

    kbd_emit_fn emit;
    uint8_t modifiers;
};
