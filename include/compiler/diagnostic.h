/* @title: Compiler Diagnostics */
#pragma once

#if defined(__clang__)
#define cc_wno_override_init_start                                             \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Winitializer-overrides\"")
#define cc_wno_override_init_end _Pragma("clang diagnostic pop")
#define cc_wno_override_init_expr(type, initializer)                           \
    cc_wno_override_init_start initializer cc_wno_override_init_end
#elif defined(__GNUC__)
#define cc_wno_override_init_start                                             \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic ignored \"-Woverride-init\"")
#define cc_wno_override_init_end _Pragma("GCC diagnostic pop")
#define cc_wno_override_init_expr(type, initializer)                           \
    ({                                                                         \
        cc_wno_override_init_start type cc_wno_override_init_value =           \
            initializer;                                                       \
        cc_wno_override_init_end cc_wno_override_init_value;                   \
    })
#else
#define cc_wno_override_init_start
#define cc_wno_override_init_end
#define cc_wno_override_init_expr(type, initializer) initializer
#endif
