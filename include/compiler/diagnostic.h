/* @title: Compiler Diagnostics */
#pragma once
#include <compiler/name.h>

#if defined(cn_id_clang)
#define cc_wno_override_init_start                                             \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Winitializer-overrides\"")
#define cc_wno_override_init_end _Pragma("clang diagnostic pop")
#define cc_wno_override_init_expr(type, initializer)                           \
    cc_wno_override_init_start initializer cc_wno_override_init_end
#endif

#if defined(cn_id_gcc)
#define cc_wno_override_init_start                                             \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic ignored \"-Woverride-init\"")                  \
            _Pragma("GCC diagnostic ignored \"-Woverride-init-side-effects\"")
#define cc_wno_override_init_end _Pragma("GCC diagnostic pop")
#define cc_wno_override_init_cat_(a, b) a##b
#define cc_wno_override_init_cat(a, b) cc_wno_override_init_cat_(a, b)
#define cc_wno_override_init_expr(type, initializer)                           \
    cc_wno_override_init_expr_(                                                \
        type, initializer,                                                     \
        cc_wno_override_init_cat(cc_wno_override_init_value_, __COUNTER__))
#define cc_wno_override_init_expr_(type, initializer, val_)                    \
    ({                                                                         \
        cc_wno_override_init_start type val_ = initializer;                    \
        cc_wno_override_init_end val_;                                         \
    })
#endif

#if !defined(cc_wno_override_init_start)
#define cc_wno_override_init_start
#define cc_wno_override_init_end
#define cc_wno_override_init_expr(type, initializer) initializer
#endif
