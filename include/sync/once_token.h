/* @title: One-time atomic tokens */
#pragma once
#include <compiler/core.h>
#include <stdatomic.h>
#include <stdbool.h>

#define ONCE_TOKEN_DEFINE(type, name)                                          \
    struct {                                                                   \
        _Atomic(type) state;                                                   \
    } name

#define ONCE_TOKEN_INIT(val) {.state = ATOMIC_VAR_INIT(val)}

/* ===== typecheck ===== */
#define once_token_typecheck_internal(tok) ((void) sizeof((tok)->state))

#define once_token_typecheck_internal_type(tok, type)                          \
    ct_typecheck(type,                                                         \
                 atomic_load_explicit(&(tok)->state, memory_order_relaxed))

#define once_token_typecheck_internal_bool(tok)                                \
    once_token_typecheck_internal_type(tok, bool)

#define once_token_typecheck_internal_same(tok1, tok2)                         \
    ct_typecheck_same(                                                         \
        atomic_load_explicit(&(tok1)->state, memory_order_relaxed),            \
        atomic_load_explicit(&(tok2)->state, memory_order_relaxed))

/* ===== claim ===== */
#define once_token_claim_3(tok, from, to)                                      \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal(__ot_tok);                               \
        ct_typecheck_same(from, to);                                           \
        __typeof__(atomic_load_explicit(                                       \
            &__ot_tok->state, memory_order_relaxed)) __ot_exp = (from);        \
        __typeof__(__ot_exp) __ot_to = (to);                                   \
        atomic_compare_exchange_strong_explicit(&__ot_tok->state, &__ot_exp,   \
                                                __ot_to, memory_order_acq_rel, \
                                                memory_order_acquire);         \
    })

#define once_token_claim_1(tok)                                                \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal_bool(__ot_tok);                          \
        static_assert(_Generic((__ot_tok->state), bool: 1, default: 0),        \
                      "once_token_claim_1 argument requires a boolean token"); \
        once_token_claim_3(tok, false, true);                                  \
    })

#define once_token_claim(...) PP_CALL(once_token_claim, __VA_ARGS__)

/* ===== claimed ===== */
#define once_token_claimed_1(tok)                                              \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal_bool(__ot_tok);                          \
        static_assert(_Generic((__ot_tok->state), bool: 1, default: 0),        \
                      "once_token_claimed_1 requires a boolean token");        \
                                                                               \
        atomic_load_explicit(&__ot_tok->state, memory_order_acquire);          \
    })

#define once_token_claimed_2(tok, val)                                         \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal(__ot_tok);                               \
        __typeof__(atomic_load_explicit(                                       \
            &__ot_tok->state, memory_order_relaxed)) __ot_val = (val);         \
                                                                               \
        atomic_load_explicit(&__ot_tok->state, memory_order_acquire) ==        \
            __ot_val;                                                          \
    })

#define once_token_claimed(...) PP_CALL(once_token_claimed, __VA_ARGS__)

/* ===== reset ===== */
#define once_token_reset_1(tok)                                                \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal_bool(__ot_tok);                          \
        static_assert(_Generic((__ot_tok->state), bool: 1, default: 0),        \
                      "once_token_reset_1 requires a boolean token");          \
        atomic_store_explicit(&__ot_tok->state, false, memory_order_release);  \
    })

#define once_token_reset_2(tok, val)                                           \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal(__ot_tok);                               \
        __typeof__(atomic_load_explicit(                                       \
            &__ot_tok->state, memory_order_relaxed)) __ot_val = (val);         \
        atomic_store_explicit(&__ot_tok->state, __ot_val,                      \
                              memory_order_release);                           \
    })

#define once_token_reset(...) PP_CALL(once_token_reset, __VA_ARGS__)

#define once_token_read(tok)                                                   \
    ({                                                                         \
        __auto_type __ot_tok = (tok);                                          \
        once_token_typecheck_internal(__ot_tok);                               \
        atomic_load_explicit(&__ot_tok->state, memory_order_acquire);          \
    })
