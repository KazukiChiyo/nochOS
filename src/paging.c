/* paging.c - Functions for initializing and manipulating paging structures.
   author: Zhengcheng Huang, Kexuan Zou
   date: 10/19/2017, (revision) 10/30/2017
   external source: wiki.osdev.org/Paging
 */

#include <paging.h>
#include <system.h>

#define VIDEO 0xB8000

/// The page directory used by the paging unit.
/// Must be aligned because CR3 can only point to addresses that are
/// multiples of a page size (4kB)
uint32_t page_directory [NUM_PDE] __attribute__((aligned(PAGE_SIZE)));

/// The first page table; covers the first 4MB of physical memory.
/// Must be aligned because PDEs can only point to addresses that are
/// multiples of a page size (4kB)
uint32_t first_page_table [NUM_PTE] __attribute__((aligned(PAGE_SIZE)));

// program page table; used to house 4kb pages
uint32_t prog_page_table[NUM_PTE] __attribute__((aligned(PAGE_SIZE)));

// heap page table; 1024 4kb pages are assigned for malloc
uint32_t heap_page_table[NUM_PTE] __attribute__((aligned(PAGE_SIZE)));

/* map_virtual_4mb
   description: map a physical 4mb page to virtual page
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory
   author: Kexuan Zou
*/
void map_virtual_4mb(uint32_t phys_start, uint32_t virt_start) {
    uint32_t pde_idx = virt_start >> 22;
    /* enable P flag, R/W flag, U/S flag, PS flag for the page */
    page_directory[pde_idx] = (phys_start & 0xFFC00000 & ~EN_A) | EN_P | EN_RW | EN_US | EN_PS;
    flush_tlb();
}


/* map_virtual_4kb_first
   description: map a physical 4kb page to virtual page, whose page table entry resides in first_page_table
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void map_virtual_4kb_first(uint32_t phys_start, uint32_t virt_start) {
    uint32_t pde_idx = virt_start >> 22;
    uint32_t pte_idx = (virt_start << 10) >> 22;
    /* enable P flag, R/W flag, U/S flag, PS flag for the page */
    page_directory[pde_idx] = ((uint32_t)first_page_table & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW;
    first_page_table[pte_idx] = (phys_start & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW;
    flush_tlb();
}


/* map_virtual_4kb_prog
   description: map a physical 4kb page to virtual page, whose page table entry resides in prog_page_table
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void map_virtual_4kb_prog(uint32_t phys_start, uint32_t virt_start) {
    uint32_t pde_idx = virt_start >> 22;
    uint32_t pte_idx = (virt_start << 10) >> 22;
    /* enable P flag, R/W flag, U/S flag, PS flag for the page */
    page_directory[pde_idx] = ((uint32_t)prog_page_table & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW | EN_US;
    prog_page_table[pte_idx] = (phys_start & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW | EN_US;
    flush_tlb();
}


/* set_virtual_4mb_heap
   description: set page directory entry for heap
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void set_virtual_4mb_heap(uint32_t virt_start) {
    uint32_t pde_idx = virt_start >> 22;
    /* enable P flag, R/W flag, U/S flag, PS flag for the page */
    page_directory[pde_idx] = ((uint32_t)heap_page_table & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW | EN_US;
    flush_tlb();
}


/* set_virtual_4kb_heap
   description: set a physical 4kb page to virtual page, whose page table entry resides in heap_page_table
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void set_virtual_4kb_heap(uint32_t phys_start, uint32_t virt_start) {
    uint32_t pte_idx = (virt_start << 10) >> 22;
    /* enable P flag, R/W flag, U/S flag, PS flag for the page */
    heap_page_table[pte_idx] = (phys_start & 0xFFFFF000 & ~EN_A) | EN_P | EN_RW | EN_US;
    flush_tlb();
}


/* free_virtual_4kb_heap
   description: free a 4kb page in heap; this is just to clear the present bit
   input: phys_start - starting address of physical 4mb page
          virt_start - starting address of virtual 4mb page
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void free_virtual_4kb_heap(uint32_t virt_start) {
    uint32_t pte_idx = (virt_start << 10) >> 22;
    heap_page_table[pte_idx] &= ~EN_P;
    flush_tlb();
}


/* setup_utility_page
   description: set up utility page in a one-to-one fashion, which is mainly used to obtain struct offset in list.h
   input: none
   output: none
   return value: none
   side effect: Modifies the page directory and page table
   author: Kexuan Zou
*/
void setup_utility_page(void) {
    map_virtual_4kb_first(UTIL_ADDR, UTIL_ADDR);
}


///
/// Fill the page table that covers the first 4MB in memory.
/// This table is referred to as the first page table.
///
/// - author: Zhengcheng Huang
/// - side effects: Fills the first page table with default values.
///                 Sets PDE corresponding to this page table.
///
void setup_first_page_table(void) {
    int i;
    for (i = 0; i < NUM_PTE; i++) {
        // Set the second lowest bit to indicate a R/W page.
        // All pages are R/W by default.
        first_page_table[i] = (i * PAGE_SIZE) | EN_RW;
    }

    // Set PDE pointing to this table
    // Set bit 0 (present flag) of corresponding PDE.
    // Set bit 1 (R/W flag) of corresponding PDE.
    page_directory[0] = (uint32_t)first_page_table | EN_P | EN_RW;
}

///
/// Setup page for kernel. This page is 4MB in size, thus directly pointed
/// by a PDE.
///
/// - author: Zhengcheng Huang
/// - side effects: Sets index 1 PDE to point to the kernel page.
///
void setup_kernel_page(void) {
    // Address of kernel page is 4MB, which is 400000 in hex.
    // Set bit 0 (present flag) of the kernel page.
    // Set bit 1 (R/W flag) of the kernel page.
    // Set bit 7 (4MB page flag) of the kernel page.
    page_directory[1] = 0x00400000 | EN_P | EN_RW | EN_PS;
}

///
/// Setup page for video memory. This page is 4kB in size and exists in the
/// first 4MB, so its corresponding PTE in in the first page table.
///
/// - author: Zhengcheng Huang
/// - side effects: Sets corresponding PTE to point to video memory page.
///
void setup_video_page(void) {
    int i;

    // Calculate index of PTE of video memory by taking the middle 10 bytes
    // bits[21,12].
    i = (VIDEO >> 12) & 0x3FF;

    // Set bit 0 (present flag) and bit 1 (R/W flag) of video memory page.
    first_page_table[i] = VIDEO | EN_P | EN_RW;
}


///
/// Setup page for modex. 
///
/// - author: Zhengcheng Huang
/// - side effects: Sets corresponding PTE to point to modex page.
///
void setup_modex_page(void) {
    int i;

    for (i = 0; i < 32; ++i) {
      first_page_table[0x3FF & ((0xA0000 + (i << 12)) >> 12)] = (0xA0000 + (i << 12)) | EN_P | EN_RW;
    }
    flush_tlb();
}

///
/// Setup page for terminal history. This page is 4MB in size and exists in the
/// physical memory 36MB to 40MB, thus directly pointed by a PDE.
///
/// - author: ZZ
/// - side effects: Sets corresponding PTE to point to terminal history page.
///
void setup_history_page(void) {
    // Address of kernel page is 4MB, which is 240 0000 in hex.
    // Set bit 0 (present flag) of the kernel page.
    // Set bit 1 (R/W flag) of the kernel page.
    // Set bit 7 (4MB page flag) of the kernel page.
    page_directory[9] = 0x02400000 | EN_P | EN_RW | EN_PS;
}

///
/// Sets up paging by creating and initializing page table and page directory,
/// and enabling paging.
///
/// - author: Zhengcheng Huang
/// - side effects: Creates and initializes page directory and page table.
///                 Sets control registers to enable paging.
///
void setup_paging(void) {
    // Setup the first page table.
    setup_first_page_table();

    // set up the utility page
    setup_utility_page();

    // Create kernel page and video memory page.
    setup_kernel_page();
    setup_video_page();
    setup_history_page();

    // Enable paging.
    enable_paging();
}
