/* TODO: spec citations per structure */

#include <drivers/e1000.h>
#include <drivers/verifier.h>

#define DV_STRUCT e1000_tx_desc
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, addr, 0x00);
dv_field(uint16_t, length, 0x08);
dv_field(uint8_t, cso, 0x0A);
dv_field(uint8_t, css, 0x0D);
dv_field(uint16_t, special, 0x0E);
#undef DV_STRUCT

#define DV_STRUCT e1000_rx_desc
dv_size(0x10);
dv_align(1);
dv_field(uint64_t, addr, 0x00);
dv_field(uint16_t, length, 0x08);
dv_field(uint16_t, checksum, 0x0A);
dv_field(uint8_t, errors, 0x0D);
dv_field(uint16_t, special, 0x0E);
#undef DV_STRUCT

#define DV_STRUCT eth_hdr
dv_size(0x0E);
dv_align(1);
dv_field(uint8_t[6], dest, 0x00);
dv_field(uint8_t[6], src, 0x06);
dv_field(uint16_t, ethertype, 0x0C);
#undef DV_STRUCT

#define DV_STRUCT ipv4_hdr
dv_size(0x14);
dv_align(1);
dv_field(uint8_t, version_ihl, 0x00);
dv_field(uint8_t, tos, 0x01);
dv_field(uint16_t, total_length, 0x02);
dv_field(uint16_t, id, 0x04);
dv_field(uint16_t, flags_fragment, 0x06);
dv_field(uint8_t, ttl, 0x08);
dv_field(uint8_t, protocol, 0x09);
dv_field(uint16_t, checksum, 0x0A);
dv_field(uint32_t, src_ip, 0x0C);
dv_field(uint32_t, dest_ip, 0x10);
#undef DV_STRUCT

#define DV_STRUCT icmp_hdr
dv_size(0x08);
dv_align(1);
dv_field(uint8_t, type, 0x00);
dv_field(uint8_t, code, 0x01);
dv_field(uint16_t, checksum, 0x02);
dv_field(uint16_t, identifier, 0x04);
dv_field(uint16_t, sequence, 0x06);
#undef DV_STRUCT
