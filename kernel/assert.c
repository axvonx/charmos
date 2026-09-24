#include <console/crash.h>
#include <console/printf.h>
#include <stdarg.h>
#include <string.h>

static cc_noreturn void impl_default(struct crash_payload pluh,
                                     const char *file, int line,
                                     const char *func, const char *prefix,
                                     const char *fmt, va_list args) {
    cc_unused(pluh);
    static char msg[CRASH_MSG_MAX];

    int n = 0;
    if (prefix && strlen(prefix) > 0) {
        n = snprintf(msg, sizeof(msg), "%s", prefix);
        if (n < 0)
            n = 0;
        if (n > (int) sizeof(msg) - 1)
            n = (int) sizeof(msg) - 1;
    }

    if (fmt && strlen(fmt) > 0)
        vsnprintf(msg + n, sizeof(msg) - n, fmt, args);

    crash_full(&(struct crash_context){
        .payload = pluh,
        .source = CRASH_SOURCE_ASSERT,
        .formats = CRASH_FMT_DEFAULT,
        .file = file,
        .line = line,
        .func = func,
        .msg = msg,
    });
}

static cc_noreturn void impl_assertion(struct crash_payload pluh,
                                       const char *file, int line,
                                       const char *func, const char *prefix,
                                       const char *assertion, const char *fmt,
                                       va_list args) {
    cc_unused(pluh);
    static char msg[CRASH_MSG_MAX];

    int n = snprintf(msg, sizeof(msg), "%s%s", prefix ? prefix : "",
                     assertion ? assertion : "");

    if (n < 0)
        n = 0;
    if (n > (int) sizeof(msg) - 1)
        n = (int) sizeof(msg) - 1;

    if (fmt && strlen(fmt) > 0) {
        n += snprintf(msg + n, (int) sizeof(msg) - n, ": ");
        if (n > (int) sizeof(msg) - 1)
            n = (int) sizeof(msg) - 1;

        vsnprintf(msg + n, (int) sizeof(msg) - n, fmt, args);
    }

    crash_full(&(struct crash_context){
        .payload = pluh,
        .source = CRASH_SOURCE_ASSERT,
        .formats = CRASH_FMT_DEFAULT,
        .file = file,
        .line = line,
        .func = func,
        .msg = msg,
    });
}

cc_noreturn void assert_impl_default(struct crash_payload pluh,
                                     const char *file, int line,
                                     const char *func, const char *prefix,
                                     const char *assertion, const char *fmt,
                                     ...) {
    va_list args;
    va_start(args, fmt);

    if (assertion && strlen(assertion) > 0) {
        impl_assertion(pluh, file, line, func, prefix, assertion, fmt, args);
    } else {
        impl_default(pluh, file, line, func, prefix, fmt, args);
    }

    va_end(args);
}
