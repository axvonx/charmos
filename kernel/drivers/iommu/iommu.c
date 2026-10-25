#include <acpi/dmar.h>
#include <drivers/iommu/iommu.h>

void iommu_init(void) {
    dmar_init();
}
