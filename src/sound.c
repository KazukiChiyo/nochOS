/*
 * sound.c
 *
 *  Created on: Nov 11, 2017
 *      Author: jzhu62
 */
#include <lib.h>
#include <types.h>
#include <sound.h>
#include <rtc.h>
#include <mod_fs.h>
#include <pit.h>

//for make the sound to run, add -soundhw all to work
  void play_sound(uint32_t nFrequence) {
    pit_set_freq(nFrequence); // set pit to interupt at the desired frequency

    //connect channel 2 with PIT
 	uint32_t freq_temp = inb(AUDIO_PORT);


  	if (freq_temp != (freq_temp | INITIAL_HIGH_ENABLE_AUDIO)) {
 		outb( freq_temp | INITIAL_HIGH_ENABLE_AUDIO,AUDIO_PORT);
 	}
 }

//disable
  void nosound() {
 	uint8_t temp = inb(AUDIO_PORT) & DISABLE_SOUND;

 	outb(temp, AUDIO_PORT);
 }

//Make a beep based on 256 bit sound
 void beep(int sound_type, int frequency) {
 	 file_op_t * a= rtc_fop;
 	 a->write(0,&frequency,4);
 	 a->read(0,&frequency,4);
 	 play_sound(sound_type);
 	 a->read(0,&frequency,4);
 	 nosound();
 }
