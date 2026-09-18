/* @title: Master Boot Record */
#pragma once
#include <compiler/core.h>
#include <stdint.h>

struct mbr_partition_entry {
    uint8_t status;
    uint8_t chs_first[3];
    uint8_t type;
    uint8_t chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} cc_packed;

struct mbr {
    uint8_t bootstrap[446];
    struct mbr_partition_entry partitions[4];
    uint16_t signature;
} cc_packed;
_Static_assert(sizeof(struct mbr) == 512, "");
