/* @title: Debugging */
#include <stddef.h>
#include <stdint.h>

void debug_print_registers();

void debug_print_stack();

void debug_print_memory(void *addr, size_t size);
void debug_print_stack_from(uint64_t *start, size_t max_scan);
#pragma once
