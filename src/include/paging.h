/* paging.h - Variables for accessing paging structures.
   author: Zhengcheng Huang
   date: 10/19/2017
   external source: wiki.osdev.org/Paging
 */

#ifndef _PAGING_H
#define _PAGING_H

#include <types.h>
#include <system.h>

#define NUM_PDE 1024
#define NUM_PTE 1024
#define PAGE_SIZE 4096
#define PROG_PAGE_SIZE 0x400000 // page size per program is 4mb
#define EN_P 0x00000001 // set present flag, bit 0, indicates page is loaded in physical memory
#define EN_RW 0x00000002 // set read/write flag, bit 1, specifies read-write privileges
#define EN_US 0x00000004 // set user/supervisor flag, bit 2, specifies user privilege level
#define EN_PS 0x00000080 // page size flag, bit 7, set 4mb page
#define EN_A 0x00000020 // set accessed flag, bit 5
#define UTIL_ADDR 0x0 // utility page uses 0x0

/// The page directory.
extern uint32_t page_directory [NUM_PDE];

/// The page table that covers the first 4MB of memory.
extern uint32_t first_page_table [NUM_PTE];

extern void map_virtual_4mb(uint32_t phys_start, uint32_t virt_start);
extern void map_virtual_4kb_first(uint32_t phys_start, uint32_t virt_start);
extern void map_virtual_4kb_prog(uint32_t phys_start, uint32_t virt_start);
extern void set_virtual_4mb_heap(uint32_t virt_start);
extern void set_virtual_4kb_heap(uint32_t phys_start, uint32_t virt_start);
extern void free_virtual_4kb_heap(uint32_t virt_start);
extern void flush_tlb(void);
extern void setup_utility_page(void);

/// Fills the first page table such that all flags of all PTEs are cleared,
/// and that consecutive PTEs points to consecutive pages.
extern void setup_first_page_table(void);

/// Setup page for the kernel.
extern void setup_kernel_page(void);


/// Setup page for the terminal history.
extern void setup_history_page(void);


/// Setup page for video memory.
extern void setup_video_page(void);

/// Set cr3 to point to page directory, and enable paging.
extern void enable_paging(void);

extern void setup_modex_page(void);

/// Setup paging
extern void setup_paging(void);

#endif
