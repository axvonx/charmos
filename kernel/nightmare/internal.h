#pragma once
#include <console/crash.h>
#include <nightmare/nightmare.h>
#include <nightmare/perturb.h>
#include <stdatomic.h>
#include <sync/completion.h>
#include <test/conc.h>
#include <thread/queue.h>
#include <time/timer.h>
#include <watchdog.h>

#define nightmare_panic(fmt, ...)                                              \
    do {                                                                       \
        char _nm_msg[CRASH_MSG_MAX];                                           \
        snprintf(_nm_msg, sizeof(_nm_msg), fmt, ##__VA_ARGS__);                \
        crash_full(&(struct crash_context) {                                   \
            .source = CRASH_SOURCE_NIGHTMARE,                                  \
            .formats = CRASH_FMT_DEFAULT,                                      \
            .file = __FILE__,                                                  \
            .line = __LINE__,                                                  \
            .func = __func__,                                                  \
            .msg = _nm_msg,                                                    \
        });                                                                    \
    } while (0)

struct nightmare_cmdline_config {
    const char *selector;
    fx32_32_t intensity;
    uint64_t seed;
    bool seed_present;
    enum nightmare_seed_mode seed_mode;
    time_ms_t duration_ms;
    time_ms_t drain_grace_ms;
    time_ms_t stat_interval_ms;
    time_ms_t stall_threshold_ms;
    enum test_on_stall on_stall;
    uint64_t boot_index;
    const char *campaign_id;
    struct cmdline_list perturb;
    bool perturb_present;
};

#define NIGHTMARE_MAX_PERTURBERS 8

struct nightmare_runtime {
    struct nightmare_ctx ctx;

    struct test_conc conc;

    atomic_size_t finding_count;
    atomic_bool terminal;
    atomic_bool perturb_verdict_ready;
    struct nightmare_verdict perturb_verdict;
    char perturb_reason[64];
    char perturb_msg[256];
    size_t perturber_count;
    const struct nightmare_perturb_desc *perturbers[NIGHTMARE_MAX_PERTURBERS];
    struct timer soft_timer;
    struct timer hard_timer;
    time_ms_t started_ms;
    time_ms_t stat_interval_ms;
    const char *campaign_id;
    uint64_t boot_index;
    char caps[256];
};

extern struct nightmare_runtime nightmare_runtime;
extern struct test_liveness_state nightmare_liveness;

void nightmare_publish_stop(enum test_stop reason);
void nightmare_cmdline_get(struct nightmare_cmdline_config *config);
void nightmare_thread_main(void *arg);
void nightmare_heartbeat_main(void *arg);
void nightmare_publish_perturb_verdict(struct nightmare_verdict verdict);
bool nightmare_load_perturb_verdict(struct nightmare_verdict *out);
enum nightmare_result
nightmare_result_with_findings(enum nightmare_result result, size_t findings);
struct nightmare_verdict
nightmare_verdict_for_stop(struct nightmare_verdict verdict,
                           enum test_stop stop);
const char *nightmare_result_to_str(enum nightmare_result result);
const char *nightmare_skip_to_str(enum nightmare_skip_reason reason);
const char *nightmare_seed_policy_to_str(enum nightmare_seed_policy policy);
const char *nightmare_seed_mode_to_str(enum nightmare_seed_mode mode);
bool nightmare_liveness_start(time_ms_t threshold_ms,
                              enum test_on_stall policy);
void nightmare_liveness_stop(void);
void nightmare_liveness_poll(void);
