/* @title: Per-CPU dynamic objects */
#pragma once
#include <compiler.h>
#include <global.h>
#include <linker/symbols.h>
#include <smp/core.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

struct percpu_descriptor;
typedef void (*percpu_descriptor_constructor)(void *, size_t);

struct percpu_descriptor {
    const char *name;
    size_t size;
    size_t align;
    void **percpu_ptrs;
    percpu_descriptor_constructor constructor;
    atomic_bool ready;
};

LINKER_SECTION_DEFINE(struct percpu_descriptor, percpu_desc);

#define PERCPU_DECLARE(__n, __type, __ctor)                                    \
    static typeof(__type) __percpu_##__n cc_unused;                            \
    static struct percpu_descriptor __percpu_desc_##__n;                       \
    static void __percpu_ctor_##__n(void *inst, size_t cpu) {                  \
        void (*const __typed_ctor)(typeof(__type) *, size_t) = (__ctor);       \
        if (__typed_ctor != NULL)                                              \
            __typed_ctor((typeof(__type) *) inst, cpu);                        \
        if (cpu == global.core_count - 1)                                      \
            atomic_store(&__percpu_desc_##__n.ready, true);                    \
    }                                                                          \
    static LINKER_SECTION_OBJECT(struct percpu_descriptor, percpu_desc)        \
        __percpu_desc_##__n = {                                                \
            .name = #__n,                                                      \
            .size = sizeof(typeof(__type)),                                    \
            .align = _Alignof(typeof(__type)),                                 \
            .percpu_ptrs = NULL,                                               \
            .constructor = __percpu_ctor_##__n,                                \
            .ready = false,                                                    \
    };                                                                         \
    static struct percpu_descriptor *const __percpu_desc_ref_##__n cc_unused = \
        &__percpu_desc_##__n

#define PERCPU_EXPORT_AS(sym_name, name)                                       \
    extern struct percpu_descriptor __percpu_desc_sym_##sym_name cc_alias(     \
        __percpu_desc_##name) cc_used

#define PERCPU_EXPORT(name) PERCPU_EXPORT_AS(name, name)

#define PERCPU_DEFINE_AS(name, sym_name, type)                                 \
    extern struct percpu_descriptor __percpu_desc_sym_##sym_name;              \
    static typeof(type) __percpu_##name cc_unused;                             \
    static struct percpu_descriptor *const __percpu_desc_ref_##name            \
        cc_unused = &__percpu_desc_sym_##sym_name

#define PERCPU_DEFINE(name, type) PERCPU_DEFINE_AS(name, name, type)

#define PERCPU(name) &(__percpu_##name)
#define PERCPU_READY(name) (atomic_load(&(__percpu_desc_ref_##name)->ready))

#define PERCPU_PTR_FOR_CPU(name, cpu)                                          \
    ({                                                                         \
        (void) kassert(PERCPU_READY(name));                                    \
        ((typeof(__percpu_##name) *) (__percpu_desc_ref_##name)                \
             ->percpu_ptrs[cpu]);                                              \
    })

#define PERCPU_READ_FOR_CPU(name, cpu)                                         \
    (*((typeof(__percpu_##name) *) PERCPU_PTR_FOR_CPU(name, cpu)))

#define PERCPU_PTR(clr, name) PERCPU_PTR_FOR_CPU(name, smp_id(clr))
#define PERCPU_READ(clr, name)                                                 \
    (*((typeof(__percpu_##name) *) PERCPU_PTR(clr, name)))

#define PERCPU_WRITE(clr, name, val) (PERCPU_READ(clr, name) = (val))

#define percpu_for_each_internal(name, var, cpu)                               \
    for (cpu_id_t cpu = 0; cpu < global.core_count; cpu++)                     \
        for (var = PERCPU_PTR_FOR_CPU(name, cpu); var != NULL; var = NULL)

#define percpu_for_each_internal_3(name, var, cpu)                             \
    percpu_for_each_internal(name, var, cpu)
#define percpu_for_each_internal_2(name, var)                                  \
    percpu_for_each_internal(name, var, __cpu)

#define percpu_for_each(...) PP_CALL(percpu_for_each_internal, __VA_ARGS__)

void percpu_obj_init(void);
