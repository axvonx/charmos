#include <console/printf.h>
#include <dbg.h>
#include <irq/irq.h>
#include <mem/address_range.h>
#include <mem/vmm.h>
#include <sch/sched.h>
#include <string.h>
#include <sync/spinlock.h>
#include <thread/thread.h>

static struct spinlock pf_lock = SPINLOCK_INIT;

static bool addr_is_mapped(uint64_t addr) {
    return vmm_get_phys_unsafe((vaddr_t) PAGE_ALIGN_DOWN(addr)) !=
           (uintptr_t) -1;
}

static void dump_slab_exec_fault(struct thread *curr,
                                 struct irq_context *rsp) {
    printf("\n=== SLAB EXEC FAULT DEBUG ===\n");

    printf("Faulting RIP (from CPU): %p\n", rsp->rip);
    printf("Faulting RSP (from CPU): %p\n", rsp->rsp);

    printf("\n--- Current thread struct ---\n");
    printf("Thread struct addr: %p\n", (uint64_t) curr);

    if (!addr_is_mapped((uint64_t) curr)) {
        printf("  Thread pointer is NOT MAPPED - cannot dump\n");
        return;
    }

    printf("  tid:   %lu\n", curr->id);
    printf("  name:  %p", (uint64_t) curr->name);
    if (curr->name && addr_is_mapped((uint64_t) curr->name))
        printf(" -> \"%s\"", curr->name);
    printf("\n");
    printf("  entry: %p\n", (uint64_t) curr->entry);
    printf("  stack: %p (size %lu)\n", (uint64_t) curr->stack,
           curr->stack_size);
    printf("  state: %u\n", (uint32_t) curr->state);
    printf("  core:  %u\n", (uint32_t) curr->curr_core);
    printf("  flags: 0x%lx\n", (uint64_t) thread_get_flags(curr));
    printf("  ref:   %lu\n", (uint64_t) refcount_read(&curr->refcount));

    printf("\n--- Saved regs (post-switch residual) ---\n");
    printf("  rbx = %p\n", curr->regs.rbx);
    printf("  rbp = %p\n", curr->regs.rbp);
    printf("  r12 = %p\n", curr->regs.r12);
    printf("  r13 = %p\n", curr->regs.r13);
    printf("  r14 = %p\n", curr->regs.r14);
    printf("  r15 = %p\n", curr->regs.r15);
    printf("  rsp = %p\n", curr->regs.rsp);
    printf("  rip = %p\n", curr->regs.rip);

    printf("\n--- Thread struct raw dump (992 bytes) ---\n");
    debug_print_memory((void *) curr, 992);

    uint8_t *prev_obj = (uint8_t *) curr - 992;
    printf("\n--- Preceding slab object tail (last 256 bytes) ---\n");
    if (addr_is_mapped((uint64_t) prev_obj)) {
        debug_print_memory(prev_obj + 992 - 256, 256);
    } else {
        printf("  Preceding object at %p is not mapped\n",
               (uint64_t) prev_obj);
    }

    printf("\n--- Stack at fault RSP (%p) ---\n", rsp->rsp);
    if (addr_is_mapped(rsp->rsp)) {
        debug_print_memory((void *) rsp->rsp, 256);
    } else {
        printf("  RSP is not mapped!\n");
    }

    printf("\n--- ISR context (regs at fault time) ---\n");
    printf("  rax=%p  rbx=%p\n", rsp->rax, rsp->rbx);
    printf("  rcx=%p  rdx=%p\n", rsp->rcx, rsp->rdx);
    printf("  rdi=%p  rsi=%p\n", rsp->rdi, rsp->rsi);
    printf("  rbp=%p  rsp=%p\n", rsp->rbp, rsp->rsp);
    printf("  r8 =%p  r9 =%p\n", rsp->r8, rsp->r9);
    printf("  r10=%p  r11=%p\n", rsp->r10, rsp->r11);
    printf("  r12=%p  r13=%p\n", rsp->r12, rsp->r13);
    printf("  r14=%p  r15=%p\n", rsp->r14, rsp->r15);
    printf("  rip=%p  rfl=%p\n", rsp->rip, rsp->rflags);
    printf("  cs=%p   ss=%p\n", rsp->cs, rsp->ss);

    struct scheduler *sched = global.schedulers[smp_core_id()];
    printf("\n--- Scheduler state (core %u) ---\n", smp_core_id());
    printf("  sched->current = %p\n", (uint64_t) sched->current);
    printf("  sched->drop_last_ref = %p\n",
           (uint64_t) sched->drop_last_ref);
    printf("  sched->other_locked = %p\n",
           (uint64_t) sched->other_locked);
    printf("  sched->stealing_work = %u\n",
           (uint32_t) sched->stealing_work);

    printf("\n--- Thread migration info ---\n");
    printf("  migrate_to = %ld\n",
           (int64_t) atomic_load(&curr->migrate_to));
    printf("  migration_gen = 0x%lx\n", curr->migration_generation);
    printf("  scheduler = %p\n", (uint64_t) atomic_load(&curr->scheduler));
}

enum irq_result page_fault_handler(void *context, uint8_t vector,
                                   struct irq_context *rsp) {
    (void) context, (void) vector;
    struct thread *curr = thread_get_current();

    uint64_t error_code = rsp->error_code;
    uint64_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    struct address_range *ar = address_range_for_addr(fault_addr);
    const char *name = ar ? ar->name : "UNKNOWN";

    spin_lock_raw(&pf_lock);

    printf("\n=== PAGE FAULT === @ %p\n", rsp->rip);
    printf("Faulting Address (CR2): %p (ar: %s)\n", fault_addr, name);
    printf("Error Code: %p\n", error_code);
    printf("  - Page not Present (P): %s\n",
           (error_code & 0x01) ? "Yes" : "No");
    printf("  - Write Access (W/R): %s\n",
           (error_code & 0x02) ? "Write" : "Read");
    printf("  - User Mode (U/S): %s\n",
           (error_code & 0x04) ? "User" : "Supervisor");
    printf("  - Reserved Bit Set (RSVD): %s\n",
           (error_code & 0x08) ? "Yes" : "No");
    printf("  - Instruction Fetch (I/D): %s\n",
           (error_code & 0x10) ? "Yes" : "No");
    printf("  - Protection Key Violation (PK): %s\n",
           (error_code & 0x20) ? "Yes" : "No");
    printf("  - Kernel stack %p -> %p\n", curr->stack,
           (uintptr_t) curr->stack + curr->stack_size);

    vaddr_t protector_base = (uintptr_t) curr->stack - PAGE_SIZE;
    vaddr_t protector_top = (uintptr_t) curr->stack;
    if (fault_addr >= protector_base && fault_addr <= protector_top)
        printf("Likely stack overflow!! Fault in protector page!!!\n");

    printf("\n--- Stack at fault RSP ---\n");
    debug_print_stack_from((uint64_t *) rsp->rsp, 0);

    bool is_slab_exec =
        (error_code & 0x10) && ar && strcmp(ar->name, "slab") == 0;

    if (!is_slab_exec && (error_code & 0x10)) {
        struct address_range *rip_ar =
            address_range_for_addr(rsp->rip);
        if (rip_ar && strcmp(rip_ar->name, "slab") == 0)
            is_slab_exec = true;
    }

    if (is_slab_exec)
        dump_slab_exec_fault(curr, rsp);

    if (!(error_code & 0x04)) {
        spin_unlock_raw(&pf_lock);
        panic("KERNEL PAGE FAULT ON CORE %llu under thread %s\n",
              smp_core_id(), thread_get_current()->name);
        while (true) {
            disable_interrupts();
            wait_for_interrupt();
        }
    }

    spin_unlock_raw(&pf_lock);
    return IRQ_HANDLED;
}
