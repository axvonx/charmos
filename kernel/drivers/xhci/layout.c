/* TODO: spec citations per structure */

#include <drivers/usb/xhci.h>
#include <drivers/verifier.h>

#define DV_STRUCT xhci_cap_regs
dv_size(0x20);
dv_align(1);
dv_field(uint8_t, cap_length, 0x00);
dv_field(uint8_t, reserved, 0x01);
dv_field(uint16_t, hci_version, 0x02);
dv_field(uint32_t, hcs_params1, 0x04);
dv_field(uint32_t, hcs_params2, 0x08);
dv_field(uint32_t, hcs_params3, 0x0C);
dv_field(uint32_t, hcc_params1, 0x10);
dv_field(uint32_t, dboff, 0x14);
dv_field(uint32_t, rtsoff, 0x18);
dv_field(uint32_t, hcc_params2, 0x1C);
#undef DV_STRUCT

#define DV_STRUCT xhci_op_regs
dv_size(0x400);
dv_field(struct xhci_usbcmd, usbcmd, 0x00);
dv_field(uint32_t, usbsts, 0x04);
dv_field(uint32_t, pagesize, 0x08);
dv_field(uint32_t[2], reserved, 0x0C);
dv_field(uint32_t, dnctrl, 0x14);
dv_field(uint64_t, crcr, 0x18);
dv_field(uint32_t[4], reserved2, 0x20);
dv_field(uint64_t, dcbaap, 0x30);
dv_field(uint32_t, config, 0x38);
dv_field(uint32_t[241], reserved3, 0x3C);
dv_field_at(regs, 0x400);
#undef DV_STRUCT

#define DV_STRUCT xhci_port_regs
dv_size(0x10);
dv_field(uint32_t, portsc, 0x00);
dv_field(uint32_t, portpmsc, 0x04);
dv_field(uint32_t, portli, 0x08);
dv_field(uint32_t, portct, 0x0C);
#undef DV_STRUCT

#define DV_STRUCT xhci_usbcmd
dv_size(0x04);
dv_align(1);
#undef DV_STRUCT

#define DV_STRUCT xhci_slot_ctx
dv_size(0x20);
dv_align(1);
dv_field(uint32_t[4], reserved3, 0x10);
/* bitfields: route_string, speed, reserved0, mtt, hub, context_entries,
 * max_exit_latency, root_hub_port, num_ports, parent_hub_slot_id,
 * parent_port_number, parent_think_time, reserved1, interrupter_target,
 * usb_device_address, reserved2, slot_state */
#undef DV_STRUCT

/* xHCI specification, page 450 */
#define DV_STRUCT xhci_ep_ctx
dv_size(0x20);
dv_align(1);
dv_field(uint32_t[3], reserved5, 0x14);
/* bitfields: ep_state, reserved1, mult, max_pstreams, lsa, interval,
 * max_esit_payload_hi, reserved2, error_count, ep_type, reserved3,
 * host_initiate_disable, max_burst_size, max_packet_size, average_trb_length,
 * max_esit_payload_lo */
#undef DV_STRUCT

/* xHCI specification, page 461 */
#define DV_STRUCT xhci_input_ctrl_ctx
dv_size(0x20);
dv_align(1);
dv_field(uint32_t, drop_flags, 0x00);
dv_field(uint32_t, add_flags, 0x04);
dv_field(uint32_t[5], reserved, 0x08);
/* bitfields: config, interface_num, alternate_setting, reserved1 */
#undef DV_STRUCT

/* xHCI specification, page 460 */
#define DV_STRUCT xhci_input_ctx
dv_size(0x420);
dv_align(1);
dv_field(struct xhci_input_ctrl_ctx, ctrl_ctx, 0x00);
dv_field(struct xhci_slot_ctx, slot_ctx, 0x20);
dv_field(struct xhci_ep_ctx[31], ep_ctx, 0x40);
#undef DV_STRUCT

#define DV_STRUCT xhci_device_ctx
dv_size(0x420);
dv_align(1);
dv_field(struct xhci_slot_ctx, slot_ctx, 0x00);
dv_field(struct xhci_ep_ctx[32], ep_ctx, 0x20);
#undef DV_STRUCT

#define DV_STRUCT xhci_trb
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, parameter, 0x00);
dv_field(uint32_t, status, 0x08);
dv_field(uint32_t, control, 0x0C);
#undef DV_STRUCT

#define DV_STRUCT xhci_erst_entry
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, ring_segment_base, 0x00);
dv_field(uint32_t, ring_segment_size, 0x08);
dv_field(uint32_t, reserved, 0x0C);
#undef DV_STRUCT

#define DV_STRUCT xhci_interrupter_regs
dv_size(0x20);
dv_align(1);
dv_field(uint32_t, iman, 0x00);
dv_field(uint32_t, imod, 0x04);
dv_field(uint32_t, erstsz, 0x08);
dv_field(uint32_t, reserved, 0x0C);
dv_field(uint64_t, erstba, 0x10);
dv_field(uint64_t, erdp, 0x18);
#undef DV_STRUCT

/* xHCI specification, page 441 */
#define DV_STRUCT xhci_dcbaa
dv_size(0x800);
dv_align(64);
dv_field(uint64_t[256], ptrs, 0x00);
#undef DV_STRUCT
