/* @title: Enforcement */
#include <compiler/core.h>
#include <compiler/intrinsic.h>
#include <console/crash.h>
#include <console/panic.h>

#define _enforce_msg(x) "Enforcement \"" #x "\" failed"

#define _enforce_eval(x, msg_stmt)                                             \
    _enforce_eval_as(x, msg_stmt, PP_CONCAT(_enforce_res_, __COUNTER__))

#define _enforce_eval_as(x, msg_stmt, res_)                                    \
    ({                                                                         \
        ct_decay(x) res_ = (x);                                                \
        if (cc_unlikely(!(res_))) {                                            \
            msg_stmt;                                                          \
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

#define _enforce_ctx(prefix, x)                                                \
    __FILE__, __LINE__, __func__, prefix, _enforce_msg(x)

/* enforce(x) */
#define _enforce_0(prefix, x)                                                  \
    _enforce_eval(x, enforce_impl_code_only(_enforce_ctx(prefix, x),           \
                                            CRASH_CODE_GENERIC))

/*
 * enforce(x, CODE)
 * enforce(x, "msg")
 */
#define _enforce_1(prefix, x, a)                                               \
    _enforce_eval(                                                             \
        x, _enforce_fn(a, enforce_impl_code_only)(_enforce_ctx(prefix, x), a))

/*
 * enforce(x, "msg %d", 12)
 * enforce(x, CODE, "msg %d", 12)
 */
#define _enforce_n(prefix, x, a, ...)                                          \
    _enforce_eval(x, _enforce_fn(a, enforce_impl_code)(                        \
                         _enforce_ctx(prefix, x), a, __VA_ARGS__))

#define _enforce_fail(prefix, ...)                                             \
    enforce_impl_msg(__FILE__, __LINE__, __func__, prefix, NULL, "" __VA_ARGS__)

#define enforce(x, ...) _enforce_arity(__VA_ARGS__)("", x, ##__VA_ARGS__)

#define unimplemented(...) _enforce_fail("unimplemented! ", ##__VA_ARGS__)

#define todo(...) _enforce_fail("TODO: ", ##__VA_ARGS__)

#define unreachable(...) _enforce_fail("unreachable! ", ##__VA_ARGS__)

/*
 * assume(x)
 * assume(x, CODE)
 * assume(x, "msg %d", 12)
 * assume(x, CODE, "msg %d", 12)
 *
 * enforce() is a runtime check: "I'm not sure this holds, verify it". assume()
 * is an unreachability hint: "the path where x is false does not exist". It
 * is only checked under DEBUG_ASSUME, as a test of that claim. Without it, x
 * is never evaluated and a false x is UB that the optimizer may propagate
 * anywhere, including deleting checks before and after it. Anything
 * that depends on input, hardware, timing, or another subsystem's correctness
 * belongs in enforce()
 *
 * x must be free of side effects: no ++/=, no atomic or volatile accesses,
 * and no calls to functions that aren't cc_pure/cc_constfn. Clang rejects
 * violations in both modes (-Werror=assume) via the unevaluated cc_assume
 * below, and a constant-false x (assume(0)) is an error in favor of
 * unreachable(). Evaluates to nothing either way
 */
/* Never defined: only ever named inside sizeof, the call is not emitted */
void _assume_not_constant_false(bool cond)
    cc_diagnose_if(!cond,
                   "assume() on a constant-false condition; use unreachable() "
                   "for a dead path or static_assert() for a constant",
                   "error");

#define _assume_static_checks(x)                                               \
    ((void) sizeof(({                                                          \
        cc_assume(x);                                                          \
        _assume_not_constant_false(x);                                         \
        0;                                                                     \
    })))

#ifdef DEBUG_ASSUME

#define assume(x, ...)                                                         \
    ({                                                                         \
        _assume_static_checks(x);                                              \
        (void) _enforce_arity(__VA_ARGS__)("assume! ", x, ##__VA_ARGS__);      \
    })

#else

#define assume(x, ...)                                                         \
    ({                                                                         \
        _assume_static_checks(x);                                              \
        cc_assume(x);                                                          \
    })

#endif
