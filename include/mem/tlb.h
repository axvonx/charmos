/* @title: TLB */
#include <acpi/lapic.h>
#include <atomic.h>
#include <mem/page.h>
#include <stdint.h>
#include <types/types.h>

/* per-cpu */
#define TLB_QUEUE_SIZE 64

struct tlb_shootdown_cpu {
    atomic_uintptr_t queue[TLB_QUEUE_SIZE];
    atomic_uint32_t head;
    atomic_uint32_t tail;
    atomic_bool in_tlb_shootdown;
    atomic_uint8_t flush_all;
    atomic_uint64_t req_gen;  /* last requested generation */
    atomic_uint64_t done_gen; /* last completed generation */
};

void tlb_init(void);
enum irq_result tlb_shootdown_isr(void *ctx, irq_t irq,
                                  struct irq_context *rsp);
void tlb_shootdown(uintptr_t addr, bool synchronous);
