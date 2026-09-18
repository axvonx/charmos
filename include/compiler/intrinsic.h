/* @title: Compiler Intrinsics (ci_) */
#pragma once
#include <stddef.h>
#include <stdint.h>

/*
 * Prefix Convention:
 *
 * ci_ "Compiler Intrinsic" is for direct 1:1 or minimal wrappers around
 *     compiler builtins (__builtin_*)
 */

#define ci_unreachable() __builtin_unreachable()
#define ci_expect(x, v) __builtin_expect((x), (v))

#define ci_return_address(level) __builtin_return_address(level)
#define ci_frame_address(level) __builtin_frame_address(level)
#define ci_extract_return_addr(addr) __builtin_extract_return_addr(addr)
#define ci_assume_aligned(ptr, ...) __builtin_assume_aligned((ptr), __VA_ARGS__)

#define ci_clz(x) __builtin_clz(x)
#define ci_clzl(x) __builtin_clzl(x)
#define ci_clzll(x) __builtin_clzll(x)
#define ci_ctz(x) __builtin_ctz(x)
#define ci_ctzl(x) __builtin_ctzl(x)
#define ci_ctzll(x) __builtin_ctzll(x)
#define ci_popcount(x) __builtin_popcount(x)
#define ci_popcountl(x) __builtin_popcountl(x)
#define ci_popcountll(x) __builtin_popcountll(x)
#define ci_ffs(x) __builtin_ffs(x)
#define ci_ffsl(x) __builtin_ffsl(x)
#define ci_ffsll(x) __builtin_ffsll(x)
#define ci_parity(x) __builtin_parity(x)
#define ci_parityl(x) __builtin_parityl(x)
#define ci_parityll(x) __builtin_parityll(x)

#define ci_bswap16(x) __builtin_bswap16((uint16_t) (x))
#define ci_bswap32(x) __builtin_bswap32((uint32_t) (x))
#define ci_bswap64(x) __builtin_bswap64((uint64_t) (x))

#define ci_add_overflow(a, b, res) __builtin_add_overflow((a), (b), (res))
#define ci_sub_overflow(a, b, res) __builtin_sub_overflow((a), (b), (res))
#define ci_mul_overflow(a, b, res) __builtin_mul_overflow((a), (b), (res))

#define ci_add_overflow_p(a, b, type) __builtin_add_overflow_p((a), (b), (type))
#define ci_sub_overflow_p(a, b, type) __builtin_sub_overflow_p((a), (b), (type))
#define ci_mul_overflow_p(a, b, type) __builtin_mul_overflow_p((a), (b), (type))

#define ci_constant_p(x) __builtin_constant_p(x)
#define ci_choose_expr(cond, a, b) __builtin_choose_expr((cond), (a), (b))
#define ci_types_compatible_p(a, b) __builtin_types_compatible_p(a, b)
#define ci_offsetof(type, member) __builtin_offsetof(type, member)
#define ci_classify_type(x) __builtin_classify_type(x)

#define ci_object_size(ptr, type) __builtin_object_size((ptr), (type))
#define ci_dynamic_object_size(ptr, type)                                      \
    __builtin_dynamic_object_size((ptr), (type))
#define ci_prefetch(addr, ...) __builtin_prefetch((addr), __VA_ARGS__)
#define ci_trap() __builtin_trap()

#define ci_memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#define ci_memset(s, c, n) __builtin_memset((s), (c), (n))
#define ci_memmove(dest, src, n) __builtin_memmove((dest), (src), (n))
#define ci_memcmp(s1, s2, n) __builtin_memcmp((s1), (s2), (n))
