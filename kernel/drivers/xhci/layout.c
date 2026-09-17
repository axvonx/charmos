/* TODO: spec citations per structure */
#include <drivers/usb/xhci.h>
#include <drivers/verifier.h>

dv_layout(xhci_cap_regs, 0x20);
dv_field(xhci_cap_regs, cap_length, 0x0, 1);
dv_field(xhci_cap_regs, reserved, 0x1, 1);
dv_field(xhci_cap_regs, hci_version, 0x2, 2);
dv_field(xhci_cap_regs, hcs_params1, 0x4, 4);
dv_field(xhci_cap_regs, hcs_params2, 0x8, 4);
dv_field(xhci_cap_regs, hcs_params3, 0xc, 4);
dv_field(xhci_cap_regs, hcc_params1, 0x10, 4);
dv_field(xhci_cap_regs, dboff, 0x14, 4);
dv_field(xhci_cap_regs, rtsoff, 0x18, 4);
dv_field(xhci_cap_regs, hcc_params2, 0x1c, 4);

dv_layout(xhci_op_regs, 0x400);
dv_field(xhci_op_regs, usbcmd, 0x0, 4);
dv_field(xhci_op_regs, usbsts, 0x4, 4);
dv_field(xhci_op_regs, pagesize, 0x8, 4);
dv_field(xhci_op_regs, reserved, 0xc, 8);
dv_field(xhci_op_regs, dnctrl, 0x14, 4);
dv_field(xhci_op_regs, crcr, 0x18, 8);
dv_field(xhci_op_regs, reserved2, 0x20, 16);
dv_field(xhci_op_regs, dcbaap, 0x30, 8);
dv_field(xhci_op_regs, config, 0x38, 4);
dv_field(xhci_op_regs, reserved3, 0x3c, 964);
dv_field_at(xhci_op_regs, regs, 0x400);

dv_layout(xhci_port_regs, 0x10);
dv_field(xhci_port_regs, portsc, 0x0, 4);
dv_field(xhci_port_regs, portpmsc, 0x4, 4);
dv_field(xhci_port_regs, portli, 0x8, 4);
dv_field(xhci_port_regs, portct, 0xc, 4);

dv_layout(xhci_usbcmd, 0x4);

dv_layout(xhci_slot_ctx, 0x20);
dv_field(xhci_slot_ctx, reserved3, 0x10, 16);
/* bitfields: route_string, speed, reserved0, mtt:1, hub:1,
 * context_entries, max_exit_latency, root_hub_port, num_ports,
 * parent_hub_slot_id, parent_port_number, parent_think_time, reserved1,
 * interrupter_target, usb_device_address, reserved2, slot_state */

/* xHCI specification, page 450 */
dv_layout(xhci_ep_ctx, 0x20);
dv_field(xhci_ep_ctx, reserved5, 0x14, 12);
/* bitfields: ep_state, reserved1, mult, max_pstreams, lsa:1,
 * interval, max_esit_payload_hi, reserved2, error_count, ep_type, reserved3,
 * host_initiate_disable, max_burst_size, max_packet_size, average_trb_length,
 * max_esit_payload_lo */

/* xHCI specification, page 461 */
dv_layout(xhci_input_ctrl_ctx, 0x20);
dv_field(xhci_input_ctrl_ctx, drop_flags, 0x0, 4);
dv_field(xhci_input_ctrl_ctx, add_flags, 0x4, 4);
dv_field(xhci_input_ctrl_ctx, reserved, 0x8, 20);
/* bitfields: config, interface_num, alternate_setting,
 * reserved1 */

/* xHCI specification, page 460 */
dv_layout(xhci_input_ctx, 0x420);
dv_field(xhci_input_ctx, ctrl_ctx, 0x0, 32);
dv_field(xhci_input_ctx, slot_ctx, 0x20, 32);
dv_field(xhci_input_ctx, ep_ctx, 0x40, 992);

dv_layout(xhci_device_ctx, 0x420);
dv_field(xhci_device_ctx, slot_ctx, 0x0, 32);
dv_field(xhci_device_ctx, ep_ctx, 0x20, 1024);

dv_layout(xhci_trb, 0x10);
dv_field(xhci_trb, parameter, 0x0, 8);
dv_field(xhci_trb, status, 0x8, 4);
dv_field(xhci_trb, control, 0xc, 4);

dv_layout(xhci_erst_entry, 0x10);
dv_field(xhci_erst_entry, ring_segment_base, 0x0, 8);
dv_field(xhci_erst_entry, ring_segment_size, 0x8, 4);
dv_field(xhci_erst_entry, reserved, 0xc, 4);

dv_layout(xhci_interrupter_regs, 0x20);
dv_field(xhci_interrupter_regs, iman, 0x0, 4);
dv_field(xhci_interrupter_regs, imod, 0x4, 4);
dv_field(xhci_interrupter_regs, erstsz, 0x8, 4);
dv_field(xhci_interrupter_regs, reserved, 0xc, 4);
dv_field(xhci_interrupter_regs, erstba, 0x10, 8);
dv_field(xhci_interrupter_regs, erdp, 0x18, 8);

/* xHCI specification, page 441 */
dv_layout(xhci_dcbaa, 0x800);
dv_field(xhci_dcbaa, ptrs, 0x0, 2048);
