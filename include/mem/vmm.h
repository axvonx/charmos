/* @title: Virtual memory management */
#pragma once
#include <console/printf.h>
#include <err.h>
#include <mem/demand_page.h>
#include <mem/tlb.h>
#include <stdbool.h>
#include <stdint.h>
#include <types/types.h>
struct limine_executable_address_response;
struct limine_memmap_response;

struct page_table;

enum vmm_map_page_size : uint8_t {
    VMM_MAP_PAGE_SIZE_4KB,
    VMM_MAP_PAGE_SIZE_2MB,
    VMM_MAP_PAGE_SIZE_1GB,
};

/* Big bitmap set that is embedded in vmm_request */
enum vmm_flags : uint64_t {
    VMM_FLAG_NONE = 0,
    VMM_FLAG_USER = 1 << 1,
    VMM_FLAG_MODIFY_LEAF = 1 << 2, /* This flag exists since
                                    * certain calls simply serve
                                    * to modify the leaf (e.g. faulting
                                    * on a demand page and
                                    * mapping in the physical frame)
                                    *
                                    * By default, we panic if the
                                    * leaf is modified, and this
                                    * flag says "don't panic,
                                    * I know what I'm doing" */

    VMM_FLAG_CLEAR_LEAF = 1 << 3, /* Clear all the data (besides the lock bit,
                                   * transiently), zeroing out the leaf */

    VMM_FLAG_HANDLE_PTE_EXISTING = 1 << 4, /* Will return ERR_EXIST
                                            * when used in conjunction
                                            * with MODIFY_LEAF and PRESENT
                                            * is seen on the leaf PTE */
};

/* e.g. vmm_map_page(virt, phys, .page_flags = PAGE_PRESENT | PAGE_WRITE,
 *                                .page_size = VMM_MAP_PAGE_SIZE_2MB);
 * vmm_api_internal.h has the defaults */
struct vmm_request {
    page_flags_t page_flags;
    enum vmm_flags vmm_flags;
    enum vmm_map_page_size page_size;
    enum tlb_mode tlb_mode;
};

/* map/unmap page full get these, everyone else uses it to call into it,
 * wrappers automatically build these, everything's hunky dory! */
struct vmm_map_request {
    struct page_table *pml4;
    vaddr_t virt;
    paddr_t phys;
    size_t len;

    struct vmm_request rq;

    /* Internal flags */
    bool is_unmap_internal;
};

void vmm_init(struct limine_memmap_response *memmap,
              struct limine_executable_address_response *xa);

bool vmm_phys_is_kernel_text(paddr_t phys);

enum err vmm_map_aliased_full(vaddr_t virt, size_t len, paddr_t phys,
                              struct vmm_request rq);
enum err vmm_unshare_path_full(vaddr_t virt, struct vmm_request rq);

enum err vmm_map_page_full(vaddr_t virt, paddr_t phys, struct vmm_request rq);
enum err vmm_map_page_user_full(struct page_table *pml4, vaddr_t virt,
                                paddr_t phys, struct vmm_request rq);
void vmm_unmap_page_full(vaddr_t virt, struct vmm_request rq);
enum err vmm_mark_demand_page_full(vaddr_t virt, enum demand_page_flags flags,
                                   struct vmm_request rq);
enum err vmm_mark_demand_page_user_full(struct page_table *pml4, vaddr_t virt,
                                        enum demand_page_flags flags,
                                        struct vmm_request rq);
enum err vmm_map_demand_page_full(vaddr_t virt, paddr_t phys,
                                  enum demand_page_flags flags,
                                  struct vmm_request rq);

paddr_t vmm_get_phys_full(vaddr_t virt, struct vmm_request rq);
pte_t vmm_get_leaf_pte_full(vaddr_t virt, struct vmm_request rq);
void vmm_unmap_full(void *addr, uint64_t len, struct vmm_request rq);
void *vmm_map_full(paddr_t paddr, vaddr_t vaddr, uint64_t len,
                   struct vmm_request rq);
void *vmm_map_bump_full(uint64_t addr, uint64_t len, struct vmm_request rq);
uintptr_t vmm_make_user_pml4(void);
void vmm_unmap_all_user_pages_full(struct page_table *pml4,
                                   struct vmm_request rq);
void vmm_reclaim_page_tables(void);
struct page_table *vmm_phys_to_pml4(paddr_t paddr);

#include <mem/vmm_api_internal.h>
