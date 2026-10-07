/* @title: LAPIC Timer */
#pragma once
#include <stdbool.h>
#include <types/types.h>

void lapic_timer_init(cpu_id_t core_id);
void lapic_timer_init_bsp(void);
void lapic_timer_disable(void);
void lapic_timer_enable(void);
bool lapic_timer_is_enabled(void);
void lapic_clock_evdev_group_init(void);
