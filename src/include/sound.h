/*
 * sound.h
 *
 *  Created on: Nov 11, 2017
 *      Author: Jiacheng Zhu
 */

#include <types.h>

#ifndef SOUND_H_
#define SOUND_H_
#define HARDWARE_CLK 1193180
#define AUDIO_PORT 0x61
#define INITIAL_HIGH_ENABLE_AUDIO 3
#define DISABLE_SOUND 0xFC
#define PIT_BEEP_CMD 0xB6
void nosound();

void play_sound(uint32_t nFrequence);

extern void beep(int sound_type, int frequency) ;

#endif /* SOUND_H_ */
