/* @title: MMIO */
#pragma once
#include <compiler/core.h>
#include <mem/page.h>
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

#define MMIO_RANGE_SIZE ((uintptr_t) PAGE_1GB * 64ULL)

/* mmio_map is for device registers, and mmio_map_dma is for
 * host memory that devices use as DMA (latter cacheable) */
void cc_mem_io *mmio_map(paddr_t phys, size_t size);
void mmio_unmap(void cc_mem_io *vaddr, size_t size);
void *mmio_map_dma(paddr_t phys, size_t size);
void mmio_init();

static inline void mmio_write_64(volatile void cc_mem_io *address,
                                 uint64_t value) {
    asm volatile("movq %0, (%1)" : : "r"(value), "r"(address) : "memory");
}

static inline void mmio_write_32(volatile void cc_mem_io *address,
                                 uint32_t value) {
    asm volatile("movl %0, (%1)" : : "r"(value), "r"(address) : "memory");
}

static inline void mmio_write_16(volatile void cc_mem_io *address,
                                 uint16_t value) {
    asm volatile("movw %0, (%1)" : : "r"(value), "r"(address) : "memory");
}

static inline void mmio_write_8(volatile void cc_mem_io *address,
                                uint8_t value) {
    asm volatile("movb %0, (%1)" : : "r"(value), "r"(address) : "memory");
}

static inline uint64_t mmio_read_64(volatile void cc_mem_io *address) {
    uint64_t value;
    asm volatile("movq (%1), %0" : "=r"(value) : "r"(address) : "memory");
    return value;
}

static inline uint32_t mmio_read_32(volatile void cc_mem_io *address) {
    uint32_t value;
    asm volatile("movl (%1), %0" : "=r"(value) : "r"(address) : "memory");
    return value;
}

static inline uint16_t mmio_read_16(volatile void cc_mem_io *address) {
    uint16_t value;
    asm volatile("movw (%1), %0" : "=r"(value) : "r"(address) : "memory");
    return value;
}

static inline uint8_t mmio_read_8(volatile void cc_mem_io *address) {
    uint8_t value;
    asm volatile("movb (%1), %0" : "=r"(value) : "r"(address) : "memory");
    return value;
}
