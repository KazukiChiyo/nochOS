/* keyboard.c - the C part to initialize keyboard
 */

#include <rtc.h>
#include <lib.h>
#include <x86_desc.h>
#include <types.h>
#include <i8259.h>
#include <interrupt.h>
#include <idt.h>
#include <proc.h>
#include <vfs.h>

volatile int wait;

/// File operations for RTC.
static int32_t rtc_fopen(file_t * self, const int8_t * filename);
static int32_t rtc_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t rtc_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t rtc_fclose(file_t * self);

/// File operation jumptable for RTC.
static file_op_t rtc_fops = {
    .open = rtc_fopen,
    .read = rtc_fread,
    .write = rtc_fwrite,
    .close = rtc_fclose
};

file_op_t * rtc_fop = &rtc_fops;

/* reference: wiki.osdev.org/RTC */

///
/// Handles an RTC interrupt.
///
/// - author: Zhengcheng Huang
/// - side effects: Writes garbage to video memory (for now).
///
void rtc_handler(void) {
   //test_interrupts();
   wait = 1;
   outb(0x0C, INDEX_PORT);  // select register C
   inb(VALUE_PORT);

}

///
/// Initializes RTC and enables RTC interrupts.
///
/// - author: Jiacheng Zhu, Zixu Zhao, Zhengcheng Huang
/// - side_effect: Set the RTC to periodically tick.
///                Don't USE NMI.
///
void rtc_init(void){
    char prev;
    // Set corresponding IDT entry
    int_set_idt(RTC_IRQ_PIN);

    // disable NMI, and select register A(register is random does not matter)
    outb(REG_B_NMI_OFF,INDEX_PORT);
    // get the value of register B
    prev = inb(VALUE_PORT);
    //reset index to B
    outb(REG_B_NMI_OFF,INDEX_PORT);
    // write the previous value into B. Turn on periodic interrupt,
    // turn off alarm.
    outb((prev | PRIODIC_INT_ON) & DISABLE_ALARM, VALUE_PORT);

    outb(REG_A_NMI_OFF, INDEX_PORT);
    prev = inb(VALUE_PORT);
    outb(REG_A_NMI_OFF, INDEX_PORT);
    outb((prev & MASK_KEEP_SETTINGS) | SHIFT, VALUE_PORT);

    enable_irq(RTC_IRQ_PIN);
    wait = 0;
}

///
/// Determine the rate value to write to RTC from the given frequency.
///
/// - author: Jiacheng Zhu, Zhengcheng Huang
///
/// - arguments
///     frequency: The frequency of RTC in Hz.
///
/// - return: The number of bits by which right shifting the base value (0x8000)
///           yields the frequency in Hz.
///           The RTC hardware takes this number to decide its frequency.
///
int32_t rtc_rate(uint16_t frequency){
    int j;

    // Check if frequency is valid.
    if (frequency > MAX_REP){
        return -1;
    }

    // since the defaut is 0x8000, there is only 15 posiible shifts match.
    for (j = SHIFT_MAX; j > 0; j--){
        if(((frequency >> j) & BIT_MASK_1) == 1){
            // RTC can only generate power of 2 Hz.
            if ((frequency >> j) << j != frequency)
                return -1;

            return SHIFT_MAX - j + 1;
        }
    }
    return -1;
}

///
/// File open operation for RTC.
///
/// - side effects: Modifies the file descriptor array.
///
int32_t rtc_fopen(file_t * self, const int8_t * filename) {
    // Write value to RTC. Keep previous settings.
    uint8_t prev;
    outb(REG_A_NMI_OFF, INDEX_PORT);
    prev = inb(VALUE_PORT);
    outb(REG_A_NMI_OFF, INDEX_PORT);
    outb((prev & MASK_KEEP_SETTINGS) | (0x0f & 13), VALUE_PORT);

    self->f_dentry.d_inode.i_ino = 0;
    self->f_pos = 0;

    return 0;
}
 
///
/// File read operation for RTC.
///
/// This function returns only on the next RTC interrupt.
///
int32_t rtc_fread(file_t * self, void * buf, uint32_t nbytes) {
    sti();

    // Starts waiting.
    wait = 0;
    // Wait.
    while (!wait);

    return 0;
}

/*
 * File write operation for RTC.
 *
 * - side effects: Modifies the frequency of RTC
 */
int32_t rtc_fwrite(file_t * self, const void * buf, uint32_t nbytes){
    // The frequency of RTC in Hz to be set.
    uint16_t frequency = *(uint16_t *) buf;
    // Input to RTC representing the frequency.
    int32_t rate;
    // Keep previous settings of RTC.
    unsigned char prev;

    if (nbytes == 4) {
        // Calculate the rate value to write to RTC.
        rate = rtc_rate(frequency);
        // Check failure.
        if(rate == -1) return -1;
        else {
            // Write value to RTC. Keep previous settings.
            outb(REG_A_NMI_OFF, INDEX_PORT);
            prev = inb(VALUE_PORT);
            outb(REG_A_NMI_OFF, INDEX_PORT);
            outb((prev & MASK_KEEP_SETTINGS) | (0x0f & rate), VALUE_PORT);
            return nbytes;
        }
    }
    else return -1;
}

///
/// Auto success.
///
int32_t rtc_fclose(file_t * self) {
    return 0;
}

