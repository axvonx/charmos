#include <irq/irq.h>
#include <structures/cpu_mask_types.h>
#include <structures/list.h>

struct irq_action {
    irq_handler_t handler;
    void *data;
    struct list_node list;
};

struct irq_desc {
    uint8_t vector;
    enum irq_flags flags;

    const char *name;

    struct irq_chip *chip;
    void *chip_data;

    struct list_head actions;

    struct cpu_mask affinity;
    struct cpu_mask masked_cpus;

    bool present;   /* Have we set handlers? */
    bool allocated; /* Has this been allocated? */
    bool enabled;
};
