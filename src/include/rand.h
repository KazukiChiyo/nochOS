/* author: Kexuan Zou
   date: 11/27/2017
   external source: http://wiki.osdev.org/Random_Number_Generator
   https://rosettacode.org/wiki/Linear_congruential_generator#C
*/

#ifndef _RAND_H
#define _RAND_H

#include <types.h>
#include <system.h>
#include <time.h>

#define RAND_INVALID ~0UL
#define RAND_MAX ((1U << 31) - 1)
#define RAND_PRIME_1 214013
#define RAND_PRIME_2 2531011
#define RAND_SHIFT 16
#define __RAND_RANGE(num, lo, hi) \
    ((num % (hi - lo + 1)) + lo)

uint32_t rand(void);
uint32_t rand_range(uint32_t lo, uint32_t hi);

#if !defined(LEGACY_MODE)
/**
 * __rand - generate a 32-bit random number. NOTE: intel added support for instruction 'RDRAND' starting from core pentium, so older architectures do not support this command. the real-randomized number is hardware-generated.
 * @return - RAND_INVALID if command is not available, or the random number
 */
static inline uint32_t __rand(void) {
    uint32_t ret_val = RAND_INVALID;
    asm volatile (
        "movl $1,%%eax\n"
        "movl $0,%%ecx\n"
        "cpuid\n"
        "shrl $30,%%ecx\n"
        "cmpl $1,%%ecx\n" // if instruction is not available yet
        "jne 1f\n"
        "rdrand %%eax\n"
        "1:\n"
        : "=r" (ret_val) // output operands
    );
    return ret_val;
}

#else


/**
 * __rand - alternative method to generate 32-bit random number as a fallback option for older architectures. this is a linear congruential generator to generate a sequence of pseudo-randomized numbers
 * @return - random number
 */
extern uint32_t rseed;
static inline uint32_t __rand(void) {
    if (!rseed) rseed = system_time.count_ms;
    return (rseed = (rseed*RAND_PRIME_1 + RAND_PRIME_2) & RAND_MAX);
}
#endif

#endif
