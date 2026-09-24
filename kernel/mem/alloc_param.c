#include <mem/alloc.h>

LINKER_SECTION_DEFINE(struct alloc_flag_ex_desc, alloc_flag_ex_descs);

size_t
alloc_flag_ex_get_desc(enum alloc_flags flag,
                       struct alloc_flag_ex_desc out[ALLOC_FLAG_EX_DESC_MAX]) {

    size_t idx = 0;
    struct alloc_flag_ex_desc *exd;

    linker_section_for_each_object(exd, alloc_flag_ex_descs) {
        if (flag & exd->flag) {
            out[idx++] = *exd;
            if (idx == ALLOC_FLAG_EX_DESC_MAX)
                return idx;
        }
    }

    return idx;
}
