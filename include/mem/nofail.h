/* @title: Boot OOM */
#pragma once
#include <boot/global.h>
#include <console/panic.h>
#include <log.h>
#include <mem/alloc.h>

#define nofail(expr)                                                           \
    ({                                                                         \
        if (boot_global.stage >= BOOTSTAGE_COMPLETE)                           \
            log_msg_once(LOG_WARN, "nofail() invoked after boot");             \
        typeof(expr) _p_ = (expr);                                             \
        if (cc_unlikely(!_p_))                                                 \
            panic(OUT_OF_MEMORY, "%s == NULL, bootstage: %s", #expr,           \
                  bootstage_str[boot_global.stage]);                           \
        _p_;                                                                   \
    })

#define nofail_valloc(...) nofail(valloc(__VA_ARGS__))
#define nofail_vnew(...) nofail(vnew(__VA_ARGS__))
#define nofail_vnews(...) nofail(vnews(__VA_ARGS__))
#define nofail_vnew_flex(...) nofail(vnew_flex(__VA_ARGS__))
#define nofail_vrealloc(...) nofail(vrealloc(__VA_ARGS__))
#define nofail_vrenews(...) nofail(vrenews(__VA_ARGS__))
