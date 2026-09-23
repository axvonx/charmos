/* @title: HyperLogLog */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

struct hyperloglog;

#define HYPERLOGLOG_REGISTER_WIDTH_DEFAULT 4

struct hyperloglog *hyperloglog_create(uint8_t register_width);
void hyperloglog_destroy(struct hyperloglog *hll);
fx32_32_t hyperloglog_estimate(struct hyperloglog *hll);
void hyperloglog_add(struct hyperloglog *hll, uint32_t hash);
