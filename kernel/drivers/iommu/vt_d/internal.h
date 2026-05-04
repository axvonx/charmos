#include <asm.h>
#include <console/printf.h>
#include <drivers/iommu/vt_d.h>
#include <log.h>
#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vmm.h>

void vtd_write_gcmd(struct vtd_unit *u, uint32_t cmd);
void vtd_wait_gsts(struct vtd_unit *u, uint32_t bit, bool set);
enum iommu_error vtd_iq_init(struct vtd_unit *u);
void vtd_iq_submit(struct vtd_unit *u, struct vtd_inv_desc desc);
void vtd_iq_flush(struct vtd_unit *u);
enum iommu_error vtd_root_table_init(struct vtd_unit *u);
enum iommu_error vtd_root_table_init(struct vtd_unit *u);
