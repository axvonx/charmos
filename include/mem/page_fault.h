/* @title: Page Fault */
#pragma once
#include <irq/irq.h>
#include <types/types.h>

struct page;

enum page_fault_access { PAGE_FAULT_READ, PAGE_FAULT_WRITE, PAGE_FAULT_EXEC };

enum page_fault_error_code : uint64_t {
    PAGE_FAULT_EC_PRESENT = 1ULL << 0,     /* P:    protection violation, not
                                              a not-present page */
    PAGE_FAULT_EC_WRITE = 1ULL << 1,       /* W/R:  access was a write */
    PAGE_FAULT_EC_USER = 1ULL << 2,        /* U/S:  fault taken in user mode */
    PAGE_FAULT_EC_RESERVED = 1ULL << 3,    /* RSVD: reserved bit set in a
                                              paging-structure entry */
    PAGE_FAULT_EC_INSTRUCTION = 1ULL << 4, /* I/D:  instruction fetch */
    PAGE_FAULT_EC_PROTECTION_KEY = 1ULL << 5, /* PK:   protection-key block */
    PAGE_FAULT_EC_SHADOW_STACK = 1ULL << 6,   /* SS:   shadow-stack access */
    PAGE_FAULT_EC_SGX = 1ULL << 15,           /* SGX:  enclave violation */
};

struct page_fault_info {
    vaddr_t addr;
    enum page_fault_access access;
    bool was_present;
    bool user;
};

struct page_fault_handler_ops {
    bool (*is_valid_fault)(struct page_fault_info *pfi);
    void (*update_after_map)(vaddr_t vaddr, struct page *page);
    paddr_t (*alloc_pages)(uint8_t order);
};

struct page_fault_handler {
    struct page_fault_handler_ops *ops;
};

struct page_fault_scratch_buffer {
    vaddr_t virt;
    uint64_t error_code;
};

enum irq_result page_fault_isr(void *context, uint8_t vector,
                               struct irq_context *rsp);
