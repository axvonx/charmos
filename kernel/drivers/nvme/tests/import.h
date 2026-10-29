#pragma once
#include <block/bio.h>
#include <test/export.h>
#include <types/types.h>

TEST_IMPORT(enum bio_request_status, nvme_to_bio_status, uint16_t status_word);
