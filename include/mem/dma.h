/* @title: DMA */
#pragma once
#include <types/types.h>
struct device;

struct dma_region {
    iova_t iova; /* address the device uses */
    void *virt;  /* address the CPU uses     */
    size_t size;
};

struct dma_region dma_alloc(struct device *dev, size_t size);
void dma_free(struct device *dev, struct dma_region region);
