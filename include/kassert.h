/* @title: Assertions */
#include <compiler.h>
#include <console/panic.h>

#define kassert_1(x)                                                           \
    do {                                                                       \
        if (unlikely(!(x))) {                                                  \
            panic("Assertion \"" #x "\" failed\n");                            \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

#define kassert_n(x, fmt, ...)                                                 \
    do {                                                                       \
        if (unlikely(!(x))) {                                                  \
            panic("Assertion \"" #x "\" failed with message: " fmt "\n",       \
                  ##__VA_ARGS__);                                              \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

#define _kassert_pick(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define kassert(...)                                                           \
    _kassert_pick(__VA_ARGS__, kassert_n, kassert_n, kassert_n, kassert_n,     \
                  kassert_n, kassert_n, kassert_n, kassert_1)(__VA_ARGS__)

#define kassert_unreachable()                                                  \
    do {                                                                       \
        kassert("unreachable");                                                \
        __builtin_unreachable();                                               \
    } while (0)
