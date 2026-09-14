/* @title: General Locking */
#pragma once

/* TODO: Actually start using this
 *
 * The reason we have this is because a lot of code around the kernel
 * is a big fan of using a function parameter to indicate the held
 * state of a lock. The problem with this is that it results in a bunch
 * of odd booleans with ad-hoc parameter name comments to clarify what
 * they are. Thus, we're using this enum to represent such cases
 * to assign a name and also improve searchability of such code */
enum lock_held_state {
    LOCK_NOT_HELD = 0,
    LOCK_HELD = 1, /* Mapped 0 and 1 so code can simply go `if (held)` */
};
