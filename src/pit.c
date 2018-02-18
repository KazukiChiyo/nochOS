/* pit.c - Driver for PIT.
   author: Kexuan Zou
   date: 11/13/2017
   external source: http://wiki.osdev.org/Programmable_Interval_Timer
*/

#include <lib.h>
#include <x86_desc.h>
#include <types.h>
#include <i8259.h>
#include <interrupt.h>
#include <idt.h>
#include <vfs.h>
#include <pit.h>
#include <time.h>
#include <sched.h>
#include <debug.h>
#include <proc.h>

volatile time_t system_time;

/* pit_read_freq
   description: reads current frequency from PIT
   input: none
   output: none
   return value: current frequency from PIT
   side effect: change PIT rate of interrupt
*/
uint16_t pit_read_freq(void) {
    unsigned long flags;
    cli_and_save(flags); // critical section begins
    uint16_t ret_val;
    outb(LATCH_READ_CMD, PIT_CMD_PORT); // set latch read command
    asm volatile (
        "inb %1, %%al\n"
        "movb %%al, %%ah\n"
        "inb %1, %%al\n"
        "rolw $8, %%ax\n" // rotate ax by 8 bits (switch al and ah)
        "movw %%ax, %0\n"
        : "=g" (ret_val) // output operands
        : "g" (CH0_DATA_PORT)// input operands
        : "%eax" // clobber list
    );
    sti();
    restore_flags(flags); // critical section ends
    return (uint16_t)(PIT_MAX_FREQ / ret_val);
}


/* pit_set_freq
   description: sets frequency on the PIT; in this configuration, PIT uses channel 0, low/high bytes, and rate generator
   input: freq - frequency to set
   output: none
   return value: none
   side effect: change PIT rate of interrupt
*/
void pit_set_freq(uint32_t freq) {
//    unsigned long flags;
//    cli_and_save(flags); // critical section begins

    /* normalize frequency */
    uint16_t divisor;
    if (freq <= PIT_MIN_FREQ) divisor = PIT_MIN_DIV;
    else if (freq >= PIT_MAX_FREQ) divisor = PIT_MAX_DIV;
    else divisor = (uint16_t)(PIT_MAX_FREQ / (uint16_t)freq);

    /* write command to prots */
    outb(CH0_RATE_CMD, PIT_CMD_PORT); // write command
    outb(divisor & 0xFF, CH0_DATA_PORT); // write low byte of PIT reload value
    outb(divisor >> 8, CH0_DATA_PORT); // write high byte of PIT reload value
//    sti();
//    restore_flags(flags); // critical section ends
}


/* pit_init
   description: write to terminal screen
   input: status - status of execute function
   output: none
   return value: status of execute funciton, pass by %eax
   side effect: none
*/
void pit_init(void) {
    int_set_idt(PIT_IRQ_PIN); // set idt entry for pit
    pit_set_freq(MS_FREQ); // set PIT ticking frequency
    enable_irq(PIT_IRQ_PIN); // enable irq for pit
}


/* pit_handler
   description: increment time counter
   input: none
   output: none
   return value: none
   side effect: none
*/
void pit_handler(void) {
    system_time.count_ms++;
#ifndef RUN_TESTS
    if (!(system_time.count_ms % TIME_QUANTUM) && cur_proc_pcb != NULL)
        do_sched();
#endif
}
