#ifndef IDT_H
#define IDT_H

extern void idt_init(void);
extern void exp_set_idt(void);
extern void int_set_idt(int irq);
extern void syscall_set_idt(void);

#define INT_TABLE_SIZE 16
#define EXP_TABLE_SIZE 20

#endif /* idt.h */
