/* TODO: spec citations per structure */

#include <drivers/iommu/vt_d.h>
#include <drivers/verifier.h>

#define DV_STRUCT vtd_regs
dv_size(0xC0);
dv_align(1);
dv_field(uint32_t, version, 0x00);
dv_field(uint32_t, _res0, 0x04);
dv_field(uint64_t, capabilities, 0x08);
dv_field(uint64_t, extended_capabilities, 0x10);
dv_field(uint32_t, global_command, 0x18);
dv_field(uint32_t, global_status, 0x1C);
dv_field(uint64_t, root_table_addr, 0x20);
dv_field(uint64_t, context_command, 0x28);
dv_field(uint32_t, _res1, 0x30);
dv_field(uint32_t, fault_status, 0x34);
dv_field(uint32_t, fault_event_control, 0x38);
dv_field(uint32_t, fault_event_data, 0x3C);
dv_field(uint32_t, fault_event_addr, 0x40);
dv_field(uint32_t, fault_event_addr_upper, 0x44);
dv_field(uint64_t[2], _res2, 0x48);
dv_field(uint64_t, advanced_fault_log, 0x58);
dv_field(uint32_t, _res3, 0x60);
dv_field(uint32_t, protected_memory_enable, 0x64);
dv_field(uint32_t, protected_low_mem_base, 0x68);
dv_field(uint32_t, protected_low_mem_limit, 0x6C);
dv_field(uint64_t, protected_high_mem_base, 0x70);
dv_field(uint64_t, protected_high_mem_limit, 0x78);
dv_field(uint64_t, invalidation_queue_head, 0x80);
dv_field(uint64_t, invalidation_queue_tail, 0x88);
dv_field(uint64_t, invalidation_queue_addr, 0x90);
dv_field(uint32_t, _res4, 0x98);
dv_field(uint32_t, invalidation_comp_status, 0x9C);
dv_field(uint32_t, invalidation_event_ctrl, 0xA0);
dv_field(uint32_t, invalidation_event_data, 0xA4);
dv_field(uint32_t, invalidation_event_addr, 0xA8);
dv_field(uint32_t, invalidation_event_addr_upper, 0xAC);
dv_field(uint64_t, invalidation_queue_error_record, 0xB0);
dv_field(uint64_t, interrupt_remapping_table_addr, 0xB8);
#undef DV_STRUCT

#define DV_STRUCT vtd_root_entry
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, lo, 0x00);
dv_field(uint64_t, hi, 0x08);
#undef DV_STRUCT

#define DV_STRUCT vtd_context_entry
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, lo, 0x00);
dv_field(uint64_t, hi, 0x08);
#undef DV_STRUCT

#define DV_STRUCT vtd_inv_desc
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, lo, 0x00);
dv_field(uint64_t, hi, 0x08);
#undef DV_STRUCT
