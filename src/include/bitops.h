/* author: Kexuan Zou
   date: 11/21/2017
   external source: http://elixir.free-electrons.com/linux/v3.0/source/arch/x86/include/asm/bitops.h
*/

#ifndef _BITOPS_H
#define _BITOPS_H
#include <types.h>

#define BITOP_ADDR(x) "=m" (*(volatile uint32_t* )(x))

/* ffs
   description: find first set bit in word
   input: word - word to test
   output: none
   return value: 0 if word is 0; index (1 based) of first set bit otherwise
   side effect: none
*/
static inline int ffs(uint32_t word) {
    int ret_val;
    asm volatile (
        "bsfl %1, %0\n"
        "jnz 1f\n"
        "movl $-1,%0\n"
        "1:\n"
        : "=r" (ret_val) // output operands
        : "rm" (word) // input operands
    );
    return ret_val + 1;
}


/* fls
   description: find last set bit in word
   input: word - word to test
   output: none
   return value: 0 if word is 0; index (1 based) of last set bit otherwise
   side effect: none
*/
static inline int fls(uint32_t word) {
    int ret_val;
    asm volatile (
        "bsrl %1, %0\n"
        "jnz 1f\n"
        "movl $-1,%0\n"
        "1:\n"
        : "=r" (ret_val) // output operands
        : "rm" (word) // input operands
    );
    return ret_val + 1;
}


/* test_and_set_bit
   description: find first set bit in word
   input: addr - address of the word to set bit
          bit - bit index (0 based) to set
   output: none
   return value: the bit's old value
   side effect: none
*/
static inline int test_and_set_bit(volatile uint32_t* addr, uint32_t bit) {
    int old_bit;
    asm volatile (
        "bts %2,%1\n"
        "sbb %0,%0\n"
        : "=r" (old_bit), BITOP_ADDR(addr)
        : "Ir" (bit)
    );
    return old_bit;
}


/* test_and_clear_bit
   description: find first clear bit in word
   input: addr - address of the word to clear bit
          bit - bit index (0 based) to clear
   output: none
   return value: the bit's old value
   side effect: none
*/
static inline int test_and_clear_bit(volatile uint32_t* addr, uint32_t bit) {
    int old_bit;
    asm volatile (
        "btr %2,%1\n"
        "sbb %0,%0\n"
        : "=r" (old_bit), BITOP_ADDR(addr)
        : "Ir" (bit)
    );
    return old_bit;
}

#endif
