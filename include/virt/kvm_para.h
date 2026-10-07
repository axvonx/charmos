/* @title: KVM Paravirtual ABI */
#pragma once

#define KVM_CPUID_SIGNATURE "KVMKVMKVM\0\0\0"

/* CPUID offsets */
#define KVM_CPUID_FEATURES_OFF 1
#define KVM_FEATURE_PV_SEND_IPI 11

/* Hypercall numbers, passed in RAX to vmcall/vmmcall */
#define KVM_HC_SEND_IPI 10

/* Various error codes... TODO: Consider enum? I only
 * need a few now just to do the simple stuff */
#define KVM_EPERM 1
#define KVM_E2BIG 7
#define KVM_EFAULT 14
#define KVM_EINVAL 22
#define KVM_EOPNOTSUPP 95
#define KVM_ENOSYS 1000

/* KVM_HC_SEND_IPI needs a 128 bit APIC ID */
#define KVM_PV_IPI_WINDOW 128
