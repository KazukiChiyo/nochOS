/* mem.c - kernel memory allocation and deallocation. Actual layout of a allocated object looks like this:
+--------+----------------------+
| header | data starts here ... |
+--------+----------------------+
         |
   malloc pointer
   author: Kexuan Zou
   date: 11/20/2017
*/

#include <mem.h>
#include <bitmap.h>
#include <lib.h>
#include <types.h>
#include <paging.h>
#include <bitops.h>
#include <system.h>

alloc_t alloc_info; // a global instance of the struct
uint32_t heap_map[HEAP_BITMAP_ENTRY]; // heap bitmap

/* alloc_request_order
   description: get the order of pages to allocate
   input: size - size of the object to allocate in bytes
   output: none
   return value: order of pages to allocate, -1 if size is 0 byte
   side effect: none
*/
int alloc_request_order(uint32_t size) {
    int size_raw = request_size_raw(size);
    int ffs_count = ffs(size_raw); // obtain first set bit
    int fls_count = fls(size_raw); // obtain last set bit
    return (ffs_count == fls_count) ? fls_count - 1 : fls_count; // round up log base 2 of raw page size
}


/* create_heap_pages
   description: create heap page(s) contiguously within heap block.
   input: start_idx - starting index of page to create
          num_pages - number of pages to create
   output: none
   return value: starting address of the first page
   side effect: none
*/
uint32_t create_heap_pages(int start_idx, int num_pages) {
    int i = 0, idx = start_idx; // loop iterators
    for (; i < num_pages; i++, idx++) // setup each 4kb page
        set_virtual_4kb_heap(HEAP_PHYS_ADDR(idx), HEAP_VIRT_ADDR(idx));
    return (uint32_t)(HEAP_VIRT_ADDR(start_idx));
}


/* destroy_heap_pages
   description: destroy heap page(s) contiguously within heap block.
   input: start_idx - starting index of page to create
          num_pages - number of pages to create
   output: none
   return value: none
   side effect: none
*/
void destroy_heap_pages(int start_idx, int num_pages) {
    int i = 0; // loop iterator
    for (; i < num_pages; i++, start_idx++) // clear each 4kb page
        free_virtual_4kb_heap(HEAP_VIRT_ADDR(start_idx));
}


/* alloc_request_page
   description: find free region within heap bitmap and allocate the page(s), update usage info
   input: size - size to allocate (header + data object size)
   output: none
   return value: starting address of the first page
   side effect: none
*/
uint32_t alloc_request_pages(uint32_t size) {
    int order = alloc_request_order(size);
    int num_pages = ORDER_TO_SIZE(order); // number of pages to set
    int start_idx = find_free_region(heap_map, HEAP_ENTRY, order); // find free pages and set in bitmap
    if (start_idx == -ENOMEM) return ENOMEM; // if out of memory, return
    uint32_t start_addr = create_heap_pages(start_idx, num_pages); // create heap pages, get starting page address
    alloc_info.page_alloc += num_pages; // set number of pages allocated
    alloc_info.obj_alloc++; // set number of objects allocated
    return start_addr;
}


/* free_request_pages
   description: free the allocated pages, update usage info
   input: start_idx - starting index of page to free
          size - size to free (header + data object size)
   output: none
   return value: none
   side effect: none
*/
void free_request_pages(uint32_t start_idx, uint32_t size) {
    int order = alloc_request_order(size);
    int num_pages = ORDER_TO_SIZE(order); // number of pages to set
    release_region(heap_map, start_idx, order); // find free pages and set in bitmap
    destroy_heap_pages(start_idx, num_pages); // destroy heap pages
    alloc_info.page_alloc -= num_pages; // clear number of pages allocated
    alloc_info.obj_alloc--; // clear number of objects allocated
}


/* __alloc
   description: given the size of a object, dynamically allocate a heap space for it.
   input: size - size of the object to allocate
   output: none
   return value: pointer to the object if success; null if fail
   side effect: none
*/
void* __alloc(uint32_t size) {
    uint32_t total_size = HEAP_HEADER_SIZE + size; // calculate total size of the requested size (header + data object size)

    /* if size is 0 or exceed maximum pages that can be allocated */
    if (size == 0 || REQ_PAGE_NUM(size) > MAX_HEAP_ENTRY)
        return NULL;

    /* request page(s) for object and obtain the pointer. if success, load size of the object in the header field */

    uint32_t page_ptr = alloc_request_pages(total_size);
    if (page_ptr == ENOMEM) return NULL; // if out of memory
    *((uint32_t* )page_ptr) = total_size; // load total size into header

    /* return the data pointer */
    return (void*)(HEAP_PAGE_TO_DATA(page_ptr));
}


/* __free
   description: free a dynamically allocated region, given its object pointer
   input: ptr - object pointer
   output: none
   return value: none
   side effect: none
*/
void __free(void* ptr) {
    if (ptr == NULL) return; // if pointer is invalid
    /* destroy page(s) for object */
    uint32_t total_size = malloc_info(ptr); // read total size of the object
    uint32_t start_idx = get_alloc_idx(ptr); // get starting index from pointer
    free_request_pages(start_idx, total_size);
}


/**
 * malloc - dynamically allocate space
 * @param size - size of the object to allocate
 * @return - void pointer to the object if success; null if fail
 */
void* malloc(uint32_t size) {
    return __alloc(size);
}


/**
 * calloc - dynamically allocate space and set to zero
 * @param size - size of the object to allocate
 * @return - void pointer to the object if success; null if fail
 */
void* calloc(uint32_t size) {
    void* mem_ptr = __alloc(size);
    if (mem_ptr == NULL) return mem_ptr;
    memset(mem_ptr, 0L, size);
    return mem_ptr;
}


/**
 * realloc - reallocate a previously allocated object
 * @param ptr - pointer to the object allocated
 * @param size - size of the object to reallocate
 * @return - void pointer to the object if success; null if fail
 */
void* realloc(void* ptr, uint32_t size) {
    void* new_ptr = __alloc(size);
    if (new_ptr == NULL) return new_ptr;
    uint32_t old_data_size = malloc_info(ptr) - HEAP_HEADER_SIZE;
    uint32_t cpysize = (old_data_size < size) ? old_data_size : size;
    memcpy(new_ptr, ptr, cpysize);
    __free(ptr);
    return new_ptr;
}


/**
 * free - free a dynamically allocated region
 * @param ptr - object pointer
 */
void free(void* ptr) {
    __free(ptr);
}


/* malloc_info
   description: obtain the info relative to the specified object
   input: ptr - object pointer
   output: none
   return value: header packed in a uint32_t format
   side effect: none
*/
uint32_t malloc_info(void* ptr) {
    uint32_t* header_ptr = (uint32_t* )HEAP_DATA_TO_PAGE((uint32_t)ptr);
    return *(header_ptr);
}


/* heap_init
   description: initialize heap page initial sequence
   input: none
   output: none
   return value: none
   side effect: none
*/
void heap_init(void) {
    bitmap_clear(heap_map, HEAP_ENTRY); // clear heap bitmap
    alloc_info.page_alloc = alloc_info.obj_alloc = 0; // clear heap struct
    set_virtual_4mb_heap(HEAP_VIRT_TOP); // setup page for heap
}
