/* author: Kexuan Zou
   date: 11/27/2017
   external source: http://courses.engr.illinois.edu/ece385/
   https://en.wikipedia.org/wiki/Finite_field_arithmetic
   https://github.com/kokke/tiny-AES-c/blob/master/aes.c
   https://github.com/openluopworld/aes_128
   http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf
*/

#ifndef _AES_H
#define _AES_H

#include <types.h>
#include <lib.h>
#include <system.h>

#define AES_BLOCK_BITS 128 // block size is 128 bits
#define AES_BLOCK_SIZE 16
#define AES_KEY_BITS AES_BLOCK_BITS // key size for AES 128 is 128 bits
#define AES_NUM_ROUND 10 // number of rounds for AES 128
#define AES_COLS_STATE 4 // number of columns in a state is 4
#define AES_ROW_STATE (AES_BLOCK_SIZE / AES_COLS_STATE)
#define AES_KEY_ENTRIES 4 // number of entries in a key
#define AES_KEY_SIZE AES_BLOCK_SIZE
#define AES_BYTE_MASK 0xFF
#define AES_SCHED_SIZE 176 // expansion key size is 176
#define AES_EXPD_ROUND (AES_SCHED_SIZE / AES_KEY_ENTRIES) // expansion key rounds are 44

/* states for AES encryption/decryption */
#define AES_IDLE 0 // no encryption or decryption is currently in place
#define AES_EXPD 1 // currently doing key expansion
#define AES_ENCRY 2 // currently doing encryption
#define AES_DECRY 3 // currently doing decryption

void sub_word(uint8_t* word);
void sub_bytes(uint8_t* state);
void inv_sub_bytes(uint8_t* state);
void aes_get_random_key(uint8_t* key);
void key_expansion(uint8_t* key);
void aes_block_encryption(uint8_t* in, uint8_t* out);
void aes_block_decryption(uint8_t* in, uint8_t* out);

extern uint8_t round_key[AES_SCHED_SIZE];


/**
 * GF_HELPER - helper funciton to obtain the finite field GF(2^8) of x.
 * @param x - number to convert
 */
#define GF_HELPER(num) \
    (((num) & 0x80) ? (((num) << 1) ^ 0x1B) : ((num) << 1))


#if !defined(AES_FAST_CALC)
/**
 * XTIME - helper function for power of x multiplication. higher powers of x can be implementated by repeated application of this funciton.
 * @param num - number to be transformed
 */
#define XTIME(num) \
    (((num) << 1) ^ ((((num) >> 7) & 1) * 0x1B))

/**
 * gf_mul - multiple two numbers in the field of GF(2^8). NOTE: differ from the look up table approach used in ECE 385, we trade time with space and calculate value each time.
 * @param x, y - two bytes to multiply
 * @return - its GF(2^8) multiplicative value
 */
static inline uint8_t gf_mul(uint8_t x, uint8_t y) {
    return (((y & 1) * x) ^
     ((y >> 1 & 1) * XTIME(x)) ^
     ((y >> 2 & 1) * XTIME(XTIME(x))) ^
     ((y >> 3 & 1) * XTIME(XTIME(XTIME(x)))) ^
     ((y >> 4 & 1) * XTIME(XTIME(XTIME(XTIME(x))))));
}


/**
 * mix_columns - combine four bytes of each column of the states using invertible linear transformation, defined as follows:
 *  2  3  1  1
 *  1  2  3  1
 *  1  1  2  3
 *  3  1  1  2
 * @param state - state of the block in current round
 */
static inline void mix_columns(uint8_t* state) {
    uint8_t xtime;
    uint8_t temp[AES_BLOCK_SIZE]; // temporary array to hold the state
    int i;
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        temp[i] = state[i];
    for (i = 0; i < AES_COLS_STATE; i++) {
        xtime = temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3];
        state[AES_ROW_STATE*i] = XTIME(temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1]) ^ temp[AES_ROW_STATE*i] ^ xtime;
        state[AES_ROW_STATE*i+1] = XTIME(temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2]) ^ temp[AES_ROW_STATE*i+1] ^ xtime;
        state[AES_ROW_STATE*i+2] = XTIME(temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3]) ^ temp[AES_ROW_STATE*i+2] ^ xtime;
        state[AES_ROW_STATE*i+3] = XTIME(temp[AES_ROW_STATE*i+3] ^ temp[AES_ROW_STATE*i]) ^ temp[AES_ROW_STATE*i+3] ^ xtime;
    }
}


/**
* mix_columns - inversely transform the state. the invese matrix is defined as follows.
*  e  b  d  9
*  9  e  b  d
*  d  9  e  b
*  b  d  9  e
* @param state - state of the block in current round
 */
static inline void inv_mix_columns(uint8_t* state) {
    uint8_t temp[AES_BLOCK_SIZE]; // temporary array to hold the state
    int i;
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        temp[i] = state[i];
    for (i = 0; i < AES_COLS_STATE; i++) {
        state[AES_ROW_STATE*i] = gf_mul(temp[AES_ROW_STATE*i], 0x0E) ^ gf_mul(temp[AES_ROW_STATE*i+1], 0x0B) ^ gf_mul(temp[AES_ROW_STATE*i+2], 0x0D) ^ gf_mul(temp[AES_ROW_STATE*i+3], 0x09);
        state[AES_ROW_STATE*i+1] = gf_mul(temp[AES_ROW_STATE*i], 0x09) ^ gf_mul(temp[AES_ROW_STATE*i+1], 0x0E) ^ gf_mul(temp[AES_ROW_STATE*i+2], 0x0B) ^ gf_mul(temp[AES_ROW_STATE*i+3], 0x0D);
        state[AES_ROW_STATE*i+2] = gf_mul(temp[AES_ROW_STATE*i], 0x0D) ^ gf_mul(temp[AES_ROW_STATE*i+1], 0x09) ^ gf_mul(temp[AES_ROW_STATE*i+2], 0x0E) ^ gf_mul(temp[AES_ROW_STATE*i+3], 0x0B);
        state[AES_ROW_STATE*i+3] = gf_mul(temp[AES_ROW_STATE*i], 0x0B) ^ gf_mul(temp[AES_ROW_STATE*i+1], 0x0D) ^ gf_mul(temp[AES_ROW_STATE*i+2], 0x09) ^ gf_mul(temp[AES_ROW_STATE*i+3], 0x0E);
    }
}

#else


/**
 * mix_columns - combine four bytes of each column of the states using gf helper
 * @param state - state of the block in current round
 */
static inline void mix_columns(uint8_t* state) {
    uint8_t xtime;
    uint8_t temp[AES_BLOCK_SIZE]; // temporary array to hold the state
    int i;
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        temp[i] = state[i];
    for (i = 0; i < AES_COLS_STATE; i++) {
        xtime = temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3];
        state[AES_ROW_STATE*i] = GF_HELPER(temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1]) ^ temp[AES_ROW_STATE*i] ^ xtime;
        state[AES_ROW_STATE*i+1] = GF_HELPER(temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2]) ^ temp[AES_ROW_STATE*i+1] ^ xtime;
        state[AES_ROW_STATE*i+2] = GF_HELPER(temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3]) ^ temp[AES_ROW_STATE*i+2] ^ xtime;
        state[AES_ROW_STATE*i+3] = GF_HELPER(temp[AES_ROW_STATE*i+3] ^ temp[AES_ROW_STATE*i]) ^ temp[AES_ROW_STATE*i+3] ^ xtime;
    }
}


/**
* inv_mix_columns - inversely transform the state using gf helper.
* @param state - state of the block in current round
 */
static inline void inv_mix_columns(uint8_t* state) {
    uint8_t xtime, factor1, factor2;
    uint8_t temp[AES_BLOCK_SIZE]; // temporary array to hold the state
    int i;
    for (i = 0; i < AES_BLOCK_SIZE; i++)
        temp[i] = state[i];
    for (i = 0; i < AES_COLS_STATE; i++) {
        xtime = temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3];
        state[AES_ROW_STATE*i] = GF_HELPER(temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+1]) ^ temp[AES_ROW_STATE*i] ^ xtime;
        state[AES_ROW_STATE*i+1] = GF_HELPER(temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+2]) ^ temp[AES_ROW_STATE*i+1] ^ xtime;
        state[AES_ROW_STATE*i+2] = GF_HELPER(temp[AES_ROW_STATE*i+2] ^ temp[AES_ROW_STATE*i+3]) ^ temp[AES_ROW_STATE*i+2] ^ xtime;
        state[AES_ROW_STATE*i+3] = GF_HELPER(temp[AES_ROW_STATE*i+3] ^ temp[AES_ROW_STATE*i]) ^ temp[AES_ROW_STATE*i+3] ^ xtime;
        factor1 = GF_HELPER(GF_HELPER(temp[AES_ROW_STATE*i] ^ temp[AES_ROW_STATE*i+2]));
        factor2 = GF_HELPER(GF_HELPER(temp[AES_ROW_STATE*i+1] ^ temp[AES_ROW_STATE*i+3]));
        xtime = GF_HELPER(factor1 ^ factor2);
        state[AES_ROW_STATE*i] ^= xtime ^ factor1;
        state[AES_ROW_STATE*i+1] ^= xtime ^ factor2;
        state[AES_ROW_STATE*i+2] ^= xtime ^ factor1;
        state[AES_ROW_STATE*i+3] ^= xtime ^ factor2;
    }
}

#endif


/**
 * ROT_WORD - rotate a word. used to expand the key into round keys.
 * @param word - a word represented by 4 bytes to rotate
 */
static inline void rot_word(uint8_t* word) {
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}


/**
 * shift_rows - shift rows of the state matrix to left, each with a different offset scheme stated as follows:
 * 0: [s0]  [s4]  [s8]  [s12] << 0
 * 1: [s1]  [s5]  [s9]  [s13] << 1
 * 2: [s2]  [s6]  [s10] [s14] << 2
 * 3: [s3]  [s7]  [s11] [s15] << 3
 * @param state - state of the block in current round
 */
static inline void shift_rows(uint8_t* state) {
	uint8_t temp = state[1];
	state[1] = state[5];
	state[5] = state[9];
	state[9] = state[13];
	state[13] = temp;
	temp = state[2];
	state[2] = state[10];
    state[10] = temp;
    temp = state[6];
	state[6] = state[14];
	state[14] = temp;
	temp = state[3];
	state[3] = state[15];
    state[15] = state[11];
    state[11] = state[7];
	state[7] = temp;
}


/**
 * inv_shift_rows - shift rows of the state matrix to right, each with a different offset scheme stated as follows:
 * 0: [s0]  [s4]  [s8]  [s12] >> 0
 * 1: [s1]  [s5]  [s9]  [s13] >> 1
 * 2: [s2]  [s6]  [s10] [s14] >> 2
 * 3: [s3]  [s7]  [s11] [s15] >> 3
 * @param state - state of the block in current round
 */
static inline void inv_shift_rows(uint8_t* state) {
	uint8_t temp = state[13];
	state[13] = state[9];
	state[9] = state[5];
	state[5] = state[1];
	state[1] = temp;
	temp = state[14];
	state[14] = state[6];
    state[6] = temp;
    temp = state[10];
	state[10] = state[2];
	state[2] = temp;
	temp = state[7];
	state[7] = state[11];
    state[11] = state[15];
    state[15] = state[3];
	state[3] = temp;
}


/**
 * add_round_key - adds round key to each state be an xor operation
 * @param state - state of the block in current round
 * @param round_key - round key to add to each state
 */
static inline void add_round_key(uint8_t* state, uint8_t* key) {
    int i = 0;
    for (; i < AES_COLS_STATE; i++) {
        state[i] = state[i] ^ key[i];
        state[i+4] = state[i+4] ^ key[i+4];
        state[i+8] = state[i+8] ^ key[i+8];
        state[i+12] = state[i+12] ^ key[i+12];
    }
}

#endif
