/* TODO: spec citations per structure */
#include <drivers/e1000.h>
#include <drivers/verifier.h>

dv_layout(e1000_tx_desc, 0x10);
dv_field(e1000_tx_desc, addr, 0x0, 8);
dv_field(e1000_tx_desc, length, 0x8, 2);
dv_field(e1000_tx_desc, cso, 0xa, 1);
dv_field(e1000_tx_desc, css, 0xd, 1);
dv_field(e1000_tx_desc, special, 0xe, 2);

dv_layout(e1000_rx_desc, 0x10);
dv_field(e1000_rx_desc, addr, 0x0, 8);
dv_field(e1000_rx_desc, length, 0x8, 2);
dv_field(e1000_rx_desc, checksum, 0xa, 2);
dv_field(e1000_rx_desc, errors, 0xd, 1);
dv_field(e1000_rx_desc, special, 0xe, 2);

dv_layout(eth_hdr, 0xe);
dv_field(eth_hdr, dest, 0x0, 6);
dv_field(eth_hdr, src, 0x6, 6);
dv_field(eth_hdr, ethertype, 0xc, 2);

dv_layout(ipv4_hdr, 0x14);
dv_field(ipv4_hdr, version_ihl, 0x0, 1);
dv_field(ipv4_hdr, tos, 0x1, 1);
dv_field(ipv4_hdr, total_length, 0x2, 2);
dv_field(ipv4_hdr, id, 0x4, 2);
dv_field(ipv4_hdr, flags_fragment, 0x6, 2);
dv_field(ipv4_hdr, ttl, 0x8, 1);
dv_field(ipv4_hdr, protocol, 0x9, 1);
dv_field(ipv4_hdr, checksum, 0xa, 2);
dv_field(ipv4_hdr, src_ip, 0xc, 4);
dv_field(ipv4_hdr, dest_ip, 0x10, 4);

dv_layout(icmp_hdr, 0x8);
dv_field(icmp_hdr, type, 0x0, 1);
dv_field(icmp_hdr, code, 0x1, 1);
dv_field(icmp_hdr, checksum, 0x2, 2);
dv_field(icmp_hdr, identifier, 0x4, 2);
dv_field(icmp_hdr, sequence, 0x6, 2);
