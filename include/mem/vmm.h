/* @title: Virtual memory management */
#pragma once
#include <console/printf.h>
#include <errno.h>
#include <limine.h>
#include <stdint.h>
#include <types/types.h>

struct page_table;

enum vmm_flags {
    VMM_FLAG_NONE = 0,
    VMM_FLAG_NO_TLB_SHOOTDOWN = 1 << 0,
    VMM_FLAG_USER = 1 << 1,
};

void vmm_init(struct limine_memmap_response *memmap,
              struct limine_executable_address_response *xa);

enum errno vmm_map_page_full(struct page_table *pml4, vaddr_t virt,
                             paddr_t phys, uint64_t flags,
                             enum vmm_flags vflags);
enum errno vmm_map_page(uintptr_t virt, uintptr_t phys, uint64_t pflags,
                        enum vmm_flags vflags);
enum errno vmm_map_page_user(struct page_table *pml4, vaddr_t virt,
                             paddr_t phys, uint64_t flags,
                             enum vmm_flags vflags);

enum errno vmm_map_2mb_page_full(struct page_table *pml4, vaddr_t virt,
                                 paddr_t phys, uint64_t flags,
                                 enum vmm_flags vflags);
enum errno vmm_map_2mb_page(uintptr_t virt, uintptr_t phys, uint64_t flags,
                            enum vmm_flags vflags);
enum errno vmm_map_2mb_page_user(struct page_table *pml4, vaddr_t virt,
                                 paddr_t phys, uint64_t flags,
                                 enum vmm_flags vflags);

void vmm_unmap_2mb_page(uintptr_t virt, enum vmm_flags vflags);
void vmm_unmap_page(uintptr_t virt, enum vmm_flags vflags);
uintptr_t vmm_get_phys(uintptr_t virt, enum vmm_flags vflags);
void vmm_unmap(void *addr, uint64_t len, enum vmm_flags vflags);
void *vmm_map(paddr_t paddr, vaddr_t vaddr, uint64_t len, uint64_t flags,
              enum vmm_flags vflags);
void *vmm_map_bump(uint64_t addr, uint64_t len, uint64_t flags,
                   enum vmm_flags vflags);
void vmm_unmap_virt(void *addr, uint64_t len, enum vmm_flags vflags);
uintptr_t vmm_make_user_pml4(void);
void vmm_unmap_all_user_pages(struct page_table *pml4, enum vmm_flags vflags);
void vmm_reclaim_page_tables(void);
struct page_table *vmm_phys_to_pml4(paddr_t paddr);
