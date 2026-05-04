/* @title: DMAR */
#pragma once
#include <console/panic.h>
#include <console/printf.h>
#include <log.h>
#include <math/bit.h>
#include <uacpi/tables.h>

LOG_HANDLE_EXTERN(dmar);
LOG_SITE_EXTERN(dmar);
#define dmar_log(lvl, fmt, ...)                                                \
    log(LOG_SITE(dmar), LOG_HANDLE(dmar), lvl, fmt, ##__VA_ARGS__)

#define dmar_err(fmt, ...) dmar_log(LOG_ERROR, fmt, ##__VA_ARGS__)
#define dmar_warn(fmt, ...) dmar_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define dmar_info(fmt, ...) dmar_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define dmar_debug(fmt, ...) dmar_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define dmar_trace(fmt, ...) dmar_log(LOG_TRACE, fmt, ##__VA_ARGS__)

bool dmar_init();
