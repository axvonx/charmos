/* @title: Per-NUMA Node dynamic objects */
#pragma once
#include <compiler.h>
#include <linker/symbols.h>
#include <mem/numa.h>
#include <stddef.h>
#include <stdint.h>

struct pernode_descriptor;
typedef void (*pernode_descriptor_constructor)(void *, size_t);

struct pernode_descriptor {
    const char *name;
    size_t size;
    size_t align;
    void **pernode_ptrs;
    pernode_descriptor_constructor constructor;
} __linker_aligned;

LINKER_SECTION_DEFINE(struct pernode_descriptor, pernode_desc);

#define PERNODE_DECLARE(__n, __type, __ctor)                                   \
    extern __type __pernode_##__n;                                             \
    static void __pernode_ctor_##__n(void *inst, size_t cpu) {                 \
        if ((__ctor) != NULL)                                                  \
            __ctor((__type *) inst, cpu);                                      \
    }                                                                          \
    LINKER_SECTION_OBJECT(struct pernode_descriptor, pernode_desc)             \
    __pernode_desc_##__n = {                                                   \
        .name = #__n,                                                          \
        .size = sizeof(__type),                                                \
        .align = _Alignof(__type),                                             \
        .pernode_ptrs = NULL,                                                  \
        .constructor = __pernode_ctor_##__n,                                   \
    };                                                                         \
    __type __pernode_##__n

void pernode_obj_init(void);

#define PERNODE_PTR_FOR_NODE(name, d)                                          \
    ((typeof(__pernode_##name) *) __pernode_desc_##name.pernode_ptrs[d])
#define PERNODE_READ_FOR_NODE(name, d)                                         \
    (*((typeof(__pernode_##name) *) PERNODE_PTR_FOR_NODE(name, d)))

#define PERNODE_PTR(name) PERNODE_PTR_FOR_NODE(name, node_local_id())
#define PERNODE_READ(name) (*((typeof(__pernode_##name) *) PERNODE_PTR(name)))

#define PERNODE_WRITE(name, val) (PERNODE_READ(name) = (val))
