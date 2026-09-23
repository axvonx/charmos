/* @title: Compiler Diagnostics */
#pragma once

#if defined(__clang__)
#define cc_wno_override_init_start                                             \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Winitializer-overrides\"")
#define cc_wno_override_init_end _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define cc_wno_override_init_start                                             \
    _Pragma("GCC diagnostic push")                                             \
        _Pragma("GCC diagnostic ignored \"-Woverride-init\"")
#define cc_wno_override_init_end _Pragma("GCC diagnostic pop")
#else
#define cc_wno_override_init_start
#define cc_wno_override_init_end
#endif
