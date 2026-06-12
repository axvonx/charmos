/* @title: Simple allocator */
#include <stddef.h>

struct vas;
void *simple_alloc(struct vas *space, size_t size);
void simple_free(struct vas *space, void *ptr, size_t size);
