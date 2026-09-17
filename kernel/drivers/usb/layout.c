/* TODO: spec citations per structure */
#include <drivers/usb/usb.h>
#include <drivers/verifier.h>

/* USB specification, page 276 */
dv_layout(usb_setup_packet, 0x8);
dv_field(usb_setup_packet, bitmap_request_type, 0x0, 1);
dv_field(usb_setup_packet, request, 0x1, 1);
dv_field(usb_setup_packet, value, 0x2, 2);
dv_field(usb_setup_packet, length, 0x6, 2);

/* USB specification, page 290 */
dv_layout(usb_device_descriptor, 0x12);
dv_field(usb_device_descriptor, length, 0x0, 1);
dv_field(usb_device_descriptor, type, 0x1, 1);
dv_field(usb_device_descriptor, usb_num_bcd, 0x2, 2);
dv_field(usb_device_descriptor, class, 0x4, 1);
dv_field(usb_device_descriptor, subclass, 0x5, 1);
dv_field(usb_device_descriptor, protocol, 0x6, 1);
dv_field(usb_device_descriptor, max_packet_size, 0x7, 1);
dv_field(usb_device_descriptor, vendor_id, 0x8, 2);
dv_field(usb_device_descriptor, product_id, 0xa, 2);
dv_field(usb_device_descriptor, device_num_bcd, 0xc, 2);
dv_field(usb_device_descriptor, manufacturer, 0xe, 1);
dv_field(usb_device_descriptor, product, 0xf, 1);
dv_field(usb_device_descriptor, serial_num, 0x10, 1);
dv_field(usb_device_descriptor, num_configs, 0x11, 1);

/* TODO: spec citation */
dv_layout(usb_interface_descriptor, 0x9);
dv_field(usb_interface_descriptor, length, 0x0, 1);
dv_field(usb_interface_descriptor, type, 0x1, 1);
dv_field(usb_interface_descriptor, interface_number, 0x2, 1);
dv_field(usb_interface_descriptor, alternate_setting, 0x3, 1);
dv_field(usb_interface_descriptor, num_endpoints, 0x4, 1);
dv_field(usb_interface_descriptor, class, 0x5, 1);
dv_field(usb_interface_descriptor, subclass, 0x6, 1);
dv_field(usb_interface_descriptor, protocol, 0x7, 1);
dv_field(usb_interface_descriptor, interface, 0x8, 1);

/* TODO: spec citation */
dv_layout(usb_config_descriptor, 0x9);
dv_field(usb_config_descriptor, length, 0x0, 1);
dv_field(usb_config_descriptor, descriptor_type, 0x1, 1);
dv_field(usb_config_descriptor, total_length, 0x2, 2);
dv_field(usb_config_descriptor, num_interfaces, 0x4, 1);
dv_field(usb_config_descriptor, configuration_value, 0x5, 1);
dv_field(usb_config_descriptor, configuration, 0x6, 1);
dv_field(usb_config_descriptor, bitmap_attributes, 0x7, 1);
dv_field(usb_config_descriptor, max_power, 0x8, 1);

/* TODO: spec citation */
dv_layout(usb_endpoint_descriptor, 0x7);
dv_field(usb_endpoint_descriptor, length, 0x0, 1);
dv_field(usb_endpoint_descriptor, type, 0x1, 1);
dv_field(usb_endpoint_descriptor, address, 0x2, 1);
dv_field(usb_endpoint_descriptor, attributes, 0x3, 1);
dv_field(usb_endpoint_descriptor, max_packet_size, 0x4, 2);
dv_field(usb_endpoint_descriptor, interval, 0x6, 1);
