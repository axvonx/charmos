/* @title: CPUID Definitions */
#pragma once
#include <math/bit.h>

#define CPUID_LEAF_VENDOR 0x0
#define CPUID_VENDOR_EBX_AMD 0x68747541   /* "Auth" */
#define CPUID_VENDOR_EBX_HYGON 0x6f677948 /* "Hygo" */

/* Leaf 1: feature flags */
#define CPUID_LEAF_FEATURES 0x1
#define CPUID_ECX_X2APIC BIT(21)
#define CPUID_ECX_HYPERVISOR BIT(31)

/* Hypervisor leaves */
#define CPUID_HV_LEAF_BASE 0x40000000
#define CPUID_HV_LEAF_LIMIT 0x40010000
#define CPUID_HV_LEAF_STRIDE 0x100
#define CPUID_HV_SIGNATURE_LEN 12
