#pragma once
#include <block/bio.h>
#include <block/block.h>
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <mem/alloc.h>
#include <test/test.h>
#include <time/spin_sleep.h>
#include <types/types.h>

#include "fs/detect.h"

TEST_GROUP_EXTERN(bio);
