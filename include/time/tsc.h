/* @title: TSC */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <types/types.h>

struct tsc_globals {
    bool use_tsc;
    int64_t max_warp;
    uint64_t min_rtt;
};

extern struct tsc_globals tsc_global;

struct clock *tsc_clock_init(freq_hz_t freq_hz);
freq_hz_t tsc_calibrate_hpet(void);
bool tsc_sync_check_bsp(cpu_id_t ap_cpu);
void tsc_sync_check_ap(cpu_id_t self);
void tsc_sync_check_all_aps(void);
bool tsc_should_use_tsc(void);
bool tsc_has_invariant(void);
uint64_t tsc_clock_read(struct clock *clk);
