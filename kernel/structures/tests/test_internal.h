#pragma once
#include <test/test.h>

#include <crypto/prng.h>
#include <math/fixed.h>
#include <math/min_max.h>
#include <mem/alloc.h>
#include <structures/bitmap.h>
#include <structures/minheap.h>
#include <structures/rbit.h>
#include <structures/rbt.h>

#include <structures/avl.h>
#include <structures/bloom.h>
#include <structures/radix.h>
#include <structures/splay.h>
#include <structures/treap.h>

#include <structures/cpu_mask.h>
#include <structures/id_space.h>
#include <structures/mpmc_queue.h>
#include <structures/spsc_fifo.h>

TEST_GROUP_DECLARE(minheap);
TEST_GROUP_DECLARE(rbt);
TEST_GROUP_DECLARE(rbit);
TEST_GROUP_DECLARE(bitmap);
TEST_GROUP_DECLARE(radix);
TEST_GROUP_DECLARE(avl);
TEST_GROUP_DECLARE(bloom);
TEST_GROUP_DECLARE(splay);
TEST_GROUP_DECLARE(treap);
TEST_GROUP_DECLARE(mpmc_queue);
TEST_GROUP_DECLARE(spsc_fifo);
TEST_GROUP_DECLARE(id_space);
TEST_GROUP_DECLARE(cpu_mask);
