/* TODO: spec citations per structure */

#include <drivers/ata.h>
#include <drivers/verifier.h>

#define DV_STRUCT ata_identify
dv_size(0x1FE);
dv_align(1);
dv_field(uint16_t, config, 0x00);
dv_field(uint16_t, cylinders, 0x02);
dv_field(uint16_t, reserved1, 0x04);
dv_field(uint16_t, heads, 0x06);
dv_field(uint16_t[2], vendor1, 0x08);
dv_field(uint16_t, sectors_per_track, 0x0C);
dv_field(uint16_t[3], vendor2, 0x0E);
dv_field(uint16_t[10], serial_number, 0x14);
dv_field(uint16_t[2], vendor3, 0x28);
dv_field(uint16_t, obsolete1, 0x2C);
dv_field(uint16_t[4], firmware_revision, 0x2E);
dv_field(uint16_t[20], model_number, 0x36);
dv_field(uint16_t, max_rw_multiple, 0x5E);
dv_field(uint16_t, reserved2, 0x60);
dv_field(uint16_t[2], capabilities, 0x62);
dv_field(uint16_t[2], obsolete2, 0x66);
dv_field(uint16_t, field_validity, 0x6A);
dv_field(uint16_t, current_cylinders, 0x6C);
dv_field(uint16_t, current_heads, 0x6E);
dv_field(uint16_t, current_sectors, 0x70);
dv_field(uint16_t, current_capacity_lo, 0x72);
dv_field(uint16_t, current_capacity_hi, 0x74);
dv_field(uint16_t, rw_multiple, 0x76);
dv_field(uint32_t, lba28_capacity, 0x78);
dv_field(uint16_t, dma_supported, 0x7C);
dv_field(uint16_t, advanced_pio_modes, 0x7E);
dv_field(uint16_t, min_dma_cycle_time, 0x80);
dv_field(uint16_t, recommended_dma_cycle_time, 0x82);
dv_field(uint16_t, min_pio_cycle_time, 0x84);
dv_field(uint16_t, min_pio_cycle_time_iordy, 0x86);
dv_field(uint16_t, additional_supported, 0x88);
dv_field(uint16_t[5], reserved3, 0x8A);
dv_field(uint16_t, queue_depth, 0x94);
dv_field(uint16_t, sata_capabilities, 0x96);
dv_field(uint16_t, sata_additional, 0x98);
dv_field(uint16_t, sata_features_supported, 0x9A);
dv_field(uint16_t, sata_features_enabled, 0x9C);
dv_field(uint16_t, major_version, 0x9E);
dv_field(uint16_t, minor_version, 0xA0);
dv_field(uint16_t[3], command_set_supported, 0xA2);
dv_field(uint16_t[3], command_set_enabled, 0xA8);
dv_field(uint16_t, features_supported_extension, 0xAE);
dv_field(uint16_t, security_erase_time, 0xB0);
dv_field(uint16_t, enhanced_security_erase_time, 0xB2);
dv_field(uint16_t, current_advanced_power_mgmt, 0xB4);
dv_field(uint16_t, master_password_revision, 0xB6);
dv_field(uint16_t, hardware_reset_result, 0xB8);
dv_field(uint16_t, acoustic_management, 0xBA);
dv_field(uint16_t, stream_min_req_size, 0xBC);
dv_field(uint16_t, stream_transfer_time_dma, 0xBE);
dv_field(uint16_t, stream_access_latency, 0xC0);
dv_field(uint32_t, streaming_performance_gran, 0xC2);
dv_field(uint64_t, lba48_sector_count, 0xC6);
dv_field(uint16_t, streaming_transfer_time, 0xCE);
dv_field(uint16_t, dsm_cap, 0xD0);
dv_field(uint16_t, phys_log_sector_size, 0xD2);
dv_field(uint16_t, inter_seek_delay, 0xD4);
dv_field(uint16_t[4], world_wide_name, 0xD6);
dv_field(uint16_t[144], reserved4, 0xDE);
#undef DV_STRUCT
