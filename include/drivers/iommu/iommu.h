/* @title: IOMMU */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

struct iommu_domain;
struct iommu;
struct device;

enum iommu_perms {
    IOMMU_READ = 1u << 0,
    IOMMU_WRITE = 1u << 1,
    IOMMU_NOEXEC = 1u << 2,
};

enum iommu_error {
    IOMMU_ERR_OK,
    IOMMU_ERR_NO_MEM,
    IOMMU_ERR_UNSUPPORTED,
    IOMMU_ERR_INVALID,
};

enum iommu_status { IOMMU_STATUS_INACTIVE, IOMMU_STATUS_ACTIVE };

struct iommu_ops {
    enum iommu_error (*unit_init)(struct iommu *unit);
    void (*unit_destroy)(struct iommu *unit);

    struct iommu_domain *(*domain_alloc)(struct iommu *unit);
    void (*domain_free)(struct iommu_domain *domain);

    enum iommu_error (*attach_device)(struct iommu_domain *domain, uint16_t seg,
                                      uint8_t bus, uint8_t dev, uint8_t fn);
    void (*detach_device)(struct iommu_domain *domain, uint16_t seg,
                          uint8_t bus, uint8_t dev, uint8_t fn);

    enum iommu_error (*map)(struct iommu_domain *domain, iova_t iova,
                            paddr_t paddr, size_t size, enum iommu_perms perm);
    void (*unmap)(struct iommu_domain *domain, iova_t iova, size_t size);

    void (*flush_iotlb_domain)(struct iommu_domain *domain);
    void (*flush_iotlb_range)(struct iommu_domain *domain, iova_t iova,
                              size_t size);

    enum iommu_error (*enable)(struct iommu *unit);
    void (*disable)(struct iommu *unit);

    const char *name;
};

struct iommu {
    const struct iommu_ops *ops;
    void *private;
    enum iommu_status status;
};

struct iommu_domain {
    struct iommu *unit;
    void *priv;
    iova_t iova_base;
    iova_t iova_limit;
    struct vas_space *vas;
};

void iommu_init();

enum iommu_error iommu_unit_init(struct iommu *unit);
void iommu_unit_destroy(struct iommu *unit);

struct iommu_domain *iommu_domain_alloc(struct iommu *unit);
void iommu_domain_free(struct iommu_domain *domain);

enum iommu_error iommu_attach_device(struct iommu_domain *domain,
                                     struct device *dev);
void iommu_detach_device(struct iommu_domain *domain, struct device *dev);

enum iommu_error iommu_map(struct iommu_domain *domain, iova_t iova,
                           uint64_t pa, size_t size, enum iommu_perms perm);
void iommu_unmap(struct iommu_domain *domain, iova_t iova, size_t size);
