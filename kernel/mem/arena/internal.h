#include <mem/arena.h>

struct arena_layout_desc {
    uint8_t total_header_size;
    int8_t offset_tag;
    int8_t offset_depot;
    int8_t offset_canary;
};
