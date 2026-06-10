/* @title: Fixed Size Allocation */
#pragma once
#include <compiler.h>
#include <mem/alloc.h>
#include <mem/page.h>
#include <structures/list.h>
#include <sync/spinlock.h>

/* Number of fully-empty freelist backing pages to keep cached per local tree
 * before reclaiming on free
 *
 * Provides hysteresis so a workload that oscillates across a page boundary
 * does not pmm_alloc/pmm_free on every transition */
#define FIXED_SIZE_KEEP_EMPTY_PAGES 1

/* The idea here:
 *
 * memory is laid out as
 *
 * [ data ] [ potential empty ] [ fixed_size_node ]
 *
 * essentially, the objects have their own alignment, so each
 * of these "full nodes" comes with the data first, then the
 * empty zone. the fixed size node trails to link it all up
 */
struct fixed_size_node {
    struct list_head list_node;
};
static_assert(offsetof(struct fixed_size_node, list_node) == 0);

struct fixed_size_page_hdr {
    size_t free_count;
    size_t total;
    struct list_head page_list;
};

struct fixed_size_range_attributes {
    size_t obj_size;
    size_t obj_align;
    void (*init_obj)(void *);
    void (*deinit_obj)(void *);
    bool bootstrap_mode : 1;
};

struct fixed_size_range {
    struct fixed_size_range_attributes attrs;
    struct spinlock lock;
    struct list_head freelist;
    struct list_head fl_pages;
    size_t empty_pages;
    size_t full_node_size; /* node size + object size */
};

static_assert(sizeof(struct fixed_size_range) < PAGE_SIZE);

void *fixed_size_alloc(struct fixed_size_range *fsr);
void fixed_size_free(struct fixed_size_range *fsr, void *obj);
void fixed_size_reclaim_freelist_pages(struct fixed_size_range *fsr);
struct fixed_size_range *
fixed_size_range_create(struct fixed_size_range_attributes *attrs);
void fixed_size_range_init(struct fixed_size_range *fsr,
                           struct fixed_size_range_attributes *attrs);
