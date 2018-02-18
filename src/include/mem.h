/* author: Kexuan Zou
   date: 11/21/2017
*/

#ifndef _MEM_H
#define _MEM_H
#include <types.h>
#include <system.h>

#define HEAP_ENTRY 1024 // there are 1024 entries in the 4mb heap page
#define HEAP_BITMAP_ENTRY (HEAP_ENTRY / BITS_LONG) // bitmap for heap has 32 entries
#define HEAP_PAGE_SIZE 0x1000 // heap page size is 4kb
#define MAX_HEAP_ENTRY 32 // at most 32 contiguous pages can be allocated for a object
#define HEAP_HEADER_SIZE LONG_SIZE // heap header size is 4 bytes
#define HEAP_PHYS_BOT (HEAP_PHYS_TOP + HEAP_SIZE) // heap physical bottom address
#define HEAP_VIRT_BOT (HEAP_VIRT_TOP + HEAP_SIZE) // heap virtual bottom address

/* struct for heap management  */
typedef struct alloc_t {
    uint32_t page_alloc; // number of pages allocated
    uint32_t obj_alloc; // number of objects allocated
} __attribute__((packed)) alloc_t;

extern alloc_t alloc_info; // a global instance of the struct
extern uint32_t heap_map[HEAP_BITMAP_ENTRY]; // heap bitmap

#define REQ_PAGE_NUM(size) ((size) / HEAP_PAGE_SIZE)
#define REQ_SIZE_OFFSET(size) ((size) % HEAP_PAGE_SIZE)
#define ORDER_TO_SIZE(order) (1 << (order))

#define HEAP_VIRT_ADDR(idx) \
    (HEAP_VIRT_TOP + (idx) * HEAP_PAGE_SIZE)

#define HEAP_PHYS_ADDR(idx) \
    (HEAP_PHYS_TOP + (idx) * HEAP_PAGE_SIZE)

#define HEAP_PAGE_TO_DATA(addr) \
    ((addr) + LONG_SIZE)

#define HEAP_DATA_TO_PAGE(addr) \
    ((addr) - LONG_SIZE)

#define PAGE_PTR_TO_IDX(addr) \
    (((addr) - HEAP_VIRT_TOP) / HEAP_PAGE_SIZE)

/* private functions vibsible to this file */
int alloc_request_order(uint32_t size);
uint32_t create_heap_pages(int start_idx, int num_pages);
void destroy_heap_pagess(int start_idx, int num_pages);
uint32_t alloc_request_pages(uint32_t size);
void free_request_pages(uint32_t start_idx, uint32_t size);
void* __alloc(uint32_t size);
void __free(void* ptr);
extern void* malloc(uint32_t size);
extern void* calloc(uint32_t size);
extern void* realloc(void* ptr, uint32_t size);
extern void free(void* ptr);
extern uint32_t malloc_info(void* ptr);
extern void heap_init(void);

/* request_size_raw
   description: get number of 4kb heap pages requested (not aligned)
   input: size - size of the object to allocate in bytes
   output: none
   return value: order of pages to allocate
   side effect: none
*/
static inline int request_size_raw(uint32_t size) {
    int page_num = REQ_PAGE_NUM(size);
    return (REQ_SIZE_OFFSET(size)) ? page_num + 1 : page_num;
}


/* get_alloc_idx
   description: get page index from object pointer
   input: obj_ptr - object pointer to a allocated object
   output: none
   return value: starting page index
   side effect: none
*/
static inline uint32_t get_alloc_idx(void* obj_ptr) {
    uint32_t page_ptr = HEAP_DATA_TO_PAGE((uint32_t)obj_ptr);
    return PAGE_PTR_TO_IDX(page_ptr);
}

#endif
