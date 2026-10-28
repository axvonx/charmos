/* @title: Compiler Core & Compile-Time Metaprogramming */
#pragma once
#include <compiler/name.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ==== cc_ Attributes, Markers, and Compiler Directives ==== */
#define cc_align_as(x) _Alignas(x)
#define cc_alignof(x) _Alignof(x)
#define cc_generic(...) _Generic(__VA_ARGS__)

/* clang-format off */
#define cc_bitfield_barrier(t) t:0
/* clang-format on */

#define cc_code_init /* Nothing for now, TODO: */

#define cc_noinline __attribute__((noinline))
#define cc_always_inline __attribute__((always_inline))
#define cc_noreturn __attribute__((noreturn))
#define cc_unused __attribute__((unused))
#define cc_maybe_unused __attribute__((unused))
#define cc_warn_unused_result __attribute__((warn_unused_result))
#define cc_nodiscard __attribute__((warn_unused_result))
#define cc_must_check [[nodiscard]]
#define cc_packed __attribute__((__packed__))
#define cc_aligned(x) __attribute__((aligned(x)))
#define cc_cache_aligned __attribute__((aligned(64)))
#define cc_used __attribute__((used))
#define cc_section(x) __attribute__((section(x)))
#define cc_hidden __attribute__((visibility("hidden")))
#define cc_export __attribute__((visibility("default")))
#define cc_overloadable __attribute__((overloadable))

#define cc_likely(x) __builtin_expect(!!(x), 1)
#define cc_unlikely(x) __builtin_expect(!!(x), 0)
#define cc_unreachable() __builtin_unreachable()

#if defined(cn_id_clang)
#define cc_assume(expr) __builtin_assume(expr)
#elif cn_has_attribute(assume)
#define cc_assume(expr) __attribute__((assume(expr)))
#else
#define cc_assume(expr)                                                        \
    do {                                                                       \
        if (!(expr))                                                           \
            __builtin_unreachable();                                           \
    } while (0)
#endif

#define cc_cold __attribute__((cold))
#define cc_hot __attribute__((hot))
#define cc_flatten __attribute__((flatten))

#if cn_has_attribute(nodebug)
#define cc_nodebug __attribute__((nodebug))
#else
#define cc_nodebug
#endif

#define cc_no_asan __attribute__((no_sanitize("address")))
#define cc_no_ubsan __attribute__((no_sanitize("undefined")))
#define cc_no_tsan __attribute__((no_sanitize("thread")))

#if defined(cn_id_clang)
#define cc_no_msan __attribute__((no_sanitize("memory")))
#define cc_no_csan __attribute__((no_sanitize("coverage")))
#elif cn_has_attribute(no_sanitize_coverage)
#define cc_no_msan /* GCC has no MSan */
#define cc_no_csan __attribute__((no_sanitize_coverage))
#else
#define cc_no_msan
#define cc_no_csan
#endif

#define cc_no_sanitize(...) __attribute__((no_sanitize(__VA_ARGS__)))
#define cc_no_stack_protector __attribute__((no_stack_protector))
#define cc_no_instrument __attribute__((no_instrument_function))

#define cc_deprecated __attribute__((deprecated))
#define cc_deprecated_msg(msg) __attribute__((deprecated(msg)))
#define cc_pure __attribute__((pure))
#define cc_constfn __attribute__((const))
#define cc_noclone __attribute__((noclone))
#define cc_malloc_like __attribute__((malloc))
#define cc_naked __attribute__((naked))
#define cc_interrupt __attribute__((interrupt))
#define cc_no_caller_saved_registers __attribute__((no_caller_saved_registers))
#define cc_returns_nonnull __attribute__((returns_nonnull))
#define cc_returns_twice __attribute__((returns_twice))
#define cc_may_alias __attribute__((__may_alias__))
#define cc_weak __attribute__((weak))
#define cc_weakref(sym) __attribute__((weakref(#sym)))
#define cc_alias(sym) __attribute__((alias(#sym)))

#define cc_alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
#define cc_alloc_align(param_idx) __attribute__((alloc_align(param_idx)))

#if defined(cn_id_clang)
#define cc_nullable _Nullable
#define cc_nonnull_ptr _Nonnull
#else
#define cc_nullable
#define cc_nonnull_ptr
#endif

#if cn_id_gcc_at_least(11, 0)
#define cc_dealloc(fn, arg_idx) __attribute__((malloc(fn, arg_idx)))
#else
#define cc_dealloc(fn, arg_idx)
#endif

#if defined(cn_id_clang)
#define cc_owner_returns(tag, ...)                                             \
    __attribute__((ownership_returns(tag, ##__VA_ARGS__)))
#define cc_owner_takes(tag, arg_idx)                                           \
    __attribute__((ownership_takes(tag, arg_idx)))
#else
#define cc_owner_returns(tag, ...)
#define cc_owner_takes(tag, arg_idx)
#endif

#if defined(cn_id_clang)
#define cc_analyzer_noreturn __attribute__((analyzer_noreturn))
#else
#define cc_analyzer_noreturn
#endif

#define cc_error(msg) __attribute__((error(msg)))
#define cc_warning(msg) __attribute__((warning(msg)))
#define cc_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))

/* Bounds for a flexible array or pointer member */
#if defined(cn_id_clang) || cn_id_gcc_at_least(16, 0)
#define cc_counted_by(member) __attribute__((counted_by(member)))
#else
#define cc_counted_by(member)
#endif

#if cn_has_attribute(counted_by_or_null)
#define cc_counted_by_or_null(member)                                          \
    __attribute__((counted_by_or_null(member)))
#else
#define cc_counted_by_or_null(member)
#endif

#if cn_has_attribute(sized_by)
#define cc_sized_by(member) __attribute__((sized_by(member)))
#else
#define cc_sized_by(member)
#endif

#if cn_has_attribute(sized_by_or_null)
#define cc_sized_by_or_null(member) __attribute__((sized_by_or_null(member)))
#else
#define cc_sized_by_or_null(member)
#endif

/*
 *     void fill(void *p cc_pass_object_size(0), size_t n)
 *         cc_diagnose_if(n > __builtin_object_size(p, 0), "overflow", "error");
 */
#if cn_has_attribute(pass_object_size)
#define cc_pass_object_size(type) __attribute__((pass_object_size(type)))
#else
#define cc_pass_object_size(type)
#endif

#if cn_has_attribute(diagnose_if)
#define cc_diagnose_if(cond, msg, severity)                                    \
    __attribute__((diagnose_if(cond, msg, severity)))
#else
#define cc_diagnose_if(cond, msg, severity)
#endif

/*
 *     void take(void *p, unsigned flags, size_t n)
 *         ct_precond(p != NULL)
 *         ct_precond(flags & F_A, "use take_noflag() when F_A is clear")
 *         ct_precond(n <= 4096);
 *
 * Notably only is able to check constants passed in.
 * enforce_precond() for runtime precondition validation
 */
#define ct_precond(...)                                                        \
    PP_CONCAT(_ct_precond_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
#define _ct_precond_1(cond)                                                    \
    cc_diagnose_if(!(cond), "precondition violated: " #cond, "error")
#define _ct_precond_2(cond, hint)                                              \
    cc_diagnose_if(!(cond), "precondition violated: " #cond " -- " hint,       \
                   "error")

#if cn_has_attribute(enable_if)
#define cc_enable_if(cond, msg) __attribute__((enable_if(cond, msg)))
#else
#define cc_enable_if(cond, msg)
#endif

#if defined(cn_id_gcc)
#define cc_access(mode, ...) __attribute__((access(mode, __VA_ARGS__)))
#else
#define cc_access(mode, ...)
#endif

#define cc_printf_like(fmt_idx, arg_idx)                                       \
    __attribute__((format(printf, fmt_idx, arg_idx)))
#define cc_constructor(prio) __attribute__((constructor(prio)))
#define cc_destructor(prio) __attribute__((destructor(prio)))
#define cc_fallthrough __attribute__((fallthrough))
#define cc_cleanup(fn) __attribute__((cleanup(fn)))
#define cc_target(...) __attribute__((target(__VA_ARGS__)))
#define cc_optimize(...) __attribute__((optimize(__VA_ARGS__)))
#define cc_designated_init __attribute__((designated_init))

#if cn_has_attribute(uninitialized)
#define cc_uninitialized __attribute__((uninitialized))
#else
#define cc_uninitialized
#endif

#if cn_has_attribute(nonstring)
#define cc_nonstring __attribute__((nonstring))
#else
#define cc_nonstring
#endif

#if defined(cn_id_gcc) || defined(cn_id_clang)
#define cc_restrict __restrict__
#else
#define cc_restrict
#endif

#if defined(cn_id_clang)
#define cc_mem(n) __attribute__((address_space(n)))
#else
#define cc_mem(n)
#endif

#define cc_mem_io cc_mem(1)

#if defined(cn_id_clang)
#define cc_mem_rcu __attribute__((address_space(2), noderef))
#else
#define cc_mem_rcu
#endif

#if defined(cn_id_clang)
#define cc_mem_any_fn __attribute__((address_space(3), noderef))
#else
#define cc_mem_any_fn
#endif

/* ==== Preprocessor Metaprogramming ==== */

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

/* The 2 and 3 variants are used when a PP_CALL wants to expand
 * a PP_CALL (first layer), and when a PP_CALL2 wants to expand
 * a PP_CALL or PP_CALL2 (second layer). If anyone wants another
 * layer, they should probably restructure their code */
#define PP_OVERLOAD2(name, ...) PP_DISPATCH(name, PP_NARG(__VA_ARGS__))
#define PP_CALL2(name, ...) PP_OVERLOAD2(name, __VA_ARGS__)(__VA_ARGS__)

#define PP_OVERLOAD3(name, ...) PP_DISPATCH(name, PP_NARG(__VA_ARGS__))
#define PP_CALL3(name, ...) PP_OVERLOAD3(name, __VA_ARGS__)(__VA_ARGS__)

#define PP_CONCAT_(a, b) a##b
#define PP_CONCAT(a, b) PP_CONCAT_(a, b)

#define PP_STRINGIZE_(x) #x
#define PP_STRINGIZE(x) PP_STRINGIZE_(x)

#define PP_EXPAND(...) __VA_ARGS__

#define PP_GET_1(a, ...) a
#define PP_GET_2(a, b, ...) b
#define PP_GET_3(a, b, c, ...) c
#define PP_GET_4(a, b, c, d, ...) d
#define PP_GET_5(a, b, c, d, e, ...) e

#define PP_ARG(n, ...) PP_CONCAT(PP_GET_, n)(__VA_ARGS__)

#define PP_DROP_1(a, ...) __VA_ARGS__
#define PP_DROP_2(a, b, ...) __VA_ARGS__
#define PP_DROP_3(a, b, c, ...) __VA_ARGS__
#define PP_DROP_4(a, b, c, d, ...) __VA_ARGS__
#define PP_DROP_5(a, b, c, d, e, ...) __VA_ARGS__

#define PP_VA_ARGS_AFTER(n, ...) PP_CONCAT(PP_DROP_, n)(__VA_ARGS__)

/* Unused argument discarders */
#define PP_CC_VAR_UNUSED_1(a) ((void) (a))
#define PP_CC_VAR_UNUSED_2(a, b) (PP_CC_VAR_UNUSED_1(a), (void) (b))
#define PP_CC_VAR_UNUSED_3(a, b, c) (PP_CC_VAR_UNUSED_2(a, b), (void) (c))
#define PP_CC_VAR_UNUSED_4(a, b, c, d) (PP_CC_VAR_UNUSED_3(a, b, c), (void) (d))
#define PP_CC_VAR_UNUSED_5(a, b, c, d, e)                                      \
    (PP_CC_VAR_UNUSED_4(a, b, c, d), (void) (e))
#define PP_CC_VAR_UNUSED_6(a, b, c, d, e, f)                                   \
    (PP_CC_VAR_UNUSED_5(a, b, c, d, e), (void) (f))
#define PP_CC_VAR_UNUSED_7(a, b, c, d, e, f, g)                                \
    (PP_CC_VAR_UNUSED_6(a, b, c, d, e, f), (void) (g))
#define PP_CC_VAR_UNUSED_8(a, b, c, d, e, f, g, h)                             \
    (PP_CC_VAR_UNUSED_7(a, b, c, d, e, f, g), (void) (h))

#define PP_UNPAREN(x) PP_UNPAREN_ESC_(PP_UNPAREN_ISH_ x)
#define PP_UNPAREN_ISH_(...) PP_UNPAREN_ISH_ __VA_ARGS__
#define PP_UNPAREN_ESC_(...) PP_UNPAREN_ESC__(__VA_ARGS__)
#define PP_UNPAREN_ESC__(...) PP_UNPAREN_VAN_##__VA_ARGS__
#define PP_UNPAREN_VAN_PP_UNPAREN_ISH_

/* 1 if x is parenthesized, else 0 */
#define PP_IS_PAREN(x) PP_IS_PAREN_N_(PP_IS_PAREN_PROBE_ x, 0, ~)
#define PP_IS_PAREN_PROBE_(...) ~, 1,
#define PP_IS_PAREN_N_(...) PP_IS_PAREN_PICK_(__VA_ARGS__)
#define PP_IS_PAREN_PICK_(x, n, ...) n

/* PP_IIF(0 or 1)(then, else) */
#define PP_IIF(c) PP_CONCAT(PP_IIF_, c)
#define PP_IIF_0(t, f) f
#define PP_IIF_1(t, f) t

#define PP_FOREACH_CTX(m, c, ...)                                              \
    PP_CONCAT(PP_FE_, PP_NARG(__VA_ARGS__))(m, c, __VA_ARGS__)
#define PP_FE_0(m, c, ...)
#define PP_FE_1(m, c, x) m(c, x)
#define PP_FE_2(m, c, x, ...) m(c, x) PP_FE_1(m, c, __VA_ARGS__)
#define PP_FE_3(m, c, x, ...) m(c, x) PP_FE_2(m, c, __VA_ARGS__)
#define PP_FE_4(m, c, x, ...) m(c, x) PP_FE_3(m, c, __VA_ARGS__)
#define PP_FE_5(m, c, x, ...) m(c, x) PP_FE_4(m, c, __VA_ARGS__)
#define PP_FE_6(m, c, x, ...) m(c, x) PP_FE_5(m, c, __VA_ARGS__)
#define PP_FE_7(m, c, x, ...) m(c, x) PP_FE_6(m, c, __VA_ARGS__)
#define PP_FE_8(m, c, x, ...) m(c, x) PP_FE_7(m, c, __VA_ARGS__)
#define PP_FE_9(m, c, x, ...) m(c, x) PP_FE_8(m, c, __VA_ARGS__)
#define PP_FE_10(m, c, x, ...) m(c, x) PP_FE_9(m, c, __VA_ARGS__)
#define PP_FE_11(m, c, x, ...) m(c, x) PP_FE_10(m, c, __VA_ARGS__)
#define PP_FE_12(m, c, x, ...) m(c, x) PP_FE_11(m, c, __VA_ARGS__)
#define PP_FE_13(m, c, x, ...) m(c, x) PP_FE_12(m, c, __VA_ARGS__)
#define PP_FE_14(m, c, x, ...) m(c, x) PP_FE_13(m, c, __VA_ARGS__)
#define PP_FE_15(m, c, x, ...) m(c, x) PP_FE_14(m, c, __VA_ARGS__)
#define PP_FE_16(m, c, x, ...) m(c, x) PP_FE_15(m, c, __VA_ARGS__)
#define PP_FE_17(m, c, x, ...) m(c, x) PP_FE_16(m, c, __VA_ARGS__)
#define PP_FE_18(m, c, x, ...) m(c, x) PP_FE_17(m, c, __VA_ARGS__)
#define PP_FE_19(m, c, x, ...) m(c, x) PP_FE_18(m, c, __VA_ARGS__)
#define PP_FE_20(m, c, x, ...) m(c, x) PP_FE_19(m, c, __VA_ARGS__)
#define PP_FE_21(m, c, x, ...) m(c, x) PP_FE_20(m, c, __VA_ARGS__)
#define PP_FE_22(m, c, x, ...) m(c, x) PP_FE_21(m, c, __VA_ARGS__)
#define PP_FE_23(m, c, x, ...) m(c, x) PP_FE_22(m, c, __VA_ARGS__)
#define PP_FE_24(m, c, x, ...) m(c, x) PP_FE_23(m, c, __VA_ARGS__)
#define PP_FE_25(m, c, x, ...) m(c, x) PP_FE_24(m, c, __VA_ARGS__)
#define PP_FE_26(m, c, x, ...) m(c, x) PP_FE_25(m, c, __VA_ARGS__)
#define PP_FE_27(m, c, x, ...) m(c, x) PP_FE_26(m, c, __VA_ARGS__)
#define PP_FE_28(m, c, x, ...) m(c, x) PP_FE_27(m, c, __VA_ARGS__)
#define PP_FE_29(m, c, x, ...) m(c, x) PP_FE_28(m, c, __VA_ARGS__)
#define PP_FE_30(m, c, x, ...) m(c, x) PP_FE_29(m, c, __VA_ARGS__)
#define PP_FE_31(m, c, x, ...) m(c, x) PP_FE_30(m, c, __VA_ARGS__)
#define PP_FE_32(m, c, x, ...) m(c, x) PP_FE_31(m, c, __VA_ARGS__)

#define cc_ignore(...) PP_CALL(PP_CC_VAR_UNUSED, __VA_ARGS__)

#define cc_discard(expr)                                                       \
    ({                                                                         \
        __auto_type _cc_discard_ = (expr);                                     \
        (void) _cc_discard_;                                                   \
    })

/* ==== ct_ Compile-Time & Static Assertions ==== */

#define static_assert_1(cond) _Static_assert(cond, #cond)
#define static_assert_2(cond, msg) _Static_assert(cond, msg)

#define static_assert(...) PP_CALL2(static_assert, __VA_ARGS__)

#define ct_assert_size(type, size)                                             \
    static_assert(sizeof(type) == (size), "sizeof(" #type ") != " #size)

#define ct_assert_align(type, align)                                           \
    static_assert(cc_alignof(type) == (align), "alignof(" #type ") "           \
                                               "!= " #align)

#define ct_assert_offset(type, member, offset)                                 \
    static_assert(__builtin_offsetof(type, member) == (offset),                \
                  "offsetof(" #type ", " #member ") != " #offset)

#define ct_assert_same_size(type_a, type_b)                                    \
    static_assert(sizeof(type_a) == sizeof(type_b),                            \
                  "sizeof(" #type_a ") != sizeof(" #type_b ")")

#define ct_assert_struct_size_eq(struct_, want_)                               \
    ct_assert_size(struct struct_, want_)

#define ct_assert_struct_align_eq(struct_, want_)                              \
    ct_assert_align(struct struct_, want_)

#define ct_assert_union_size_eq(union_, want_)                                 \
    ct_assert_size(union union_, want_)

#define ct_assert_union_align_eq(union_, want_)                                \
    ct_assert_align(union union_, want_)

#define ct_assert_power_of_two(n)                                              \
    static_assert(((n) > 0 && (((n) & ((n) - 1)) == 0)),                       \
                  #n " is not a power of two")

#define ct_assert_nonempty_str(s)                                              \
    static_assert(sizeof(s) > 1, "empty string: " #s)

#define ct_strong_int(stem, STEM, base, max_)                                  \
    typedef enum : base {                                                      \
        STEM##_ZERO = 0,                                                       \
        STEM##_MIN = 0,                                                        \
        STEM##_MAX = (max_),                                                   \
    } stem##_t

#define ct_expr_assert_zero(cond, msg)                                         \
    (sizeof(struct {                                                           \
         _Static_assert(__builtin_choose_expr((cond), 1, 0), msg);             \
         char __ct_pad;                                                        \
     }) *                                                                      \
     0)

#define ct_expr_assert(cond, msg) ((void) ct_expr_assert_zero(cond, msg))

/* === ct_ Compile-Time Type System & Reflection ==== */

#define ct_is_const(x) __builtin_constant_p(x)

#define ct_is_ice(x)                                                           \
    (sizeof(int) == sizeof(*(8 ? ((void *) ((long) (x) * 0l)) : (int *) 8)))

#define ct_raw(x) ((typeof((x) + 0)) (x))

#define ct_same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define ct_is_type(T, x) __builtin_types_compatible_p(T, typeof(x))

#define ct_is_array(a)                                                         \
    (!__builtin_types_compatible_p(typeof(a), typeof(&(a)[0])))

#define ct_array_size(a)                                                       \
    (ct_expr_assert_zero(ct_is_array(a), "`" #a "` is a pointer, not an "      \
                                         "array; ct_array_size() needs a "     \
                                         "real array") +                       \
     (sizeof(a) / sizeof((a)[0])))

#define ct_const_min(a, b) __builtin_choose_expr((a) < (b), (a), (b))
#define ct_const_max(a, b) __builtin_choose_expr((a) > (b), (a), (b))
#define ct_const_clamp(val, min, max) ct_const_min(ct_const_max(val, min), max)

#define ct_decay(x) typeof(0 ? (x) : ((void) ++(int) {0}, (x)))
#define ct_is_str(x)                                                           \
    (__builtin_types_compatible_p(ct_decay(x), char *) ||                      \
     __builtin_types_compatible_p(ct_decay(x), const char *))

#define ct_as_str(x) ((const char *) (uintptr_t) (x))
#define ct_as_type(T, x) ((T) (uintptr_t) (x))

#define ct_bitsizeof(x) (sizeof(x) * __CHAR_BIT__)
#define ct_type_bitsize(T) (sizeof(T) * __CHAR_BIT__)

#define ct_field_sizeof(type, member) sizeof(((type *) 0)->member)
#define ct_field_offset(type, member) __builtin_offsetof(type, member)

#define ct_is_pointer(x) (__builtin_classify_type(x) == 5)
#define ct_is_fn_ptr(x)                                                        \
    __builtin_types_compatible_p(typeof(&*(x)), typeof(&**(x)))
#define ct_is_integral(x)                                                      \
    (__builtin_classify_type(x) == 1 &&                                        \
     !__builtin_types_compatible_p(typeof(x), bool))
#define ct_is_bool(x) __builtin_types_compatible_p(typeof(x), _Bool)
#define ct_is_struct(x) (__builtin_classify_type(x) == 12)
#define ct_is_union(x) (__builtin_classify_type(x) == 13)

#define ct_is_power_of_two(n) ((n) > 0 && (((n) & ((n) - 1)) == 0))

#define ct_type_is_signed(x) ((typeof(x)) -1 < (typeof(x)) 1)

#define ct_min_val(T)                                                          \
    ((T) (ct_type_is_signed(T)                                                 \
              ? (T) ((__UINTMAX_TYPE__) 1 << (ct_type_bitsize(T) - 1))         \
              : (T) 0))

#define ct_max_val(T)                                                          \
    ((T) (ct_type_is_signed(T)                                                 \
              ? (T) (((__UINTMAX_TYPE__) 1 << (ct_type_bitsize(T) - 1)) - 1)   \
              : (T) ~(T) 0))

/* Require expr to have the requested type, does not evaluate x */
#define ct_typecheck(type, x)                                                  \
    ct_expr_assert(__builtin_types_compatible_p(type, typeof(x)),              \
                   "`" #x "` must have type `" #type "`")

#define ct_typecheck_same(x, y)                                                \
    ct_expr_assert(__builtin_types_compatible_p(typeof(x), typeof(y)),         \
                   "`" #x "` and `" #y "` must have the same type")

#define ct_typecheck_same_unqual(x, y)                                         \
    ct_expr_assert(__builtin_types_compatible_p(__typeof_unqual__(x),          \
                                                __typeof_unqual__(y)),         \
                   "`" #x "` and `" #y "` must have the same type")

#define ct_typecheck_integer_as(x, name)                                       \
    ct_expr_assert(ct_is_integral(x), "`" #name "` must have an integer "      \
                                      "type ")
#define ct_typecheck_integer(x) ct_typecheck_integer_as(x, x)

/* Require x to have a signed integer or enum type */
#define ct_typecheck_signed_as(x, name)                                        \
    (ct_typecheck_integer_as(x, name),                                         \
     ct_expr_assert(ct_type_is_signed(x),                                      \
                    "`" #name "` must have a signed integer type"))
#define ct_typecheck_signed(x) ct_typecheck_signed_as(x, x)

/* Require x to have an unsigned integer or enum type */
#define ct_typecheck_unsigned_as(x, name)                                      \
    (ct_typecheck_integer_as(x, name),                                         \
     ct_expr_assert(!ct_type_is_signed(x),                                     \
                    "`" #name "` must have an unsigned integer type"))
#define ct_typecheck_unsigned(x) ct_typecheck_unsigned_as(x, x)

#define ct_typecheck_intmax_nonnegative(x)                                     \
    (((__UINTMAX_TYPE__) (__INTMAX_TYPE__) (x) >>                              \
      (sizeof(__INTMAX_TYPE__) * __CHAR_BIT__ - 1)) == 0)

#define ct_typecheck_source_nonnegative(to, from)                              \
    (!(ct_type_is_signed(from) && !ct_type_is_signed(to)) ||                   \
     ct_typecheck_intmax_nonnegative(from))

#define ct_typecheck_converted_nonnegative(to, from)                           \
    (!(!ct_type_is_signed(from) && ct_type_is_signed(to)) ||                   \
     ct_typecheck_intmax_nonnegative((typeof(to)) (from)))

#define ct_typecheck_value_fits(to, from)                                      \
    (ct_typecheck_source_nonnegative(to, from) &&                              \
     ct_typecheck_converted_nonnegative(to, from) &&                           \
     (typeof(from)) ((typeof(to)) (from)) == (typeof(from)) (from))

/* Whole type widening from signedness and width,
 * always an integer constant expr */
#define ct_typecheck_structurally_widenable(to, from)                          \
    (((ct_type_is_signed(to) == ct_type_is_signed(from)) &&                    \
      sizeof(typeof(from)) <= sizeof(typeof(to))) ||                           \
     (ct_type_is_signed(to) && !ct_type_is_signed(from) &&                     \
      sizeof(typeof(from)) < sizeof(typeof(to))))

#define ct_widenable_ok(to, from)                                              \
    __builtin_choose_expr(ct_is_ice(from),                                     \
                          (ct_typecheck_structurally_widenable(to, from)) ||   \
                              (ct_typecheck_value_fits(to, from)),             \
                          (ct_typecheck_structurally_widenable(to, from)))

/* Require every value of source to be representable by destination,
 * or source to be a constant whose value is representable by
 * the destination. Equal widths require equal signedness, and unsigned
 * sources require wider signed destinations */
#define ct_typecheck_widenable_to(destination, source)                         \
    (ct_typecheck_integer(destination), ct_typecheck_integer(source),          \
     ct_expr_assert(ct_widenable_ok(destination, source),                      \
                    "`" #source "` cannot be converted to the destination "    \
                    "type without changing its value. cast it explicitly"))

/* Common/larger type to cast up to */
#define ct_common_type_2(a, b)                                                 \
    typeof(__builtin_choose_expr(sizeof(typeof(a)) >= sizeof(typeof(b)), (a),  \
                                 (b)))
