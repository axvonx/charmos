/* @title: Driver Verifier */
#pragma once
#include <compiler.h>

/* Comptime verification that structures matches spec
 *
 * Each macro here is a big wrapper around a `_Static_assert`,
 * so nothing here emits any code. TODO: introduce eventual
 * spec name and section definition support.
 *
 * These are used in layout.c of various subsystems, so as to
 * not pollute a header and avoid emitting one warning many times.
 *
 * NOTE: these macros CAN'T do a few things:
 * - bitfields
 * - reference spec
 * - give full names to members */
#define dv_layout_full(type, bytes, ...) ct_assert_size(type, bytes)

#define dv_align_full(type, bytes, ...) ct_assert_align(type, bytes)

#define dv_field_full(type, member, offset, bytes, ...)                        \
    _Static_assert(__builtin_offsetof(type, member) == (offset) &&             \
                       sizeof(((type *) 0)->member) == (bytes),                \
                   #type "." #member " is not at offset " #offset              \
                         " with size " #bytes)

#define dv_field_at_full(type, member, offset, ...)                            \
    ct_assert_offset(type, member, offset)

#define dv_layout(name, ...) dv_layout_full(struct name, __VA_ARGS__)
#define dv_align(name, ...) dv_align_full(struct name, __VA_ARGS__)
#define dv_field(name, ...) dv_field_full(struct name, __VA_ARGS__)
#define dv_field_at(name, ...) dv_field_at_full(struct name, __VA_ARGS__)
