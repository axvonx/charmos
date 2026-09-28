#pragma once
#include <test/export.h>

struct arena;
TEST_IMPORT(struct arena_inmem_desc *, arena_get_inmem_descs, struct arena *a);
