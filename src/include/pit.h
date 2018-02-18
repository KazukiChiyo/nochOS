/* author: Kexuan Zou
   date: 11/13/2017
*/

#ifndef _PIT_H
#define _PIT_H
#include <types.h>

#define CH0_DATA_PORT 0x40 // channel 0 data port(r/w)
#define CH1_DATA_PORT 0x41 // channel 1 data port(r/w)
#define CH2_DATA_PORT 0x42 // channel 2 data port(r/w)
#define PIT_CMD_PORT 0x43 // mode/command register(r)
#define LATCH_READ_CMD 0x00 // read from latch to complete 2 consecutive reads
#define CH0_RATE_CMD 0x34 // channel 0, low/high bytes, rate generator
#define PIT_IRQ_PIN 0 // pit irq pin is 0
#define PIT_MAX_FREQ 1193181 // max frequency
#define PIT_MAX_DIV 1 // max divisor is 1
#define PIT_MIN_FREQ 18 // min frequency
#define PIT_MIN_DIV 65535 // min divisor is 65535
#define MS_FREQ 1000 // frequency for a per-milisecond ticking is 1000

uint16_t pit_read_freq(void);
void pit_set_freq(uint32_t freq);
extern void pit_init(void);
extern void pit_handler(void);

#endif
