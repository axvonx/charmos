/* @title: Block Devices */
#pragma once
#include <block/bcache.h>
#include <fs/detect.h>
#include <sch/sched.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sync/spinlock.h>
#include <thread/thread.h>

struct block_device;
struct bio_request;

enum bdev_type {
    BDEV_IDE_DRIVE,
    BDEV_NVME_DRIVE,
    BDEV_AHCI_DRIVE,
    BDEV_ATAPI_DRIVE,
};

static inline const char *get_block_device_str(enum bdev_type type) {
    switch (type) {
    case BDEV_IDE_DRIVE: return "IDE DRIVE";
    case BDEV_NVME_DRIVE: return "NVME DRIVE";
    case BDEV_AHCI_DRIVE: return "AHCI CONTROLLER";
    case BDEV_ATAPI_DRIVE: return "ATAPI DRIVE";
    }
    return "UNKNOWN DEVICE";
}

struct partition {
    struct block_device *disk;
    uint64_t start_lba;
    uint64_t sector_count;
    enum fs_type fs_type;
    void *fs_data;
    char name[16];
    bool mounted;

    struct vfs_node *(*mount)(struct partition *);
};

enum bdev_flags {
    /* queue reordering is skipped */
    BDEV_FLAG_NO_REORDER = 1,

    /* coalescing is skipped */
    BDEV_FLAG_NO_COALESCE = 1 << 1,

    /* scheduling doesn't happen.
     * this will just call sync
     * requests, and immediately
     * trigger the callback - used
     * in things like RAMdisk. */
    BDEV_FLAG_NO_SCHED = 1 << 2,
};

struct block_device {
    enum bdev_flags flags;
    enum bdev_type type;
    enum fs_type fs_type;
    void *fs_data;
    char name[16];
    uint64_t total_sectors;
    bool is_removable;
    void *driver_data;
    uint32_t sector_size;

    /* both of these take full priority over the async operations.
     * do not pass go, do not collect two hundred dollars, submit instantly.
     *
     * these are sync and blocking
     *
     * these are not used in many areas though, and such, we can get away
     * with instant submission for the most part*/
    bool (*read_sector)(struct block_device *disk, uint64_t lba,
                        uint8_t *buffer, uint64_t sector_count);

    bool (*write_sector)(struct block_device *disk, uint64_t lba,
                         const uint8_t *buffer, uint64_t sector_count);

    /* immediate asynchronous submission */
    bool (*submit_bio_async)(struct block_device *disk,
                             struct bio_request *bio);

    struct bio_scheduler_ops *ops;
    struct bio_scheduler *scheduler;
    struct bcache *cache;
    uint64_t partition_count;
    struct partition *partitions;
};

static inline bool bdev_skip_coalesce(struct block_device *disk) {
    return disk->flags & BDEV_FLAG_NO_COALESCE;
}

static inline bool bdev_skip_sched(struct block_device *disk) {
    return disk->flags & BDEV_FLAG_NO_SCHED;
}

static inline bool bdev_skip_reorder(struct block_device *disk) {
    return disk->flags & BDEV_FLAG_NO_REORDER;
}
