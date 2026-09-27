/* @title: Compiler Identification */
#pragma once

/*
 * cn_* "Compiler Name", cn_id_* "Compiler Identity"
 */

#if defined(__has_builtin)
#define cn_has_builtin(x) __has_builtin(x)
#else
#define cn_has_builtin(x) 0
#endif

#if defined(__has_attribute)
#define cn_has_attribute(x) __has_attribute(x)
#else
#define cn_has_attribute(x) 0
#endif

#if defined(__has_feature)
#define cn_has_feature(x) __has_feature(x)
#else
#define cn_has_feature(x) 0
#endif

#if defined(__has_extension)
#define cn_has_extension(x) __has_extension(x)
#else
#define cn_has_extension(x) 0
#endif

#if defined(__has_warning)
#define cn_has_warning(x) __has_warning(x)
#else
#define cn_has_warning(x) 0
#endif

#if defined(__has_include)
#define cn_has_include(x) __has_include(x)
#else
#define cn_has_include(x) 0
#endif

/* ==== Compiler Identity (cn_id_) ==== */

#if defined(__clang__)
#define cn_id_clang
#define cn_id_clang_major __clang_major__
#define cn_id_clang_minor __clang_minor__
#define cn_id_clang_patch __clang_patchlevel__
#define cn_id_clang_version                                                    \
    ((__clang_major__ * 10000) + (__clang_minor__ * 100) + __clang_patchlevel__)
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define cn_id_gcc
#define cn_id_gcc_major __GNUC__
#define cn_id_gcc_minor __GNUC_MINOR__
#define cn_id_gcc_patch __GNUC_PATCHLEVEL__
#define cn_id_gcc_version                                                      \
    ((__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + __GNUC_PATCHLEVEL__)
#endif

#if defined(cn_id_clang)
#define cn_id_clang_at_least(maj, min)                                         \
    ((cn_id_clang_major > (maj)) ||                                            \
     (cn_id_clang_major == (maj) && cn_id_clang_minor >= (min)))
#else
#define cn_id_clang_at_least(maj, min) 0
#endif

#if defined(cn_id_gcc)
#define cn_id_gcc_at_least(maj, min)                                           \
    ((cn_id_gcc_major > (maj)) ||                                              \
     (cn_id_gcc_major == (maj) && cn_id_gcc_minor >= (min)))
#else
#define cn_id_gcc_at_least(maj, min) 0
#endif

#if defined(cn_id_clang)
#define cn_id_name "Clang"
#endif

#if defined(cn_id_gcc)
#define cn_id_name "GCC"
#endif

#if !defined(cn_id_name)
#define cn_id_name "Unknown"
#endif
