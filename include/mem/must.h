/* @title: Boot OOM */
#pragma once
#include <console/panic.h>
#include <global.h>
#include <log.h>

#define must(expr)                                                             \
    ({                                                                         \
        if (global.current_bootstage >= BOOTSTAGE_COMPLETE)                    \
            log_warn_once("must() invoked after boot");                        \
        __typeof__(expr) _p_ = (expr);                                         \
        if (cc_unlikely(!_p_))                                                 \
            panic("OOM: %s == NULL, bootstage: %s", #expr,                     \
                  bootstage_str[global.current_bootstage]);                    \
        _p_;                                                                   \
    })

#define must_kmalloc(...) must(kmalloc(__VA_ARGS__))
#define must_krealloc(...) must(krealloc(__VA_ARGS__))
