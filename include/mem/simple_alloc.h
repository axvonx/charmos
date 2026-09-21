/* @title: Simple allocator */
#include <compiler/wrapper.h>
#include <stddef.h>

struct vas;
void *simple_alloc(struct vas *space, size_t size) cw_alloc(2);
void simple_free(struct vas *space, void *ptr, size_t size);
