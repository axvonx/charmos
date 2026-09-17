/* TODO: spec citations per structure */

#include <drivers/usb/usb.h>
#include <drivers/verifier.h>

/* USB specification, page 276 */
#define DV_STRUCT usb_setup_packet
dv_size(0x08);
dv_align(1);
dv_field(uint8_t, bitmap_request_type, 0x00);
dv_field(enum usb_rq_code, request, 0x01);
dv_field(uint16_t, value, 0x02);
dv_field(uint16_t, length, 0x06);
#undef DV_STRUCT

/* USB specification, page 290 */
#define DV_STRUCT usb_device_descriptor
dv_size(0x12);
dv_align(1);
dv_field(uint8_t, length, 0x00);
dv_field(uint8_t, type, 0x01);
dv_field(uint16_t, usb_num_bcd, 0x02);
dv_field(uint8_t, class, 0x04);
dv_field(uint8_t, subclass, 0x05);
dv_field(uint8_t, protocol, 0x06);
dv_field(uint8_t, max_packet_size, 0x07);
dv_field(uint16_t, vendor_id, 0x08);
dv_field(uint16_t, product_id, 0x0A);
dv_field(uint16_t, device_num_bcd, 0x0C);
dv_field(uint8_t, manufacturer, 0x0E);
dv_field(uint8_t, product, 0x0F);
dv_field(uint8_t, serial_num, 0x10);
dv_field(uint8_t, num_configs, 0x11);
#undef DV_STRUCT

/* TODO: spec citation */
#define DV_STRUCT usb_interface_descriptor
dv_size(0x09);
dv_align(1);
dv_field(uint8_t, length, 0x00);
dv_field(uint8_t, type, 0x01);
dv_field(uint8_t, interface_number, 0x02);
dv_field(uint8_t, alternate_setting, 0x03);
dv_field(uint8_t, num_endpoints, 0x04);
dv_field(uint8_t, class, 0x05);
dv_field(uint8_t, subclass, 0x06);
dv_field(uint8_t, protocol, 0x07);
dv_field(uint8_t, interface, 0x08);
#undef DV_STRUCT

/* TODO: spec citation */
#define DV_STRUCT usb_config_descriptor
dv_size(0x09);
dv_align(1);
dv_field(uint8_t, length, 0x00);
dv_field(uint8_t, descriptor_type, 0x01);
dv_field(uint16_t, total_length, 0x02);
dv_field(uint8_t, num_interfaces, 0x04);
dv_field(uint8_t, configuration_value, 0x05);
dv_field(uint8_t, configuration, 0x06);
dv_field(uint8_t, bitmap_attributes, 0x07);
dv_field(uint8_t, max_power, 0x08);
#undef DV_STRUCT

/* TODO: spec citation */
#define DV_STRUCT usb_endpoint_descriptor
dv_size(0x07);
dv_align(1);
dv_field(uint8_t, length, 0x00);
dv_field(uint8_t, type, 0x01);
dv_field(uint8_t, address, 0x02);
dv_field(uint8_t, attributes, 0x03);
dv_field(uint16_t, max_packet_size, 0x04);
dv_field(uint8_t, interval, 0x06);
#undef DV_STRUCT
