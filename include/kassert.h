/* @title: Assertions */
#include <compiler.h>
#include <compiler_intrinsics.h>
#include <console/crash.h>
#include <console/panic.h>

#define _kassert_pick(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, NAME, ...) \
    NAME

#define _kassert_dispatch(default, name, prefix, ...)                          \
    _kassert_pick(__VA_ARGS__, name##_n, name##_n, name##_n, name##_n,         \
                  name##_n, name##_n, name##_n, name##_n, name##_n, name##_2,  \
                  name##_1)(default, prefix, __VA_ARGS__)

#define _kassert_as_code(x) ct_as_type(enum crash_code, x)
#define _kassert_msg(x) "Assertion \"" #x "\" failed"

#define _kassert_debug_off_dispatch(first, ...) ({ first; })

#define _kassert_eval(x, msg_stmt)                                             \
    ({                                                                         \
        ct_decay(x) _kassert_res = (x);                                        \
        if (cc_unlikely(!(_kassert_res))) {                                    \
            msg_stmt;                                                          \
            ci_unreachable();                                                  \
        }                                                                      \
        _kassert_res;                                                          \
    })
/*
 * kassert(x)
 */
#define _kassert_1(default, prefix, x)                                         \
    _kassert_eval(x, assert_impl_default(CRASH_CODE_TO_PAYLOAD(default),       \
                                         __FILE__, __LINE__, __func__, prefix, \
                                         _kassert_msg(x), NULL))

/*
 * kassert(x, CODE)
 * kassert(x, "msg")
 */
#define _kassert_2(default, prefix, x, a)                                      \
    _kassert_eval(                                                             \
        x, ci_choose_expr(                                                     \
               ct_is_str(a),                                                   \
               assert_impl_default(CRASH_CODE_TO_PAYLOAD(default), __FILE__,   \
                                   __LINE__, __func__, prefix,                 \
                                   _kassert_msg(x), ct_as_str(a)),             \
               assert_impl_default(CRASH_CODE_TO_PAYLOAD(_kassert_as_code(a)), \
                                   __FILE__, __LINE__, __func__, prefix,       \
                                   _kassert_msg(x), NULL)))

/*
 * kassert(x, "msg %d", 12)
 * kassert(x, CODE, "msg %d", 12)
 */
#define _kassert_n(default, prefix, x, a, b, ...)                              \
    _kassert_eval(                                                             \
        x, ci_choose_expr(ct_is_str(a), /* a is format, b is first vararg */   \
                          assert_impl_default(CRASH_CODE_TO_PAYLOAD(default),  \
                                              __FILE__, __LINE__, __func__,    \
                                              prefix, _kassert_msg(x),         \
                                              ct_as_str(a), b, ##__VA_ARGS__), \
                          assert_impl_default(                                 \
                              CRASH_CODE_TO_PAYLOAD(_kassert_as_code(a)),      \
                              __FILE__, __LINE__, __func__, prefix,            \
                              _kassert_msg(x), ct_as_str(b), ##__VA_ARGS__)))

#define _kassert_fail(c, prefix, ...)                                          \
    assert_impl_default(CRASH_CODE_TO_PAYLOAD(c), __FILE__, __LINE__,          \
                        __func__, prefix, NULL, "" __VA_ARGS__)

#define kassert_with_default(default, ...)                                     \
    _kassert_dispatch(default, "", __VA_ARGS__)
#define kassert(...)                                                           \
    _kassert_dispatch(CRASH_CODE_GENERIC, _kassert, "", __VA_ARGS__)

#define kassert_with(x, payload, fmt, ...)                                     \
    ({                                                                         \
        ct_decay(x) _kassert_res = (x);                                        \
        if (cc_unlikely(!(_kassert_res))) {                                    \
            assert_impl_default(payload, __FILE__, __LINE__, __func__, "",     \
                                _kassert_msg(x), fmt, ##__VA_ARGS__);          \
            ci_unreachable();                                                  \
        }                                                                      \
        _kassert_res;                                                          \
    })

#define unreachable(...)                                                       \
    _kassert_fail(CRASH_CODE_GENERIC, "unreachable! ", ##__VA_ARGS__)
#define unimplemented(...)                                                     \
    _kassert_fail(CRASH_CODE_GENERIC, "unimplemented! ", ##__VA_ARGS__)
#define todo(...) _kassert_fail(CRASH_CODE_GENERIC, "TODO: ", ##__VA_ARGS__)

#ifdef DEBUG_ASSERT

#define kassert_debug_with_default(default, ...)                               \
    _kassert_dispatch(default, _kassert, "DEBUG ", __VA_ARGS__)

#define kassert_debug(...)                                                     \
    _kassert_dispatch(CRASH_CODE_GENERIC, _kassert, "DEBUG ", __VA_ARGS__)

#define unreachable_debug(...)                                                 \
    _kassert_fail(CRASH_CODE_GENERIC, "DEBUG unreachable! ", ##__VA_ARGS__)
#define unimplemented_debug(...)                                               \
    _kassert_fail(CRASH_CODE_GENERIC, "DEBUG unimplemented! ", ##__VA_ARGS__)
#define todo_debug(...)                                                        \
    _kassert_fail(CRASH_CODE_GENERIC, "DEBUG TODO: ", ##__VA_ARGS__)

#else

#define kassert_debug(...) _kassert_debug_off_dispatch(__VA_ARGS__)
#define unreachable_debug(...) _kassert_debug_off_dispatch(__VA_ARGS__)
#define unimplemented_debug(...) _kassert_debug_off_dispatch(__VA_ARGS__)
#define todo_debug(...) _kassert_debug_off_dispatch(__VA_ARGS__)

#endif
