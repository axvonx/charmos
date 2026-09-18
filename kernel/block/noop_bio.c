#include <block/sched.h>

bool noop_should_coalesce(struct block_device *disk,
                          const struct bio_request *a,
                          const struct bio_request *b) {
    cc_var_unused(disk, a, b);
    return false;
}

void noop_do_coalesce(struct block_device *disk, struct bio_request *into,
                      struct bio_request *from) {
    cc_var_unused(disk, into, from);
}

void noop_reorder(struct block_device *disk) {
    cc_var_unused(disk);
}
