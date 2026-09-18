/* @title: Per-Domain dynamic objects */
#pragma once
#include <compiler/core.h>
#include <global.h>
#include <linker/symbols.h>
#include <smp/domain.h>
#include <stddef.h>
#include <stdint.h>

struct perdomain_descriptor;
typedef void (*perdomain_descriptor_constructor)(void *, size_t);

struct perdomain_descriptor {
    const char *name;
    size_t size;
    size_t align;
    void **perdomain_ptrs;
    perdomain_descriptor_constructor constructor;
    atomic_bool ready;
};

LINKER_SECTION_DEFINE(struct perdomain_descriptor, perdomain_desc);

#define PERDOMAIN_DECLARE(__n, __type, __ctor)                                 \
    static typeof(__type) __perdomain_##__n cc_unused;                         \
    static struct perdomain_descriptor __perdomain_desc_##__n;                 \
    static void __perdomain_ctor_##__n(void *inst, size_t domain) {            \
        void (*const __typed_ctor)(typeof(__type) *, size_t) = (__ctor);       \
        if (__typed_ctor != NULL)                                              \
            __typed_ctor((typeof(__type) *) inst, domain);                     \
        if (domain == global.domain_count - 1)                                 \
            atomic_store(&__perdomain_desc_##__n.ready, true);                 \
    }                                                                          \
    static LINKER_SECTION_OBJECT(struct perdomain_descriptor, perdomain_desc)  \
        __perdomain_desc_##__n = {                                             \
            .name = #__n,                                                      \
            .size = sizeof(typeof(__type)),                                    \
            .align = _Alignof(typeof(__type)),                                 \
            .perdomain_ptrs = NULL,                                            \
            .constructor = __perdomain_ctor_##__n,                             \
            .ready = false,                                                    \
    };                                                                         \
    static struct perdomain_descriptor *const __perdomain_desc_ref_##__n       \
        cc_unused = &__perdomain_desc_##__n

#define PERDOMAIN_EXPORT_AS(sym_name, name)                                    \
    extern struct perdomain_descriptor __perdomain_desc_sym_##sym_name         \
    cc_alias(__perdomain_desc_##name) cc_used

#define PERDOMAIN_EXPORT(name) PERDOMAIN_EXPORT_AS(name, name)

#define PERDOMAIN_DEFINE_AS(name, sym_name, type)                              \
    extern struct perdomain_descriptor __perdomain_desc_sym_##sym_name;        \
    static typeof(type) __perdomain_##name cc_unused;                          \
    static struct perdomain_descriptor *const __perdomain_desc_ref_##name      \
        cc_unused = &__perdomain_desc_sym_##sym_name

#define PERDOMAIN_DEFINE(name, type) PERDOMAIN_DEFINE_AS(name, name, type)

#define PERDOMAIN(name) &(__perdomain_##name)
#define PERDOMAIN_READY(name)                                                  \
    (atomic_load(&(__perdomain_desc_ref_##name)->ready))

#define PERDOMAIN_PTR_FOR_DOMAIN(name, d)                                      \
    ({                                                                         \
        (void) kassert(PERDOMAIN_READY(name));                                 \
        ((typeof(__perdomain_##name) *) (__perdomain_desc_ref_##name)          \
             ->perdomain_ptrs[d]);                                             \
    })

#define PERDOMAIN_READ_FOR_DOMAIN(name, d)                                     \
    (*((typeof(__perdomain_##name) *) PERDOMAIN_PTR_FOR_DOMAIN(name, d)))

#define PERDOMAIN_PTR(clr, name)                                               \
    PERDOMAIN_PTR_FOR_DOMAIN(name, domain_local_id(clr))
#define PERDOMAIN_READ(clr, name)                                              \
    (*((typeof(__perdomain_##name) *) PERDOMAIN_PTR(clr, name)))

#define PERDOMAIN_WRITE(clr, name, val) (PERDOMAIN_READ(clr, name) = (val))

#define perdomain_for_each_internal(name, var, domain)                         \
    for (domain_id_t domain = 0; domain < global.domain_count; domain++)       \
        for (var = PERDOMAIN_PTR_FOR_DOMAIN(name, domain); var != NULL;        \
             var = NULL)

#define perdomain_for_each_internal_3(name, var, domain)                       \
    perdomain_for_each_internal(name, var, domain)
#define perdomain_for_each_internal_2(name, var)                               \
    perdomain_for_each_internal(name, var, __domain)

#define perdomain_for_each(...)                                                \
    PP_CALL(perdomain_for_each_internal, __VA_ARGS__)

void perdomain_obj_init(void);
