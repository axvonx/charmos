/* TODO: spec citations per structure */
#include <drivers/iommu/vt_d.h>
#include <drivers/verifier.h>

dv_layout(vtd_regs, 0xc0);
dv_field(vtd_regs, version, 0x0, 4);
dv_field(vtd_regs, _res0, 0x4, 4);
dv_field(vtd_regs, capabilities, 0x8, 8);
dv_field(vtd_regs, extended_capabilities, 0x10, 8);
dv_field(vtd_regs, global_command, 0x18, 4);
dv_field(vtd_regs, global_status, 0x1c, 4);
dv_field(vtd_regs, root_table_addr, 0x20, 8);
dv_field(vtd_regs, context_command, 0x28, 8);
dv_field(vtd_regs, _res1, 0x30, 4);
dv_field(vtd_regs, fault_status, 0x34, 4);
dv_field(vtd_regs, fault_event_control, 0x38, 4);
dv_field(vtd_regs, fault_event_data, 0x3c, 4);
dv_field(vtd_regs, fault_event_addr, 0x40, 4);
dv_field(vtd_regs, fault_event_addr_upper, 0x44, 4);
dv_field(vtd_regs, _res2, 0x48, 16);
dv_field(vtd_regs, advanced_fault_log, 0x58, 8);
dv_field(vtd_regs, _res3, 0x60, 4);
dv_field(vtd_regs, protected_memory_enable, 0x64, 4);
dv_field(vtd_regs, protected_low_mem_base, 0x68, 4);
dv_field(vtd_regs, protected_low_mem_limit, 0x6c, 4);
dv_field(vtd_regs, protected_high_mem_base, 0x70, 8);
dv_field(vtd_regs, protected_high_mem_limit, 0x78, 8);
dv_field(vtd_regs, invalidation_queue_head, 0x80, 8);
dv_field(vtd_regs, invalidation_queue_tail, 0x88, 8);
dv_field(vtd_regs, invalidation_queue_addr, 0x90, 8);
dv_field(vtd_regs, _res4, 0x98, 4);
dv_field(vtd_regs, invalidation_comp_status, 0x9c, 4);
dv_field(vtd_regs, invalidation_event_ctrl, 0xa0, 4);
dv_field(vtd_regs, invalidation_event_data, 0xa4, 4);
dv_field(vtd_regs, invalidation_event_addr, 0xa8, 4);
dv_field(vtd_regs, invalidation_event_addr_upper, 0xac, 4);
dv_field(vtd_regs, invalidation_queue_error_record, 0xb0, 8);
dv_field(vtd_regs, interrupt_remapping_table_addr, 0xb8, 8);

dv_layout(vtd_root_entry, 0x10);
dv_field(vtd_root_entry, lo, 0x0, 8);
dv_field(vtd_root_entry, hi, 0x8, 8);

dv_layout(vtd_context_entry, 0x10);
dv_field(vtd_context_entry, lo, 0x0, 8);
dv_field(vtd_context_entry, hi, 0x8, 8);

dv_layout(vtd_inv_desc, 0x10);
dv_field(vtd_inv_desc, lo, 0x0, 8);
dv_field(vtd_inv_desc, hi, 0x8, 8);
