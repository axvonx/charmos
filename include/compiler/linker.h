/* @title: Linker macros */
#pragma once
#include <compiler/core.h>

#define cl_section(n) cc_section(".kernel_" #n) cc_used
#define cl_object(type, section) cl_section(section) type
#define cl_extern(type, name)                                                  \
    extern type __skernel_##name[];                                            \
    extern type __ekernel_##name[];

#define cs_paged cl_section(text_paged)
#define cs_init cl_section(text_init)
#define cs_data_init cl_section(data_init)
#define cs_ro_after_init cl_section(data_ro_after_init)
