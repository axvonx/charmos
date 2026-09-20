#include "internal.h"

#include <nightmare/record.h>
#include <sch/sched.h>

#ifdef TEST_NIGHTMARE_ENABLED
struct test_liveness_state nightmare_liveness;

bool nightmare_liveness_start(time_ms_t threshold_ms,
                              enum test_on_stall policy) {
    return test_liveness_start(&nightmare_liveness, &nightmare_runtime.conc,
                               threshold_ms, policy);
}

void nightmare_liveness_stop(void) {
    test_liveness_stop(&nightmare_liveness);
}

void nightmare_report_stall(const struct test_stall_evidence *evidence) {
    if (!evidence)
        return;
    NIGHTMARE_FINDING_TIER("stall", NIGHTMARE_TIER_AMBIGUOUS, 0,
                           "silent_ms=%lu progress=%lu",
                           (unsigned long) evidence->silent_ms,
                           (unsigned long) evidence->progress);
    nightmare_publish_stop(TEST_STOP_STALL);
}

void nightmare_liveness_poll(void) {
    struct test_stall_evidence evidence;
    if (test_liveness_take(&nightmare_liveness, &evidence)) {
        nightmare_report_stall(&evidence);
        if (nightmare_liveness.policy != TEST_ON_STALL_REPORT)
            nightmare_panic("aggregate nightmare progress stalled");
    }

    if (nightmare_runtime.ctx.nm && nightmare_runtime.ctx.nm->ops &&
        nightmare_runtime.ctx.nm->ops->probe) {
        nightmare_runtime.ctx.nm->ops->probe(&nightmare_runtime.ctx);
    }
}
#endif
