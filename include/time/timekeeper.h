/* @title: Timekeeper */
#pragma once
#include <compiler/core.h>

struct clock;

void timekeeper_init(void);
void timekeeper_set_clock(struct clock *cc_nonnull_ptr clk);
