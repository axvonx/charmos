/* TODO: spec citations (AHCI 1.3.1) per structure */

#include <drivers/ahci.h>
#include <drivers/verifier.h>

dv_layout(ahci_prdt_entry, 0x10);
dv_field(ahci_prdt_entry, dba, 0x00, 4);
dv_field(ahci_prdt_entry, dbau, 0x04, 4);
dv_field(ahci_prdt_entry, reserved, 0x08, 4);

dv_layout(ahci_cmd_table, 0x80);
dv_field(ahci_cmd_table, cfis, 0x00, AHCI_CMD_TABLE_FIS_SIZE);
dv_field(ahci_cmd_table, acmd, 0x40, AHCI_CMD_TABLE_ATAPI_SIZE);
dv_field(ahci_cmd_table, reserved, 0x50, 48);
dv_field_at(ahci_cmd_table, prdt_entry, 0x80);

dv_layout(ahci_cmd_header, 0x20);
dv_field(ahci_cmd_header, prdtl, 0x02, 2);
dv_field(ahci_cmd_header, prdbc, 0x04, 4);
dv_field(ahci_cmd_header, ctba, 0x08, 4);
dv_field(ahci_cmd_header, ctbau, 0x0C, 4);
dv_field(ahci_cmd_header, rsv1, 0x10, 16);

dv_layout(ahci_fis_reg_h2d, 0x14);
dv_field(ahci_fis_reg_h2d, fis_type, 0x00, 1);
dv_field(ahci_fis_reg_h2d, command, 0x02, 1);
dv_field(ahci_fis_reg_h2d, featurel, 0x03, 1);
dv_field(ahci_fis_reg_h2d, lba0, 0x04, 1);
dv_field(ahci_fis_reg_h2d, lba1, 0x05, 1);
dv_field(ahci_fis_reg_h2d, lba2, 0x06, 1);
dv_field(ahci_fis_reg_h2d, device, 0x07, 1);
dv_field(ahci_fis_reg_h2d, lba3, 0x08, 1);
dv_field(ahci_fis_reg_h2d, lba4, 0x09, 1);
dv_field(ahci_fis_reg_h2d, lba5, 0x0A, 1);
dv_field(ahci_fis_reg_h2d, featureh, 0x0B, 1);
dv_field(ahci_fis_reg_h2d, countl, 0x0C, 1);
dv_field(ahci_fis_reg_h2d, counth, 0x0D, 1);
dv_field(ahci_fis_reg_h2d, icc, 0x0E, 1);
dv_field(ahci_fis_reg_h2d, control, 0x0F, 1);
dv_field(ahci_fis_reg_h2d, reserved2, 0x10, 4);

dv_layout(ahci_fis_reg_d2h, 0x14);
dv_field(ahci_fis_reg_d2h, fis_type, 0x00, 1);
dv_field(ahci_fis_reg_d2h, status, 0x02, 1);
dv_field(ahci_fis_reg_d2h, error, 0x03, 1);
dv_field(ahci_fis_reg_d2h, lba0, 0x04, 1);
dv_field(ahci_fis_reg_d2h, lba1, 0x05, 1);
dv_field(ahci_fis_reg_d2h, lba2, 0x06, 1);
dv_field(ahci_fis_reg_d2h, device, 0x07, 1);
dv_field(ahci_fis_reg_d2h, lba3, 0x08, 1);
dv_field(ahci_fis_reg_d2h, lba4, 0x09, 1);
dv_field(ahci_fis_reg_d2h, lba5, 0x0A, 1);
dv_field(ahci_fis_reg_d2h, rsv2, 0x0B, 1);
dv_field(ahci_fis_reg_d2h, countl, 0x0C, 1);
dv_field(ahci_fis_reg_d2h, counth, 0x0D, 1);
dv_field(ahci_fis_reg_d2h, rsv3, 0x0E, 2);
dv_field(ahci_fis_reg_d2h, rsv4, 0x10, 4);

dv_layout(ahci_port, 0x80);
dv_field(ahci_port, clb, 0x00, 4);
dv_field(ahci_port, clbu, 0x04, 4);
dv_field(ahci_port, fb, 0x08, 4);
dv_field(ahci_port, fbu, 0x0C, 4);
dv_field(ahci_port, is, 0x10, 4);
dv_field(ahci_port, ie, 0x14, 4);
dv_field(ahci_port, cmd, 0x18, 4);
dv_field(ahci_port, rsv0, 0x1C, 4);
dv_field(ahci_port, tfd, 0x20, 4);
dv_field(ahci_port, sig, 0x24, 4);
dv_field(ahci_port, ssts, 0x28, 4);
dv_field(ahci_port, sctl, 0x2C, 4);
dv_field(ahci_port, serr, 0x30, 4);
dv_field(ahci_port, sact, 0x34, 4);
dv_field(ahci_port, ci, 0x38, 4);
dv_field(ahci_port, sntf, 0x3C, 4);
dv_field(ahci_port, fbs, 0x40, 4);
dv_field(ahci_port, rsv1, 0x44, 44);
dv_field(ahci_port, vendor, 0x70, 16);
