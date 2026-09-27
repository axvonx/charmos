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
#define cc_wno_override_init_expr(type, initializer)                           \
    ({                                                                         \
        cc_wno_override_init_start type cc_wno_override_init_value =           \
            initializer;                                                       \
        cc_wno_override_init_end cc_wno_override_init_value;                   \
    })
#endif

#if !defined(cc_wno_override_init_start)
#define cc_wno_override_init_start
#define cc_wno_override_init_end
#define cc_wno_override_init_expr(type, initializer) initializer
#endif
