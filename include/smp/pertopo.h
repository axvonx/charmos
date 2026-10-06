/* @title: Per-Topology level dynamic objects */
#pragma once
#include <atomic.h>
#include <compiler/core.h>
#include <global.h>
#include <linker/symbols.h>
#include <smp/core.h>
#include <stddef.h>
#include <stdint.h>

struct pertopo_descriptor;
typedef void (*pertopo_descriptor_constructor)(void *, size_t);

struct pertopo_descriptor {
    enum topology_level level;
    const char *name;
    size_t size;
    size_t align;
    void **pertopo_ptrs;
    pertopo_descriptor_constructor constructor;
    atomic_bool ready;
};

LINKER_SECTION_EXTERN(struct pertopo_descriptor, pertopo_desc);

#define PERTOPO_DEFINE(__type, __n, _l, __ctor)                                \
    static typeof(__type) __pertopo_##__n cc_fn_unused;                        \
    static struct pertopo_descriptor __pertopo_desc_##__n;                     \
    static void __pertopo_ctor_##__n(void *inst, size_t node_id) {             \
        void (*const __typed_ctor)(typeof(__type) *, size_t) = (__ctor);       \
        if (__typed_ctor != NULL)                                              \
            __typed_ctor((typeof(__type) *) inst, node_id);                    \
        if (node_id == pertopo_node_count(&__pertopo_desc_##__n) - 1)          \
            atomic_store(&__pertopo_desc_##__n.ready, true);                   \
    }                                                                          \
    static LINKER_SECTION_OBJECT(struct pertopo_descriptor, pertopo_desc)      \
        __pertopo_desc_##__n = {                                               \
            .name = #__n,                                                      \
            .size = sizeof(typeof(__type)),                                    \
            .align = _Alignof(typeof(__type)),                                 \
            .pertopo_ptrs = NULL,                                              \
            .level = _l,                                                       \
            .constructor = __pertopo_ctor_##__n,                               \
            .ready = false,                                                    \
    };                                                                         \
    static struct pertopo_descriptor *const __pertopo_desc_ref_##__n           \
        cc_fn_unused = &__pertopo_desc_##__n

#define PERTOPO_EXPORT_AS(sym_name, name)                                      \
    extern struct pertopo_descriptor __pertopo_desc_sym_##sym_name             \
        cc_alias(__pertopo_desc_##name) cc_used

#define PERTOPO_EXPORT(name) PERTOPO_EXPORT_AS(name, name)

#define PERTOPO_EXTERN_AS(type, name, sym_name)                                \
    extern struct pertopo_descriptor __pertopo_desc_sym_##sym_name;            \
    static typeof(type) __pertopo_##name cc_fn_unused;                         \
    static struct pertopo_descriptor *const __pertopo_desc_ref_##name          \
        cc_fn_unused = &__pertopo_desc_sym_##sym_name

#define PERTOPO_EXTERN(type, name) PERTOPO_EXTERN_AS(type, name, name)

#define PERTOPO(name) &(__pertopo_##name)
#define PERTOPO_READY(name) (atomic_load(&(__pertopo_desc_ref_##name)->ready))

#define PERTOPO_PTR_FOR_TOPO_NODE(name, node)                                  \
    ({                                                                         \
        (void) kassert(PERTOPO_READY(name));                                   \
        ((typeof(__pertopo_##name) *) (__pertopo_desc_ref_##name)              \
             ->pertopo_ptrs[node]);                                            \
    })

#define PERTOPO_READ_FOR_TOPO_NODE(name, node)                                 \
    (*((typeof(__pertopo_##name) *) PERTOPO_PTR_FOR_TOPO_NODE(name, node)))

#define PERTOPO_PTR(clr, name)                                                 \
    PERTOPO_PTR_FOR_TOPO_NODE(name, pertopo_node(&__pertopo_desc_##name, clr))
#define PERTOPO_READ(clr, name)                                                \
    (*((typeof(__pertopo_##name) *) PERTOPO_PTR(clr, name)))

#define PERTOPO_WRITE(clr, name, val) (PERTOPO_READ(clr, name) = (val))

#define pertopo_for_each_internal(name, var, node_id)                          \
    for (size_t node_id = 0;                                                   \
         node_id < pertopo_node_count(&__pertopo_desc_##name); node_id++)      \
        for (var = PERTOPO_PTR_FOR_TOPO_NODE(name, node_id); var != NULL;      \
             var = NULL)

#define pertopo_for_each_internal_3(name, var, node_id)                        \
    pertopo_for_each_internal(name, var, node_id)
#define pertopo_for_each_internal_2(name, var)                                 \
    pertopo_for_each_internal_3(name, var, __node_id)

#define pertopo_for_each(...) PP_CALL(pertopo_for_each_internal, __VA_ARGS__)

void pertopo_obj_init(void);
