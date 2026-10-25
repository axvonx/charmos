/* @title: Timekeeper */
#pragma once

struct clock;

void timekeeper_init(void);
void timekeeper_set_clock(struct clock *clk);
