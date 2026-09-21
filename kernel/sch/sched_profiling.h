#include <profiling.h>

struct scheduler_stats {
    atomic_uint64_t steals;
};

#ifdef PROFILING_SCHED
static struct scheduler_stats sched_stats = {0};

static inline void sched_profiling_record_steal(void) {
    atomic_inc(&sched_stats.steals);
}
#else
static inline void sched_profiling_record_steal(void) { /* Nothing */ }
#endif
