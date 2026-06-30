/* @title: Bootstage Runtime Patched Conditionals */
#pragma once
#include <bootstage.h>
#include <compiler.h>
#include <linker/symbols.h>
#include <stdbool.h>

enum bootstage_condition {
    BOOTSTAGE_CONDITION_EQ,
    BOOTSTAGE_CONDITION_LE,
    BOOTSTAGE_CONDITION_LT,
    BOOTSTAGE_CONDITION_GE,
    BOOTSTAGE_CONDITION_GT,
};

struct bootstage_condition_entry {
    void *code;
    void *target;
    enum bootstage_condition cond;
    enum bootstage stage;
};

/* Essentially, the syntax we strive to achieve is
 *
 * BOOTSTAGE_IF_LE(cond) {} , such as
 * BOOTSTAGE_IF_LT(BOOTSTAGE_LATE) {
 *    blah blah blah;
 * }
 *
 */

/* WARN: stage == BOOTSTAGE_EARLY_FB DOES NOT WORK, because at FB we don't
 * have the kernel mapping in our control, and we can't write to it then
 *
 * NOTE: in theory, we could walk the early page tables and change perms,
 * but this is tricky, and unless I have a super-hot-path that checks
 * EARLY_FB, I don't see this as super useful */
#define BOOTSTAGE_COND(cond, stage)                                            \
    ({                                                                         \
        __label__ l_yes, l_done;                                               \
        bool _taken = false;                                                   \
        asm goto("1:\n\t"                                                      \
                 ".byte 0xe9\n\t"                                              \
                 ".long %l[l_yes] - (1b + 5)\n\t"                              \
                 ".pushsection .kernel_bootstage_condition_entries,\"aw\"\n\t" \
                 ".balign 8\n\t"                                               \
                 ".quad 1b\n\t"                                                \
                 ".quad %l[l_yes]\n\t"                                         \
                 ".long %c[op]\n\t"                                            \
                 ".long %c[stg]\n\t"                                           \
                 ".popsection\n\t"                                             \
                 :                                                             \
                 : [op] "i"(cond), [stg] "i"(stage)                            \
                 :                                                             \
                 : l_yes);                                                     \
        goto l_done;                                                           \
    l_yes:                                                                     \
        _taken = true;                                                         \
    l_done:                                                                    \
        _taken;                                                                \
    })

#define BOOTSTAGE_IF_EQ(stage)                                                 \
    if (BOOTSTAGE_COND(BOOTSTAGE_CONDITION_EQ, stage))
#define BOOTSTAGE_IF_LE(stage)                                                 \
    if (BOOTSTAGE_COND(BOOTSTAGE_CONDITION_LE, stage))
#define BOOTSTAGE_IF_LT(stage)                                                 \
    if (BOOTSTAGE_COND(BOOTSTAGE_CONDITION_LT, stage))
#define BOOTSTAGE_IF_GE(stage)                                                 \
    if (BOOTSTAGE_COND(BOOTSTAGE_CONDITION_GE, stage))
#define BOOTSTAGE_IF_GT(stage)                                                 \
    if (BOOTSTAGE_COND(BOOTSTAGE_CONDITION_GT, stage))

LINKER_SECTION_DEFINE(struct bootstage_condition_entry,
                      bootstage_condition_entries);
