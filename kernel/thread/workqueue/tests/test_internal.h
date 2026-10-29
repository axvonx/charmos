#pragma once
#include <mem/alloc.h>
#include <mem/nofail.h>
#include <sch/sched.h>
#include <string.h>
#include <test/test.h>
#include <thread/apc.h>
#include <thread/daemon.h>
#include <thread/reaper.h>
#include <thread/thread.h>
#include <thread/workqueue.h>
#include <time/spin_sleep.h>
#include <time/time.h>
#include <types/types.h>

#include "thread/workqueue/internal.h"

TEST_GROUP_EXTERN(workqueue);
TEST_GROUP_EXTERN(defer);
