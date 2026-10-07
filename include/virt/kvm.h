/* @title: KVM Guest */
#pragma once
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <virt/kvm_para.h>

bool kvm_detect(void);

bool kvm_has_feature(uint32_t feature);

/* Uses vmcall or vmmcall as the
 * CPU vendor requires. ERR_NO_DEV when not running on KVM,
 * returning other errors too.
 *
 * TODO: Consider error facility*/
err_checked kvm_hypercall4(uint64_t nr, uint64_t a0, uint64_t a1, uint64_t a2,
                           uint64_t a3, uint64_t *out);
