/* @title: Simple allocator */
#include <stddef.h>

struct vas_space;
void *simple_alloc(struct vas_space *space, size_t size);
void simple_free(struct vas_space *space, void *ptr, size_t size);
