/* bitmap.c - bitmap library implementation. NOTE: We will be using unsigned long array as bitmap data structure.
   author: Kexuan Zou
   date: 11/19/2017
   external source: https://stackoverflow.com/questions/1225998/what-is-a-bitmap-in-c
   http://elixir.free-electrons.com/linux/v3.3/source/lib/bitmap.c
*/

#include <bitmap.h>
#include <lib.h>
#include <types.h>
#include <system.h>

/* bitmap_set_bit
   description: set a bit in the bitmap
   input: bitmap - pointer to the bitmap
          bit - bit index to set
   output: none
   return value: none
   side effect: modify bitmap
*/
void bitmap_set_bit(uint32_t* bitmap, uint32_t bit) {
    bitmap[entry_offset(bit)] |= (1 << BIT_OFFSET(bit));
}


/* bitmap_clear_bit
   description: clears a bit in the bitmap
   input: bitmap - pointer to the bitmap
          bit - bit index to clear
   output: none
   return value: none
   side effect: modify bitmap
*/
void bitmap_clear_bit(uint32_t* bitmap, uint32_t bit) {
    bitmap[entry_offset(bit)] &= ~(1 << BIT_OFFSET(bit));
}


/* bitmap_query_bit
   description: query the status of a single bit
   input: bitmap - pointer to the bitmap
          bit - bit index to query
   output: none
   return value: 0 if bit is cleared; 1 if bit is set
   side effect: none
*/
uint32_t bitmap_query_bit(const uint32_t* bitmap, uint32_t bit) {
    uint32_t ret_val = bitmap[entry_offset(bit)] & (1 << BIT_OFFSET(bit));
    return (ret_val != 0);
}


/* bitmap_empty
   description: check if a bitmap is empty
   input: bitmap - pointer to the bitmap
          num_bits - number of bits to check
   output: none
   return value: 1 if bitmap is empty, 0 if not
   side effect: none
*/
int bitmap_empty(const uint32_t* bitmap, uint32_t num_bits) {
    int i = 0, limit = entry_offset(num_bits);
    for (; i < limit; ++i) { // if contains any high bits return false
        if (bitmap[i])
            return 0;
    }

    /* if num_bits is not a multiple of 32 */
    if (BIT_OFFSET(num_bits)) {
        if (bitmap[i] & BITMAP_LAST_ENTRY_MASK(num_bits))
            return 0;
    }
    return 1;
}


/* bitmap_full
   description: check if a bitmap is full
   input: bitmap - pointer to the bitmap
          num_bits - number of bits to check
   output: none
   return value: 1 if bitmap is full, 0 if not
   side effect: none
*/
int bitmap_full(const uint32_t* bitmap, uint32_t num_bits) {
    int i = 0, limit = entry_offset(num_bits);
    for (; i < limit; ++i) { // if contains any high bits return false
        if (~bitmap[i])
            return 0;
    }

    /* if num_bits is not a multiple of 32 */
    if (BIT_OFFSET(num_bits)) {
        if (~bitmap[i] & BITMAP_LAST_ENTRY_MASK(num_bits))
            return 0;
    }
    return 1;
}


/* bitmap_equal
   description: check if two bitmaps are equal
   input: first - first bitmap
          second - second bitmap
          num_bits - number of bits to check
   output: none
   return value: 1 if bitmaps are equal, 0 if not
   side effect: none
*/
int bitmap_equal(const uint32_t* first, const uint32_t* second, uint32_t num_bits) {
    int i = 0, limit = entry_offset(num_bits);
    for (; i < limit; ++i) {
        if (first[i] != second[i])
            return 0;
    }

    /* if num_bits is not a multiple of 32 */
    if (BIT_OFFSET(num_bits)) {
        if ((first[i] ^ second[i]) & BITMAP_LAST_ENTRY_MASK(num_bits))
            return 0;
    }
    return 1;
}

enum {
    REG_OP_ISFREE, // check for contiguous zero bits
    REG_OP_ALLOC, // set specified bits
    REG_OP_RELEASE, // clear specified bits
};


/* _reg_op
   description: allocate, check, or release a region of bits in the given bitmap. the region consists of a sequence of bits in the map of variable size.
   input: bitmap - pointer to the bitmap
          pos - second bitmap
          order - order region size (log base 2 of number of bits requested). must align to power of 2s.
          reg_op - operation specifier
   output: none
   return value: 1 if REG_OP_ISFREE success; 0 in other cases.
   side effect: none
*/
static int _reg_op(uint32_t* bitmap, uint32_t pos, uint32_t order, int reg_op) {
    uint32_t num_bits_reg = 1 << order; // number of bits in region
    uint32_t index = entry_offset(pos); // entry index
    uint32_t offset = BIT_OFFSET(pos); // bit offset within that word
    uint32_t entries = BITS_TO_LONG(num_bits_reg); // number of entries in the bitmap
    uint32_t num_bits_entry = (num_bits_reg < BITS_LONG) ? num_bits_reg : BITS_LONG; // number of bits in each entry
    uint32_t bitmask;
    int i = 0;
    int ret_val = 0;

    /* obtain the bitmask for each entry */
    bitmask = (1UL << (num_bits_entry - 1));
    bitmask += bitmask - 1;
    bitmask <<= offset;

    /* switch cases for 3 operations */
    switch(reg_op) {
        /* check for contiguous zero bits */
        case REG_OP_ISFREE:
            for (; i < entries; i++) {
                if (bitmap[index + i] & bitmask)
                    goto done;
            }
            ret_val = 1; // REG_OP_ISFREE success
            break;

        /* allocate freed region and set these bits */
        case REG_OP_ALLOC:
            for (; i < entries; i++)
                bitmap[index + i] |= bitmask;
            break;

        /* release used region and clear these bits */
        case REG_OP_RELEASE:
            for (; i < entries; i++)
                bitmap[index + i] &= ~bitmask;
            break;

        default:
            break;
    }
done:
    return ret_val;
}


/* find_free_region
   description: find a contiguous aligned memory region and set bits in the bitmap.
   input: bitmap - pointer to the bitmap
          num_bits - number of bits to check
          order - order region size (log base 2 of number of bits requested). must align to power of 2s.
   output: none
   return value: bit index in the bitmap of the newly allocated region if success; -ENOMEM if out of memory
   side effect: none
*/
int find_free_region(uint32_t* bitmap, uint32_t num_bits, uint32_t order) {
    uint32_t pos = 0;
    for (; pos < num_bits; pos += (1 << order)) {
        if (_reg_op(bitmap, pos, order, REG_OP_ISFREE))
            break;
    }
    if (pos == num_bits) // if no such contiguous region can be found
        return -ENOMEM;
    _reg_op(bitmap, pos, order, REG_OP_ALLOC);
    return (int) pos;
}


/* release_region
   description: release a contiguous aligned memory region and clear bits in the bitmap.
   input: bitmap - pointer to the bitmap
          pos - starting index of bit region to release
          order - order region size (log base 2 of number of bits requested). must align to power of 2s.
   output: none
   return value: none
   side effect: none
*/
void release_region(uint32_t* bitmap, uint32_t pos, uint32_t order) {
    _reg_op(bitmap, pos, order, REG_OP_RELEASE);
}


/* allocate_region
   description: allocate a specified region of a bitmap and set bits in the bitmap.
   input: bitmap - pointer to the bitmap
          pos - starting index of bit region to allocate
          order - order region size (log base 2 of number of bits requested). must align to power of 2s.
   output: none
   return value: 0 if success; -EBUSY if specified region ocntains high bit(s)
   side effect: none
*/
int allocate_region(uint32_t* bitmap, uint32_t pos, uint32_t order) {
    if (!_reg_op(bitmap, pos, order, REG_OP_ISFREE))
        return -EBUSY;
    _reg_op(bitmap, pos, order, REG_OP_ALLOC);
    return 0;
}
