#include <acpi/acpi.h>
#include <acpi/cst.h>
#include <acpi/hpet.h>
#include <acpi/ioapic.h>
#include <acpi/lapic.h>
#include <acpi/uacpi_interface.h>
#include <asm.h>
#include <boot/gdt.h>
#include <bootstage.h>
#include <cmdline.h>
#include <compiler/core.h>
#include <console/printf.h>
#include <console/term.h>
#include <drivers/iommu/iommu.h>
#include <drivers/mmio.h>
#include <elf.h>
#include <err.h>
#include <irq/idt.h>
#include <limine.h>
#include <log.h>
#include <mem/address_range.h>
#include <mem/arena.h>
#include <mem/asan.h>
#include <mem/domain.h>
#include <mem/global.h>
#include <mem/movealloc.h>
#include <mem/page_alloc.h>
#include <mem/pmm.h>
#include <mem/slab.h>
#include <mem/tlb.h>
#include <mem/vmm.h>
#include <ndjson.h>
#include <nightmare/nightmare.h>
#include <registry.h>
#include <requests.h>
#include <sch/domain.h>
#include <sch/periodic_work.h>
#include <sch/sched.h>
#include <stack_depot.h>
#include <structures/cpu_mask.h>
#include <sync/lock_chk.h>
#include <sync/rcu.h>
#include <sync/seqlock.h>
#include <sync/spinlock.h>
#include <sync/turnstile.h>
#include <syscall.h>
#include <test/test.h>
#include <thread/dpc.h>
#include <thread/reaper.h>
#include <thread/thread.h>
#include <thread/workqueue.h>
#include <time/clock.h>
#include <time/lapic_timer.h>
#include <time/timekeeper.h>
#include <time/timer.h>
#include <topo/core.h>
#include <topo/domain.h>
#include <topo/global.h>
#include <topo/percpu.h>
#include <topo/perdomain.h>
#include <topo/pertopo.h>
#include <topo/smp.h>
#include <tsa.h>
#include <types/types.h>
#include <watchdog.h>

#define BEHAVIOR /* avoids undefined behavior */

void k_main(void);
cc_no_asan void k_main(void) TSA_ACQUIRES_IRQS {
    irq_disable();
    topo_global.cpu_count = mp_request.response->cpu_count;
    mem_global.hhdm_offset = hhdm_request.response->offset;
    atomic_init(&mem_global.pt_epoch, 1);

    printf_init(framebuffer_request.response->framebuffers[0]);
    err_facilities_init();
    enforce(topo_global.cpu_count <= CPU_MASK_BITS, CAPACITY_EXCEEDED,
            "%zu CPUs exceed CPU_MASK_BITS (%d)", topo_global.cpu_count,
            CPU_MASK_BITS);
    ndjson_early_init();
    bootstage_advance(BOOTSTAGE_EARLY_FB);

    const char *cmdline =
        cmdline_request.response ? cmdline_request.response->cmdline : "";

    if (cmdline_wants_help(cmdline))
        cmdline_dump_help();

    pmm_early_init(memmap_request);
    vmm_init(memmap_request.response, xa_request.response);
    pmm_mid_init();

#ifdef DEBUG_ASAN
    asan_init();
#endif

    address_ranges_init();
    slab_allocator_init();
    page_alloc_init();

    stack_depot_init();
    log_sites_init();
    bootstage_advance(BOOTSTAGE_EARLY_ALLOCATORS);
    gdt_load();
    syscall_setup(syscall_entry);
    smp_setup_bsp(mp_request.response);

    clocks_init();

    mmio_init();
    irq_init();
    uacpi_init(rsdp_request.response->address);
    x2apic_init();
    lapic_init();
    hpet_init();
    ioapic_init();
    acpi_find_cst();
    bootstage_advance(BOOTSTAGE_EARLY_DEVICES);

    srat_init();
    slit_init();
    iommu_init();

    domain_init();
    pmm_late_init();
    slab_domain_init();

    percpu_obj_init();
    watchdog_init();
    smp_init();
    tlb_init();

    domain_init_after_smp();
    domain_buddies_init_after_smp();
    thread_init_thread_ids();

    scheduler_init();
    turnstiles_init();

    cmdline_parse(cmdline);
    ndjson_init();
    cmdline_debug_hook();

    lapic_timer_init_bsp();
    dpc_init_percpu();
    smp_wake(mp_request.response);
    timekeeper_init();
    term_probe();

    topology_init();
    pertopo_obj_init();
    perdomain_obj_init();
    scheduler_domains_init();
    tlb_init();
    bootstage_advance(BOOTSTAGE_MID_TOPOLOGY);

    lapic_clock_evdev_group_init();
    timers_init_bsp();

    scheduler_periodic_work_init();
    movealloc_exec_all();
    bootstage_advance(BOOTSTAGE_MID_ALLOCATORS);

    scheduler_yield();
}

void k_sch_main(void *nop) {
    cc_ignore(nop);
    /* make sure everyone else is idle before we
     * advance the bootstage here... */
    smp_wait_for_others_to_idle();

    /* we have to force everyone to disable their
     * ticks because this prevents anyone from
     * possibly entering an ISR since IRQL
     * operations check bootstages and they can
     * see the MID_ALLOCATORS bootstage (no-op) in
     * an early `irql_raise` and the LATE_DEVICES
     * bootstage later on (causing mis-raised IRQLs) */
    smp_disable_all_ticks();

    smp_boot_fence_raise();
    bootstage_advance(BOOTSTAGE_LATE);
    smp_boot_fence_lower();
    lock_chk_init();
    smp_timer_init();

    smp_enable_all_ticks();
    watchdog_start();
    scheduler_debug_start();

    rcu_init();
    workqueues_permanent_init();
    slab_domain_init_late();
    domain_buddies_init_late();
    reaper_init();
    arena_global_init();

    registry_setup();

#ifdef TEST_NIGHTMARE_ENABLED
    nightmare_run();
#endif

#ifdef TEST_ENABLED
    tests_run();
#endif

    bootstage_advance(BOOTSTAGE_COMPLETE);

#ifdef TEST_ENABLED
    return;
#endif

    thread_print(thread_get_current());

    domain_buddy_dump();
}
