/* @title: Slab allocator */
#pragma once
#include <compiler.h>
#include <linker/symbols.h>
#include <stdbool.h>
#include <stddef.h>
#include <structures/list.h>

struct slab_elcm_candidate {
    size_t pages;
    size_t bitmap_size_bytes;
    size_t obj_count;
};

/* provides the ability for different subsystems to be able to make a constant
 * slab size so frequently allocated objects can waste a little less memory */
struct slab_size_constant {
    const char *name;
    size_t size;
    size_t align;
    struct list_head list;
    struct {
        struct slab_elcm_candidate cand;
    } internal;
} __linker_aligned;

#define SLAB_SIZE_REGISTER(n, s, a)                                            \
    static struct slab_size_constant slab_size_constant_##n                    \
        __attribute__((section(".kernel_slab_sizes"), used)) = {               \
            .name = #n,                                                        \
            .size = s,                                                         \
            .align = a,                                                        \
            .list = LIST_HEAD_INIT(slab_size_constant_##n.list),               \
            .internal = {{0}},                                                 \
    }

/* convenience wrapper */
#define SLAB_SIZE_REGISTER_FOR_STRUCT(sname, al)                               \
    SLAB_SIZE_REGISTER(sname, sizeof(struct sname), al)

#define SLAB_OBJ_ALIGN_DEFAULT 8u

void slab_allocator_init();
void slab_domain_init(void);
void slab_domains_print();
void slab_domain_init_late();

LINKER_SECTION_DEFINE(slab_sizes, struct slab_size_constant);
