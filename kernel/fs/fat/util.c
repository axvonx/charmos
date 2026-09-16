#include <fs/fat.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time/time.h>

uint32_t fat_eoc(struct fat_fs *fs) {
    switch (fs->type) {
    case FAT_12: return 0x0FF8;
    case FAT_16: return 0xFFF8;
    case FAT_32: return 0x0FFFFFF8;
    }
    return 0;
}

bool fat_is_eoc(struct fat_fs *fs, uint32_t cluster) {
    switch (fs->type) {
    case FAT_12: return cluster >= 0x0FF8;
    case FAT_16: return cluster >= 0xFFF8;
    case FAT_32: return (cluster & 0x0FFFFFFF) >= 0x0FFFFFF8;
    }
    return true;
}

void fat_format_filename_83(const char *name, char out[11]) {
    static const size_t base_name_max = 8;
    static const size_t extension_max = 3;

    memset(out, ' ', 11);

    const char *dot = strchr(name, '.');
    size_t base_len = dot ? (size_t) (dot - name) : strlen(name);
    base_len = MIN(base_len, base_name_max);

    for (size_t i = 0; i < base_len; i++) {
        out[i] = toupper((unsigned char) name[i]);
    }

    if (dot) {
        size_t ext_len = MIN(strlen(dot + 1), extension_max);
        for (size_t i = 0; i < ext_len; i++) {
            out[8 + i] = toupper((unsigned char) dot[1 + i]);
        }
    }
}

uint32_t fat_get_dir_cluster(struct fat_dirent *d) {
    return ((uint32_t) d->high_cluster << 16) | d->low_cluster;
}

struct fat_time fat_get_current_time() {
    struct fat_time time = {.hour = time_get_hour(),
                            .minute = time_get_minute(),
                            .second = time_get_second() / 2};
    return time;
}

struct fat_date fat_get_current_date() {
    struct fat_date date = {
        .day = time_get_day(),
        .month = time_get_month(),
        .year = (time_get_year() + (time_get_century() * 100)) - 1980};
    return date;
}
