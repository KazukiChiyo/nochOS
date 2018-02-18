/* idt.c - Initializes IDT table entry. Intel-defined interrupts(0x00 - 0x19), RTC(0x28) and keyboard(0x21) are supported. TODO: Add system call(0x80) for future cps.
   author: Kexuan Zou
   date: 10/23/2017 */

#include <idt.h>
#include <lib.h>
#include <x86_desc.h>
#include <types.h>
#include <interrupt.h>
#include <exception.h>
#include <syscall.h>

#define SYSCALL_ENTRY 0x80
#define IRQ_BASE 0x20

/* idt_init
   description: initialize for all 256 idt entries
   input: none
   output: none
   return value: none
   side effect: modifies the IDT
*/
void idt_init(void) {
    /* initialize all 256 idt entries */
    idt_desc_t the_idt_desc;
    int i;
    for (i = 0; i < NUM_VEC; i++) {
        the_idt_desc.seg_selector = KERNEL_CS;
        the_idt_desc.reserved4 = 0x0;
        the_idt_desc.reserved3 = 0x0;
        the_idt_desc.reserved2 = 0x1;
        the_idt_desc.reserved1 = 0x1;
        the_idt_desc.size = 0x1;
        the_idt_desc.reserved0 = 0x0;
        the_idt_desc.dpl = 0x0;
        the_idt_desc.present = 0x0; // idt entry is not present yet
        idt[i] = the_idt_desc;
    }
}


/* exp_set_idt
   description: sets idt table entry for intel-defined exceptions, which is entry 0x00 - 0x19
   input: none
   output: none
   return value: none
   side effect: modifies the IDT
*/
void exp_set_idt(void) {
    int i;
    for (i = 0; i < EXP_TABLE_SIZE; i++) {
        idt[i].present = 0x1;
        SET_IDT_ENTRY(idt[i], exp_table[i]);
    }
}

///
/// Sets up IDT entry for system calls.
///
/// - side effects: Modifies the IDT.
///
void syscall_set_idt(void) {
    idt[SYSCALL_ENTRY].present = 0x1;
    idt[SYSCALL_ENTRY].dpl = 0x3;
    SET_IDT_ENTRY(idt[SYSCALL_ENTRY], syscall_handler);
}

/* int_set_idt
   description: sets idt table entry for given irq pin
   input: irq - IRQ pin to set IDT
   output: none
   return value: none
   side effect: modifies the IDT
*/
void int_set_idt(int irq) {
    idt[IRQ_BASE + irq].present = 0x1;
    SET_IDT_ENTRY(idt[IRQ_BASE + irq], int_table[irq]);
}
