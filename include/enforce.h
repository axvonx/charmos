/* @title: Enforcement */
#include <compiler/core.h>
#include <compiler/intrinsic.h>
#include <console/crash.h>
#include <console/panic.h>

#define _enforce_msg(s) "Enforcement \"" s "\" failed"

#define _enforce_eval(x, ...)                                                  \
    _enforce_eval_as(x, PP_CONCAT(_enforce_res_, __COUNTER__), __VA_ARGS__)

#define _enforce_eval_as(x, res_, ...)                                         \
    ({                                                                         \
        ct_decay(x) res_ = (x);                                                \
        if (cc_unlikely(!(res_))) {                                            \
            __VA_ARGS__;                                                       \
            ci_unreachable();                                                  \
        }                                                                      \
        res_;                                                                  \
    })

#define _enforce_pick(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12,   \
                      _13, _14, _15, _16, NAME, ...)                           \
    NAME

#define _enforce_arity(...)                                                    \
    _enforce_pick(_, ##__VA_ARGS__, _enforce_n, _enforce_n, _enforce_n,        \
                  _enforce_n, _enforce_n, _enforce_n, _enforce_n, _enforce_n,  \
                  _enforce_n, _enforce_n, _enforce_n, _enforce_n, _enforce_n,  \
                  _enforce_n, _enforce_n, _enforce_1, _enforce_0)

#define _enforce_fn(a, code_fn)                                                \
    cc_generic((a), char * : enforce_impl_msg,                                 \
               const char * : enforce_impl_msg, default : code_fn)

#define _enforce_ctx(prefix, s)                                                \
    __FILE__, __LINE__, __func__, prefix, _enforce_msg(s)

/* enforce(x) */
#define _enforce_0(prefix, s, x)                                               \
    _enforce_eval(x, enforce_impl_code_only(_enforce_ctx(prefix, s), NULL))

/*
 * enforce(x, "msg")
 * enforce(x, INVALID_ARGUMENT)
 * enforce(x, (asan, POISONED))
 *
 * Code forms are documented at CRASH_CODE_ARG in console/crash.h
 */
#define _enforce_1(prefix, s, x, a)                                            \
    _enforce_eval(                                                             \
        x, CRASH_CODE_SCOPED(                                                  \
               a, _enforce_fn(CRASH_CODE_ARG(a), enforce_impl_code_only)(      \
                      _enforce_ctx(prefix, s), CRASH_CODE_ARG(a))))

/*
 * enforce(x, "msg %d", 12)
 * enforce(x, INVALID_ARGUMENT, "msg %d", 12)
 * enforce(x, (asan, POISONED), "msg %d", 12)
 */
#define _enforce_n(prefix, s, x, a, ...)                                       \
    _enforce_eval(x, CRASH_CODE_SCOPED(                                        \
                         a, _enforce_fn(CRASH_CODE_ARG(a), enforce_impl_code)( \
                                _enforce_ctx(prefix, s), CRASH_CODE_ARG(a),    \
                                __VA_ARGS__)))

#define _enforce_fail(prefix, ...)                                             \
    enforce_impl_msg(__FILE__, __LINE__, __func__, prefix, NULL, "" __VA_ARGS__)

#define enforce(x, ...) _enforce_arity(__VA_ARGS__)("", #x, x, ##__VA_ARGS__)

#define unimplemented(...) _enforce_fail("unimplemented! ", ##__VA_ARGS__)

#define todo(...) _enforce_fail("TODO: ", ##__VA_ARGS__)

#define unreachable(...)                                                       \
    enforce_impl_code(__FILE__, __LINE__, __func__, "unreachable! ", NULL,     \
                      CRASH_CODE(global, UNREACHABLE), "" __VA_ARGS__)

/*
 * assume(x)
 * assume(x, "msg %d", 12)
 *
 * No crash codes for assume()
 */
void _assume_not_constant_false(bool cond)
    cc_diagnose_if(!cond, "assume() on a constant false, use unreachable()",
                   "error");

#define _assume_static_checks(x)                                               \
    ((void) sizeof(({                                                          \
        cc_assume(x);                                                          \
        _assume_not_constant_false(x);                                         \
        0;                                                                     \
    })))

/* Only used in sizeof for a format check, not a real function anywhere */
int _assume_fmt(const char *fmt, ...) cc_printf_like(1, 2);

#define _assume_msg_only(...)                                                  \
    static_assert(!PP_IS_PAREN(PP_GET_1(__VA_ARGS__, ~)),                      \
                  "assume() takes no crash code");                             \
    (void) sizeof(_assume_fmt("" __VA_ARGS__))

#ifdef DEBUG_ASSUME

#define assume(x, ...)                                                         \
    ({                                                                         \
        _assume_static_checks(x);                                              \
        _assume_msg_only(__VA_ARGS__);                                         \
        if (cc_unlikely(!(x)))                                                 \
            enforce_impl_msg(_enforce_ctx("assume! ", #x), "" __VA_ARGS__);    \
    })

#else

#define assume(x, ...)                                                         \
    ({                                                                         \
        _assume_static_checks(x);                                              \
        _assume_msg_only(__VA_ARGS__);                                         \
        cc_assume(x);                                                          \
    })

#endif
