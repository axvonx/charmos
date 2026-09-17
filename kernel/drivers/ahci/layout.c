/* TODO: spec citations (AHCI 1.3.1) per structure */

#include <drivers/ahci.h>
#include <drivers/verifier.h>

#define DV_STRUCT ahci_prdt_entry
dv_size(0x10);
dv_align(1);
dv_field(uint32_t, dba, 0x00);
dv_field(uint32_t, dbau, 0x04);
dv_field(uint32_t, reserved, 0x08);
#undef DV_STRUCT

#define DV_STRUCT ahci_cmd_table
dv_size(0x80);
dv_align(1);
dv_field(uint8_t[64], cfis, 0x00);
dv_field(uint8_t[16], acmd, 0x40);
dv_field(uint8_t[48], reserved, 0x50);
dv_field_at(prdt_entry, 0x80);
#undef DV_STRUCT

#define DV_STRUCT ahci_cmd_header
dv_size(0x20);
dv_align(1);
dv_field(uint16_t, prdtl, 0x02);
dv_field(uint32_t, prdbc, 0x04);
dv_field(uint32_t, ctba, 0x08);
dv_field(uint32_t, ctbau, 0x0C);
dv_field(uint32_t[4], rsv1, 0x10);
#undef DV_STRUCT

#define DV_STRUCT ahci_fis_reg_h2d
dv_size(0x14);
dv_field(uint8_t, fis_type, 0x00);
dv_field(uint8_t, command, 0x02);
dv_field(uint8_t, featurel, 0x03);
dv_field(uint8_t, lba0, 0x04);
dv_field(uint8_t, lba1, 0x05);
dv_field(uint8_t, lba2, 0x06);
dv_field(uint8_t, device, 0x07);
dv_field(uint8_t, lba3, 0x08);
dv_field(uint8_t, lba4, 0x09);
dv_field(uint8_t, lba5, 0x0A);
dv_field(uint8_t, featureh, 0x0B);
dv_field(uint8_t, countl, 0x0C);
dv_field(uint8_t, counth, 0x0D);
dv_field(uint8_t, icc, 0x0E);
dv_field(uint8_t, control, 0x0F);
dv_field(uint8_t[4], reserved2, 0x10);
#undef DV_STRUCT

/* ahci_fis_reg_d2h is unused, so absent from debug info; written by hand */
#define DV_STRUCT ahci_fis_reg_d2h
dv_size(0x14);
dv_field(uint8_t, fis_type, 0x00);
dv_field(uint8_t, status, 0x02);
dv_field(uint8_t, error, 0x03);
dv_field(uint8_t, lba0, 0x04);
dv_field(uint8_t, lba1, 0x05);
dv_field(uint8_t, lba2, 0x06);
dv_field(uint8_t, device, 0x07);
dv_field(uint8_t, lba3, 0x08);
dv_field(uint8_t, lba4, 0x09);
dv_field(uint8_t, lba5, 0x0A);
dv_field(uint8_t, rsv2, 0x0B);
dv_field(uint8_t, countl, 0x0C);
dv_field(uint8_t, counth, 0x0D);
dv_field(uint8_t[2], rsv3, 0x0E);
dv_field(uint8_t[4], rsv4, 0x10);
#undef DV_STRUCT

#define DV_STRUCT ahci_port
dv_size(0x80);
dv_field(uint32_t, clb, 0x00);
dv_field(uint32_t, clbu, 0x04);
dv_field(uint32_t, fb, 0x08);
dv_field(uint32_t, fbu, 0x0C);
dv_field(uint32_t, is, 0x10);
dv_field(uint32_t, ie, 0x14);
dv_field(uint32_t, cmd, 0x18);
dv_field(uint32_t, rsv0, 0x1C);
dv_field(uint32_t, tfd, 0x20);
dv_field(uint32_t, sig, 0x24);
dv_field(uint32_t, ssts, 0x28);
dv_field(uint32_t, sctl, 0x2C);
dv_field(uint32_t, serr, 0x30);
dv_field(uint32_t, sact, 0x34);
dv_field(uint32_t, ci, 0x38);
dv_field(uint32_t, sntf, 0x3C);
dv_field(uint32_t, fbs, 0x40);
dv_field(uint32_t[11], rsv1, 0x44);
dv_field(uint32_t[4], vendor, 0x70);
#undef DV_STRUCT
