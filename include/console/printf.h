#pragma once
#include <colors.h>
#include <sch/irql.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

struct printf_cursor;
struct limine_framebuffer;

void printf(const char *format, ...);
void vprintf(struct printf_cursor *csr, const char *format, va_list args);
void serial_init();
void serial_write(const char *str, size_t len);
bool serial_try_getc(char *out);
#include <sync/lock_general.h>
#include <sync/spinlock.h>

extern struct spinlock k_printf_lock;

void printf_init(struct limine_framebuffer *fb);
void printf_unlocked(const char *format, ...);
void printf_unlock(enum irql i) TSA_RELEASES(&k_printf_lock);
enum irql printf_lock() TSA_ACQUIRES(&k_printf_lock);
