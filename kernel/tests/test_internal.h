#pragma once
#include <test/test.h>

#include <atomic.h>
#include <crypto/prng.h>
#include <log.h>
#include <mem/alloc.h>
#include <sch/sched.h>
#include <smp/core.h>
#include <stack_depot.h>
#include <string.h>
#include <thread/thread.h>

#include <parse.h>

TEST_GROUP_DECLARE(log);
TEST_GROUP_DECLARE(stack_depot);
TEST_GROUP_DECLARE(parse);
TEST_GROUP_DECLARE(string);
