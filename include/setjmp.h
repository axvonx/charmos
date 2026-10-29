/* @title: setjmp */
#pragma once
#include <compiler/core.h>
#include <setjmp.h>
#include <types/types.h>

typedef uint64_t jmp_buf[8];
cc_naked int setjmp(jmp_buf env);
cc_naked void longjmp(jmp_buf env, int val);
