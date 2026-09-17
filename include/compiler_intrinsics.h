/* @title: Compiler Intrinsics */
#pragma once

#define ci_unreachable() __builtin_unreachable()
#define ci_expect(x, v) __builtin_expect((x), (v))

#define ci_return_address(level) __builtin_return_address(level)
#define ci_frame_address(level) __builtin_frame_address(level)
#define ci_assume_aligned(ptr, ...) __builtin_assume_aligned((ptr), __VA_ARGS__)

#define ci_clz(x) __builtin_clz(x)
#define ci_clzll(x) __builtin_clzll(x)
#define ci_ctz(x) __builtin_ctz(x)
#define ci_ctzll(x) __builtin_ctzll(x)
#define ci_popcount(x) __builtin_popcount(x)
#define ci_popcountll(x) __builtin_popcountll(x)
#define ci_ffsll(x) __builtin_ffsll(x)

#define ci_add_overflow(a, b, res) __builtin_add_overflow((a), (b), (res))
#define ci_sub_overflow(a, b, res) __builtin_sub_overflow((a), (b), (res))
#define ci_mul_overflow(a, b, res) __builtin_mul_overflow((a), (b), (res))

#define ci_constant_p(x) __builtin_constant_p(x)
#define ci_choose_expr(cond, a, b) __builtin_choose_expr((cond), (a), (b))
#define ci_types_compatible_p(a, b) __builtin_types_compatible_p(a, b)
#define ci_offsetof(type, member) __builtin_offsetof(type, member)
#define ci_classify_type(x) __builtin_classify_type(x)

#define ci_memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#define ci_memset(s, c, n) __builtin_memset((s), (c), (n))
#define ci_memmove(dest, src, n) __builtin_memmove((dest), (src), (n))
#define ci_memcmp(s1, s2, n) __builtin_memcmp((s1), (s2), (n))
