#pragma once
#include <compiler/core.h>
#include <compiler/wrapper.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
void *memcpy(void *dest, const void *src, size_t n) cc_access(write_only, 1, 3)
cc_access(read_only, 2, 3) cc_nonnull(1, 2) cc_returns_nonnull;

void *memset(void *s, int c, size_t n) cc_access(write_only, 1, 3) cc_nonnull(1)
cc_returns_nonnull;

void *memmove(void *dest, const void *src, size_t n) cc_access(write_only, 1, 3)
cc_access(read_only, 2, 3) cc_nonnull(1, 2) cc_returns_nonnull;

int memcmp(const void *s1, const void *s2, size_t n) cc_access(read_only, 1, 3)
cc_access(read_only, 2, 3) cc_nonnull(1, 2) cc_pure;

void *memchr(const void *s, int c, size_t n) cc_access(read_only, 1, 3)
cc_nonnull(1) cc_pure;

void *memrchr(const void *s, int c, size_t n) cc_access(read_only, 1, 3)
cc_nonnull(1) cc_pure;

void *memmem(const void *haystack, size_t haystack_len, const void *needle,
             size_t needle_len) cc_access(read_only, 1, 2)
cc_access(read_only, 3, 4) cc_nonnull(1, 3) cc_pure;

void *mempcpy(void *dest, const void *src, size_t n) cc_access(write_only, 1, 3)
cc_access(read_only, 2, 3) cc_nonnull(1, 2) cc_returns_nonnull;

#define __STRING_FAST static inline __attribute__((always_inline, artificial))

__STRING_FAST void *__memset_inline(void *s, int c, size_t n) {
    void *ret = s;
    uint64_t word = (uint64_t) (uint8_t) c * 0x0101010101010101ULL;
    size_t qwords = n >> 3;
    size_t rem = n & 7;

    asm volatile("rep stosq" : "+D"(s), "+c"(qwords) : "a"(word) : "memory");
    asm volatile("rep stosb" : "+D"(s), "+c"(rem) : "a"(word) : "memory");

    return ret;
}

__STRING_FAST void *__memcpy_inline(void *dest, const void *src, size_t n) {
    void *ret = dest;
    size_t qwords = n >> 3;
    size_t rem = n & 7;

    asm volatile("rep movsq"
                 : "+D"(dest), "+S"(src), "+c"(qwords)
                 :
                 : "memory");
    asm volatile("rep movsb" : "+D"(dest), "+S"(src), "+c"(rem) : : "memory");

    return ret;
}

__STRING_FAST void *__memmove_inline(void *dest, const void *src, size_t n) {
    if ((uintptr_t) dest - (uintptr_t) src >= n)
        return __memcpy_inline(dest, src, n);

    uint8_t *d = (uint8_t *) dest + n;
    const uint8_t *s = (const uint8_t *) src + n;
    while (n--)
        *--d = *--s;

    return dest;
}

__STRING_FAST int __memcmp_inline(const void *s1, const void *s2, size_t n) {
    if (!n)
        return 0;

    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    asm volatile("repe cmpsb" : "+D"(p1), "+S"(p2), "+c"(n) : : "memory", "cc");

    return (int) p1[-1] - (int) p2[-1];
}

/*
 * Fortify: the primitives below are inline asm, so the compiler can't see
 * what they touch. These checks hand it the object sizes instead, via
 * __builtin_dynamic_object_size, which sees through cw_alloc's alloc_size,
 * local arrays and struct members.
 *
 *   size provably too big     -> compile error (__fortify_overflow)
 *   size known only at runtime -> one compare, panics on overflow
 *   object size unknown        -> SIZE_MAX, the check folds away
 */
cc_noreturn void fortify_panic(const char *fn, size_t want, size_t have);
void __fortify_overflow(void)
    cc_error("fortify: size provably exceeds the object it accesses");

#define _fortify_obj(p) __builtin_dynamic_object_size((p), 0)

#define _fortify_check(fn, p, n)                                               \
    ({                                                                         \
        size_t _fortify_have_ = _fortify_obj(p);                               \
        size_t _fortify_want_ = (n);                                           \
        if (__builtin_constant_p(_fortify_want_ > _fortify_have_) &&           \
            _fortify_want_ > _fortify_have_)                                   \
            __fortify_overflow();                                              \
        else if (cc_unlikely(_fortify_want_ > _fortify_have_))                 \
            fortify_panic(fn, _fortify_want_, _fortify_have_);                 \
    })

/* Only pays for the strlen() when the destination's size is known */
#define _fortify_str_check(fn, d, need_expr)                                   \
    ({                                                                         \
        if (_fortify_obj(d) != SIZE_MAX)                                       \
            _fortify_check(fn, d, (need_expr));                                \
    })

#define memset(s, c, n)                                                        \
    ({                                                                         \
        size_t _memset_n_ = (n);                                               \
        _fortify_check("memset", s, _memset_n_);                               \
        __memset_inline((s), (c), _memset_n_);                                 \
    })
#define memcpy(d, s, n)                                                        \
    ({                                                                         \
        size_t _memcpy_n_ = (n);                                               \
        _fortify_check("memcpy", d, _memcpy_n_);                               \
        _fortify_check("memcpy", s, _memcpy_n_);                               \
        __memcpy_inline((d), (s), _memcpy_n_);                                 \
    })
#define memmove(d, s, n)                                                       \
    ({                                                                         \
        size_t _memmove_n_ = (n);                                              \
        _fortify_check("memmove", d, _memmove_n_);                             \
        _fortify_check("memmove", s, _memmove_n_);                             \
        __memmove_inline((d), (s), _memmove_n_);                               \
    })
#define memcmp(a, b, n)                                                        \
    ({                                                                         \
        size_t _memcmp_n_ = (n);                                               \
        _fortify_check("memcmp", a, _memcmp_n_);                               \
        _fortify_check("memcmp", b, _memcmp_n_);                               \
        __memcmp_inline((a), (b), _memcmp_n_);                                 \
    })

size_t strlen(const char *str) cc_pure cc_nonnull(1);
char *strcpy(char *dest, const char *src) cc_nonnull(1, 2) cc_returns_nonnull;
/* Return the terminator */
char *stpcpy(char *dest, const char *src) cc_nonnull(1, 2) cc_returns_nonnull;
/* Pad to n bytes and return the first written NUL, or dest + n if truncated */
char *stpncpy(char *dest, const char *src, size_t n) cc_nonnull(1, 2)
cc_returns_nonnull;
char *strcat(char *dest, const char *src) cc_nonnull(1, 2) cc_returns_nonnull;
int strncmp(const char *s1, const char *s2, size_t n) cc_pure cc_nonnull(1, 2);
int strcmp(const char *str1, const char *str2) cc_pure cc_nonnull(1, 2);
char *strncpy(char *dest, const char *src, size_t n) cc_nonnull(1, 2)
cc_returns_nonnull;
char *strchr(const char *s, int c) cc_pure cc_nonnull(1);
char *strrchr(const char *s, int c) cc_pure cc_nonnull(1);
size_t strspn(const char *s, const char *accept) cc_pure cc_nonnull(1, 2);
size_t strcspn(const char *s, const char *reject) cc_pure cc_nonnull(1, 2);
char *strpbrk(const char *s, const char *accept) cc_pure cc_nonnull(1, 2);
char *strstr(const char *haystack, const char *needle) cc_pure cc_nonnull(1, 2);
char *strcasestr(const char *haystack, const char *needle) cc_pure
cc_nonnull(1, 2);
char *strnstr(const char *haystack, const char *needle, size_t len) cc_pure
cc_nonnull(1, 2);
char *strncasestr(const char *haystack, const char *needle, size_t len) cc_pure
cc_nonnull(1, 2);

char *strtok(char *str, const char *delim) cc_nonnull(2);
char *strtok_r(char *str, const char *delim, char **saveptr) cc_nonnull(2, 3);

char *strncat(char *dest, const char *src, size_t n) cc_nonnull(1, 2)
cc_returns_nonnull;
size_t strnlen(const char *s, size_t maxlen) cc_pure cc_nonnull(1);

int islower(int c) cc_constfn;
int isupper(int c) cc_constfn;
int isdigit(int c) cc_constfn;
int isalpha(int c) cc_constfn;
int isalnum(int c) cc_constfn;
int isspace(int c) cc_constfn;
int isprint(int c) cc_constfn;
int isblank(int c) cc_constfn;
int iscntrl(int c) cc_constfn;
int isgraph(int c) cc_constfn;
int ispunct(int c) cc_constfn;
int isxdigit(int c) cc_constfn;
int toupper(int c) cc_constfn;
int tolower(int c) cc_constfn;

/* This is rather funny, but clang does string matching on
 * string.h functionality and flags a vfree as a mismatch */
#ifdef __clang_analyzer__
#define strdup vstrdup
#define strndup vstrndup
#endif

char *cc_nullable strdup(const char *str) cw_alloc() cc_warn_unused_result;
char *cc_nullable strndup(const char *str, size_t n) cw_alloc()
    cc_warn_unused_result;
int strcasecmp(const char *s1, const char *s2) cc_pure cc_nonnull(1, 2);
int strncasecmp(const char *s1, const char *s2, size_t n) cc_pure
cc_nonnull(1, 2);

char *strrev(char *s) cc_nonnull(1) cc_returns_nonnull;
char *strtoupper(char *s) cc_nonnull(1) cc_returns_nonnull;
char *strtolower(char *s) cc_nonnull(1) cc_returns_nonnull;

int64_t atoi(const char *str) cc_pure cc_nonnull(1);
size_t atoui(const char *str) cc_pure cc_nonnull(1);
size_t atohex(const char *str) cc_pure cc_nonnull(1);

char *itoa(int64_t value, char *buf, int base) cc_nonnull(2) cc_returns_nonnull;
char *utoa(size_t value, char *buf, int base) cc_nonnull(2) cc_returns_nonnull;

long strtol(const char *nptr, char **endptr, int base) cc_nonnull(1);
long long strtoll(const char *nptr, char **endptr, int base) cc_nonnull(1);
unsigned long strtoul(const char *nptr, char **endptr, int base) cc_nonnull(1);
unsigned long long strtoull(const char *nptr, char **endptr, int base)
    cc_nonnull(1);

size_t strlcpy(char *dst, const char *src, size_t size) cc_nonnull(1, 2);
size_t strlcat(char *dst, const char *src, size_t size) cc_nonnull(1, 2);
char *strsep(char **stringp, const char *delim) cc_nonnull(1, 2);
char *strchrnul(const char *s, int c) cc_pure cc_nonnull(1);

int vsnprintf(char *buffer, int buffer_len, const char *format, va_list args)
    cc_printf_like(3, 0) cc_nonnull(3);
int snprintf(char *buffer, int buffer_len, const char *format, ...)
    cc_printf_like(3, 4) cc_nonnull(3);
int vasprintf(char **strp, const char *fmt, va_list args) cc_printf_like(2, 0);

#define strcpy(d, s)                                                           \
    ({                                                                         \
        _fortify_str_check("strcpy", d, __builtin_strlen(s) + 1);              \
        (strcpy)((d), (s));                                                    \
    })
#define stpcpy(d, s)                                                           \
    ({                                                                         \
        _fortify_str_check("stpcpy", d, __builtin_strlen(s) + 1);              \
        (stpcpy)((d), (s));                                                    \
    })
#define strcat(d, s)                                                           \
    ({                                                                         \
        _fortify_str_check("strcat", d,                                        \
                           __builtin_strlen(d) + __builtin_strlen(s) + 1);     \
        (strcat)((d), (s));                                                    \
    })
#define strncpy(d, s, n)                                                       \
    ({                                                                         \
        size_t _strncpy_n_ = (n);                                              \
        _fortify_check("strncpy", d, _strncpy_n_);                             \
        (strncpy)((d), (s), _strncpy_n_);                                      \
    })
#define stpncpy(d, s, n)                                                       \
    ({                                                                         \
        size_t _stpncpy_n_ = (n);                                              \
        _fortify_check("stpncpy", d, _stpncpy_n_);                             \
        (stpncpy)((d), (s), _stpncpy_n_);                                      \
    })
#define strlcpy(d, s, n)                                                       \
    ({                                                                         \
        size_t _strlcpy_n_ = (n);                                              \
        _fortify_check("strlcpy", d, _strlcpy_n_);                             \
        (strlcpy)((d), (s), _strlcpy_n_);                                      \
    })
#define strlcat(d, s, n)                                                       \
    ({                                                                         \
        size_t _strlcat_n_ = (n);                                              \
        _fortify_check("strlcat", d, _strlcat_n_);                             \
        (strlcat)((d), (s), _strlcat_n_);                                      \
    })
#define snprintf(buf, len, ...)                                                \
    ({                                                                         \
        int _snprintf_len_ = (len);                                            \
        if (_snprintf_len_ > 0)                                                \
            _fortify_check("snprintf", buf, (size_t) _snprintf_len_);          \
        (snprintf)((buf), _snprintf_len_, __VA_ARGS__);                        \
    })
#define vsnprintf(buf, len, fmt, args)                                         \
    ({                                                                         \
        int _vsnprintf_len_ = (len);                                           \
        if (_vsnprintf_len_ > 0)                                               \
            _fortify_check("vsnprintf", buf, (size_t) _vsnprintf_len_);        \
        (vsnprintf)((buf), _vsnprintf_len_, (fmt), (args));                    \
    })
int asprintf(char **strp, const char *fmt, ...) cc_printf_like(2, 3);
