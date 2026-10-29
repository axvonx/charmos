#include <console/crash.h>
#include <setjmp.h>
#include <structures/list.h>

struct crash_unwind_node {
    enum crash_unwind_type type;
    struct list_node list;
    struct crash_unwind_node_data data;
};

#define CRASH_UNWIND_NODES 128
struct crash_unwind_thread_data {
    struct list_head free_list;
    struct list_head in_use; /* LIFO */
    struct crash_unwind_node nodes[CRASH_UNWIND_NODES];
};

struct crash_thread_data {
    struct crash_unwind_thread_data unwind;
    struct list_head crash_hooks; /* struct crash_hook */
    bool unwinding;
    bool in_hook;
    jmp_buf env;
};

/*
 * The idea of crash hooks:
 *
 * A hook can hook into a specific code, which is checked
 * against a facility to verify if it is hookable.
 *
 * They are hooked PER-THREAD, so as to not introduce strange
 * non-determinism bugs and the chance that the crash hook
 * registration path itself can crash (would be problematic)
 * due to synchronization violations.
 *
 * The generic (NULL) code is not hookable
 */
struct crash_hook {
    char *name;
    struct list_node list;
    const struct crash_code *code;
    enum crash_source source_mask;
};
