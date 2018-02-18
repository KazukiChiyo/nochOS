/* author: Kexuan Zou
   date: 11/27/2017
   external source: http://wiki.osdev.org/Random_Number_Generator
*/

#include <rand.h>

#if defined(LEGACY_MODE)
uint32_t rseed = 0; // random seed
#endif

/**
 * rand - generate a 32-bit random number
 * @return - random number generated
 */
uint32_t rand(void) {
    return __rand();
}


/**
 * rand_range - generate a 32-bit random number within the range
 * @param lo - lower limit of the range
 * @param hi - upper limit of the range
 * @return - random number within range
 */
uint32_t rand_range(uint32_t lo, uint32_t hi) {
    return __RAND_RANGE(__rand(), lo, hi);
}
