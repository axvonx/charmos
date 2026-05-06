/* @title: Registry */
#pragma once
#include <log.h>
#include <stdint.h>

struct block_device;
void registry_register(struct block_device *disk);
void registry_unregister(struct block_device *disk);
struct block_device *registry_get_by_name(const char *name);
struct block_device *registry_get_by_index(uint64_t index);
uint64_t registry_get_disk_cnt(void);
void registry_setup();
void registry_mkname(struct block_device *disk, const char *prefix,
                     uint64_t counter);

#define k_print_register(name)                                                 \
    log_msg(LOG_INFO, "Registering " ANSI_GREEN "%s" ANSI_RESET, name)
