/* @title: Stringification signature macros */
#pragma once
#include <compiler/core.h>

#define STRINGIFY_DECLARE_2(fn_name, in_type)                                  \
    static inline const char *fn_name(in_type stringify_in)

#define STRINGIFY_DECLARE_1(type) STRINGIFY_DECLARE_2(type##_to_str, type)

#define STRINGIFY_DECLARE(...) PP_CALL(STRINGIFY_DECLARE, __VA_ARGS__)

#define STRINGIFY_DECLARE_ENUM(type)                                           \
    STRINGIFY_DECLARE_2(type##_to_str, enum type)
