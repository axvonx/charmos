/* @title: IPIs */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <types/types.h>

struct cpu_mask;

enum ipi_mask_flags {
    IPI_MASK_EXACT = 0,            /* Only CPUs in the mask take the IPI */
    IPI_MASK_SUPERSET_OK = 1 << 0, /* Others may too */
};

/* Broadcast instead of exact if (sends_needed - 1) * 256
 * >= bystanders * IPI_BCAST_RATIO_Q8 where ratio is (bystander interrupt cost /
 * cost of one send) */
#define IPI_BCAST_RATIO_Q8 (16 * 256)

void ipi_send(cpu_id_t cpu, uint8_t vector);

/* Fails instead of waiting if a send is still in flight */
bool ipi_send_try(cpu_id_t cpu, uint8_t vector);

/* Picks shorthand / KVM PV / x2APIC cluster / physical per call */
void ipi_send_mask(const struct cpu_mask *targets, uint8_t vector,
                   enum ipi_mask_flags flags);

void nmi_send(cpu_id_t cpu);

/* NMIs every CPU except `self`, for the panic path */
void nmi_send_others(cpu_id_t self);
