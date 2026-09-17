/* @title: Compiler Functions */
#pragma once

#define cc_init /* Nothing for now, TODO: */

#define cc_noinline __attribute__((noinline))
#define cc_always_inline __attribute__((always_inline))
#define cc_noreturn __attribute__((noreturn))
#define cc_unused __attribute__((unused))
#define cc_warn_unused_result __attribute__((warn_unused_result))
#define cc_packed __attribute__((__packed__))
#define cc_aligned(x) __attribute__((aligned(x)))
#define cc_cache_aligned __attribute__((aligned(64)))
#define cc_used __attribute__((used))
#define cc_section(x) __attribute__((section(x)))
#define cc_hidden __attribute__((visibility("hidden")))
#define cc_export __attribute__((visibility("default")))

#define cc_likely(x) __builtin_expect(!!(x), 1)
#define cc_unlikely(x) __builtin_expect(!!(x), 0)
#define cc_unreachable() __builtin_unreachable()

#define cc_no_sanitize_address __attribute__((no_sanitize("address")))
#define cc_deprecated __attribute__((deprecated))
#define cc_deprecated_msg(msg) __attribute__((deprecated(msg)))
#define cc_pure __attribute__((pure))
#define cc_constfn __attribute__((const))
#define cc_noclone __attribute__((noclone))
#define cc_malloc_like __attribute__((malloc))
#define cc_nullable __attribute__((nullable))
#define cc_naked __attribute__((naked))
#define cc_returns_nonnull __attribute__((returns_nonnull))
#define cc_returns_twice __attribute__((returns_twice))
#define cc_may_alias __attribute__((__may_alias__))
#define cc_weak __attribute__((weak))
#define cc_weakref(sym) __attribute__((weakref(#sym)))
#define cc_alias(sym) __attribute__((alias(#sym)))
#define cc_no_sanitize_undefined __attribute__((no_sanitize("undefined")))
#define cc_no_sanitize_thread __attribute__((no_sanitize("thread")))
#define cc_no_sanitize_memory __attribute__((no_sanitize("memory")))
#define cc_no_sanitize_coverage __attribute__((no_sanitize("coverage")))
#define cc_printf_like(fmt_idx, arg_idx)                                       \
    __attribute__((format(printf, fmt_idx, arg_idx)))
#define cc_constructor(prio) __attribute__((constructor(prio)))
#define cc_destructor(prio) __attribute__((destructor(prio)))
#define cc_fallthrough __attribute__((fallthrough))

#if defined(__GNUC__)
#define cc_restrict __restrict__
#else
#define cc_restrict
#endif

/* Preprocessor Metaprogramming Helpers */

#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,  \
                 _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,   \
                 _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38,   \
                 _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,   \
                 _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62,   \
                 _63, _64, N, ...)                                             \
    N

#define PP_RSEQ_N()                                                            \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,    \
        45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29,    \
        28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,    \
        11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define PP_NARG_HELPER(...) PP_ARG_N(__VA_ARGS__)
#define PP_NARG(...) PP_NARG_HELPER(_, ##__VA_ARGS__, PP_RSEQ_N())

#define PP_DISPATCH_(name, n) name##_##n
#define PP_DISPATCH(name, n) PP_DISPATCH_(name, n)

#define PP_OVERLOAD(name, ...) PP_DISPATCH(name, PP_NARG(__VA_ARGS__))
#define PP_CALL(name, ...) PP_OVERLOAD(name, __VA_ARGS__)(__VA_ARGS__)

#define PP_CONCAT_(a, b) a##b
#define PP_CONCAT(a, b) PP_CONCAT_(a, b)

#define PP_STRINGIZE_(x) #x
#define PP_STRINGIZE(x) PP_STRINGIZE_(x)

#define PP_EXPAND(...) __VA_ARGS__

/* Unused argument discarders */
#define PP_CC_VAR_UNUSED_1(a) ((void) (a))
#define PP_CC_VAR_UNUSED_2(a, b) ((void) (a), (void) (b))
#define PP_CC_VAR_UNUSED_3(a, b, c) ((void) (a), (void) (b), (void) (c))
#define PP_CC_VAR_UNUSED_4(a, b, c, d)                                         \
    ((void) (a), (void) (b), (void) (c), (void) (d))
#define PP_CC_VAR_UNUSED_5(a, b, c, d, e)                                      \
    ((void) (a), (void) (b), (void) (c), (void) (d), (void) (e))
#define PP_CC_VAR_UNUSED_6(a, b, c, d, e, f)                                   \
    ((void) (a), (void) (b), (void) (c), (void) (d), (void) (e), (void) (f))
#define PP_CC_VAR_UNUSED_7(a, b, c, d, e, f, g)                                \
    ((void) (a), (void) (b), (void) (c), (void) (d), (void) (e), (void) (f),   \
     (void) (g))
#define PP_CC_VAR_UNUSED_8(a, b, c, d, e, f, g, h)                             \
    ((void) (a), (void) (b), (void) (c), (void) (d), (void) (e), (void) (f),   \
     (void) (g), (void) (h))

#define cc_var_unused(...) PP_CALL(PP_CC_VAR_UNUSED, __VA_ARGS__)

/* Compile-Time & Static Assertions */

#define static_assert_1(cond) _Static_assert(cond, #cond)
#define static_assert_2(cond, msg) _Static_assert(cond, msg)
#define static_assert(...) PP_CALL(static_assert, __VA_ARGS__)

#define ct_assert_size(type, size)                                             \
    _Static_assert(sizeof(type) == (size), "sizeof(" #type ") != " #size)

#define ct_assert_align(type, align)                                           \
    _Static_assert(_Alignof(type) == (align), "alignof(" #type ") != " #align)

#define ct_assert_offset(type, member, offset)                                 \
    _Static_assert(__builtin_offsetof(type, member) == (offset),               \
                   "offsetof(" #type ", " #member ") != " #offset)

#define ct_assert_same_size(type_a, type_b)                                    \
    _Static_assert(sizeof(type_a) == sizeof(type_b),                           \
                   "sizeof(" #type_a ") != sizeof(" #type_b ")")

#define ct_assert_struct_size_eq(__struct, __want)                             \
    ct_assert_size(struct __struct, __want)

/* Memory barriers */
#define smp_mb() atomic_thread_fence(memory_order_seq_cst)
#define smp_rmb() atomic_thread_fence(memory_order_acquire)
#define smp_wmb() atomic_thread_fence(memory_order_release)

/* Compile-Time Type System & Metaprogramming (ct_) */
#define ct_is_const(x) __builtin_constant_p(x)

#define ct_same_type(a, b)                                                     \
    __builtin_types_compatible_p(__typeof__(a), __typeof__(b))
#define ct_is_type(T, x) __builtin_types_compatible_p(T, __typeof__(x))

#define ct_is_array(a)                                                         \
    (!__builtin_types_compatible_p(__typeof__(a), __typeof__(&(a)[0])))

#define ct_array_size(a)                                                       \
    (sizeof(char[ct_is_array(a) ? 1 : -1]) * 0 + (sizeof(a) / sizeof((a)[0])))

#define ct_const_min(a, b) __builtin_choose_expr((a) < (b), (a), (b))
#define ct_const_max(a, b) __builtin_choose_expr((a) > (b), (a), (b))
#define ct_const_clamp(val, min, max) ct_const_min(ct_const_max(val, min), max)

#define ct_decay(x) __typeof__(0 ? (x) : ((void) 0, (x)))
#define ct_is_str(x)                                                           \
    (__builtin_types_compatible_p(ct_decay(x), char *) ||                      \
     __builtin_types_compatible_p(ct_decay(x), const char *))

#define ct_as_str(x) ((const char *) (uintptr_t) (x))
#define ct_as_type(T, x) ((T) (uintptr_t) (x))

/* Require expr to have the requested type, does not evaluate x */
#define ct_typecheck(type, x)                                                  \
    ((void) sizeof(                                                            \
        char[__builtin_types_compatible_p(type, __typeof__(x)) ? 1 : -1]))

#define ct_typecheck_same(x, y) ct_typecheck(__typeof__(x), y)

/* Must be integer, not boolean */
#define ct_typecheck_integer(x)                                                \
    ((void) sizeof(char[(__builtin_classify_type(x) == 1 &&                    \
                         !__builtin_types_compatible_p(__typeof__(x), _Bool))  \
                            ? 1                                                \
                            : -1]))

/* Require x to have a signed integer or enum type */
#define ct_typecheck_signed(x)                                                 \
    (ct_typecheck_integer(x),                                                  \
     (void) sizeof(char[((__typeof__(x)) -1 < (__typeof__(x)) 0) ? 1 : -1]))

/* Require x to have an unsigned integer or enum type */
#define ct_typecheck_unsigned(x)                                               \
    (ct_typecheck_integer(x),                                                  \
     (void) sizeof(char[((__typeof__(x)) -1 > (__typeof__(x)) 0) ? 1 : -1]))

#define ct_type_is_signed(x) ((__typeof__(x)) -1 < (__typeof__(x)) 1)

#define ct_typecheck_intmax_nonnegative(x)                                     \
    (((__UINTMAX_TYPE__) (__INTMAX_TYPE__) (x) >>                              \
      (sizeof(__INTMAX_TYPE__) * __CHAR_BIT__ - 1)) == 0)

#define ct_typecheck_source_nonnegative(to, from)                              \
    (!(ct_type_is_signed(from) && !ct_type_is_signed(to)) ||                   \
     ct_typecheck_intmax_nonnegative(from))

#define ct_typecheck_converted_nonnegative(to, from)                           \
    (!(!ct_type_is_signed(from) && ct_type_is_signed(to)) ||                   \
     ct_typecheck_intmax_nonnegative((__typeof__(to)) (from)))

/* An integer constant expr only when `from` is too */
#define ct_typecheck_value_fits(to, from)                                      \
    (ct_typecheck_source_nonnegative(to, from) &&                              \
     ct_typecheck_converted_nonnegative(to, from) &&                           \
     (__typeof__(from)) ((__typeof__(to)) (from)) ==                           \
         (__typeof__(from)) (from))

/* Whole type widening from signedness and width,
 * always an integer constant expr */
#define ct_typecheck_structurally_widenable(to, from)                          \
    (((ct_type_is_signed(to) == ct_type_is_signed(from)) &&                    \
      sizeof(__typeof__(from)) <= sizeof(__typeof__(to))) ||                   \
     (ct_type_is_signed(to) && !ct_type_is_signed(from) &&                     \
      sizeof(__typeof__(from)) < sizeof(__typeof__(to))))

/* We use __builtin_choose_expr here because it enforces the check at compile
 * time instead of a runtime value, and thus it also enforces the check
 * on both arms at compile time */
#define ct_widenable_ok(to, from)                                              \
    __builtin_choose_expr(__builtin_constant_p(from),                          \
                          (ct_typecheck_structurally_widenable(to, from)) ||   \
                              (ct_typecheck_value_fits(to, from)),             \
                          (ct_typecheck_structurally_widenable(to, from)))

/* Require every value of source to be representable by destination,
 * or source to be a constant whose value is representable by
 * the destination. Equal widths require equal signedness, and unsigned
 * sources require wider signed destinations */
#define ct_typecheck_widenable_to(destination, source)                         \
    (ct_typecheck_integer(destination), ct_typecheck_integer(source),          \
     (void) sizeof(char[ct_widenable_ok(destination, source) ? 1 : -1]))

/* Common/larger type to cast up to */
#define ct_common_type_2(a, b)                                                 \
    __typeof__(__builtin_choose_expr(                                          \
        sizeof(__typeof__(a)) >= sizeof(__typeof__(b)), (a), (b)))
