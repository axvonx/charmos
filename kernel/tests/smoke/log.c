#include "tests/test_internal.h"

TEST_GROUP_DECLARE(log, .intensity_desc = {
                            .curve = SCALE_PIECEWISE_LOG,
                            .unit = "iters",
                        });

static void log_event(const char *msg) {
    cc_unused(msg);
}

TEST_DECLARE_SMOKE(log, emit_event) {
    log_event("smoke test message");
    return TEST_SUCCESS;
}
