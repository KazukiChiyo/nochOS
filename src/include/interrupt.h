#ifndef _INTERRUPT_H
#define _INTERRUPT_H


#include <idt.h>
#include <types.h>
#include <regs.h>

/// This table stores pointers to interrupt handlers.
extern uint32_t int_table [INT_TABLE_SIZE];

/// Handles an interrupt. Uses fastcall calling convention.
extern __attribute__((fastcall)) void do_irq(struct regs *regs);

extern void* ret_from_int;
extern void* ret_from_sig;

#endif /* _INTERRUPT_H */
