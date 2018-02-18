/* author: Kexuan Zou
   date: 11/20/2017
      external source: http://elixir.free-electrons.com/linux/v3.0/source/arch/x86/include/asm/bitmap.h
*/

#ifndef _BITMAP_H
#define _BITMAP_H
#include <types.h>
#include <lib.h>
#include <system.h>

#define entry_offset(n) ((n) / BITS_LONG)
#define BIT_OFFSET(n) ((n) % BITS_LONG)

#define BITS_TO_LONG(n) \
    ((n + BITS_LONG - 1) / BITS_LONG)

#define BITMAP_LAST_ENTRY_MASK(nbits)               \
(                                                   \
    ((nbits) % BITS_LONG) ?                         \
        (1UL << ((nbits) % BITS_LONG)) - 1 : ~0UL   \
)

void bitmap_set_bit(uint32_t* bitmap, uint32_t bit);
void bitmap_clear_bit(uint32_t* bitmap, uint32_t bit);
uint32_t bitmap_query_bit(const uint32_t* bitmap, uint32_t bit);
int bitmap_empty(const uint32_t* bitmap, uint32_t num_bits);
int bitmap_full(const uint32_t* bitmap, uint32_t num_bits);
int bitmap_equal(const uint32_t* first, const uint32_t* second, uint32_t num_bits);
int find_free_region(uint32_t* bitmap, uint32_t num_bits, uint32_t order);
void release_region(uint32_t* bitmap, uint32_t pos, uint32_t order);
int allocate_region(uint32_t* bitmap, uint32_t pos, uint32_t order);


/* bitmap_clear
   description: clear specific region of a bitmap
   input: bitmap - pointer to the bitmap
          num_bits - number of bits to clear
   output: none
   return value: none
   side effect: modify bitmap
*/
static inline void bitmap_clear(uint32_t* bitmap, uint32_t num_bits) {
    if (num_bits <= BITS_LONG)
        bitmap[0] = 0UL;
    else {
        int len = BITS_TO_LONG(num_bits) * LONG_SIZE;
        memset(bitmap, 0, len);
    }
}


/* bitmap_set
   description: set specific region of a bitmap
   input: bitmap - pointer to the bitmap
          num_bits - number of bits to clear
   output: none
   return value: none
   side effect: modify bitmap
*/
static inline void bitmap_set(uint32_t* bitmap, uint32_t num_bits) {
    int entries = BITS_TO_LONG(num_bits);
    if (entries > 1) {
        int len = (entries - 1) * LONG_SIZE;
        memset(bitmap, 0xFF, len);
    }
    bitmap[entries - 1] = BITMAP_LAST_ENTRY_MASK(num_bits);
}


/* bitmap_copy
   description: copy specified bits from src bitmap to dest bitmap
   input: dest - dest bitmap
          src - src bitmap
          num_bits - number of bits to copy
   output: none
   return value: none
   side effect: modify dest bitmap
*/
static inline void bitmap_copy(uint32_t* dest, const uint32_t* src, uint32_t num_bits) {
    if (num_bits <= BITS_LONG)
        dest[0] = src[0];
    else {
        int len = BITS_TO_LONG(num_bits) * LONG_SIZE;
        memcpy(dest, src, len);
    }
}

#endif
