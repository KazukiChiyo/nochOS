#ifndef RTC_H
#define RTC_H

#include <mod_fs.h>

#define RTC_IRQ_PIN 8 // pin for irq is IRQ8

#define PRIODIC_INT_ON			0x40
#define NMI_OFF 				0x80
#define PRIODIC_INT_OFF			0x0f
#define INDEX_PORT				0x70
#define VALUE_PORT				0x71
#define REG_A_NMI_OFF			0x8A
#define REG_B_NMI_OFF			0x8B
#define MASK_KEEP_SETTINGS		0xf0
#define MASK_KEEP_VALUE			0x0f
#define DISABLE_ALARM			0xdf
#define BIT_MASK_1				0x0001
#define SHIFT_MAX				15
#define SHIFT                   0xf //rate-1
#define MAX_REP					0x0400
#define failure					-1
#define RTC_IRQ_PIN				8

/// Handles an rtc interrupt.
extern void rtc_handler(void);

/// Initialize RTC interrupts.
extern void rtc_init(void);

extern file_op_t * rtc_fop;

#endif /* RTC */
