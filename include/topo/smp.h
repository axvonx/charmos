/* @title: SMP initialization */
#pragma once
#include <limine.h>
#include <types/types.h>

struct core;
void smp_init(void);
void smp_timer_init(void);
void smp_wake(struct limine_mp_response *mpr);
void smp_setup_bsp(struct limine_mp_response *mpr);
void smp_wait_for_others_to_idle(void);
void topology_init(void);
void smp_disable_all_ticks(void);
void smp_enable_all_ticks(void);
void smp_boot_fence_raise(void);
void smp_boot_fence_lower(void);
void smp_boot_fence_park(void);
void smp_dump_core(struct core *c);
void smp_move_everyone(void);
