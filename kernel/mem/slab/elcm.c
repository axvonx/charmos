#include <mem/elcm.h>

#include "internal.h"

struct slab_elcm_candidate slab_elcm(size_t obj_size, size_t obj_alignment) {
    kassert(obj_size && obj_alignment);
    struct elcm_params params = {
        .metadata_size_bytes = sizeof(struct slab),
        .metadata_bits_per_obj = 1,
        .obj_size = obj_size,
        .obj_alignment = obj_alignment,
        .bias_towards_pow2 = true,
        .max_wastage_pct = SLAB_ELCM_DEFAULT_MAX_WASTAGE_PCT,
        .max_pages = SLAB_MAX_PAGES,
        .metadata_bytes_per_page = sizeof(struct page *),
    };

    if (elcm(&params) != ERR_OK)
        return (struct slab_elcm_candidate){.pages = 0, .bitmap_size_bytes = 0};

    struct elcm_candidate elc = params.out;
    return (struct slab_elcm_candidate){.pages = elc.pages,
                                        .bitmap_size_bytes = elc.bitmap_bytes,
                                        .obj_count = elc.obj_count};
}
