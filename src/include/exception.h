#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <types.h>
#include <idt.h>
#include <regs.h>

typedef void exp_handler(void);

/// This jump table stores pointers to exception handlers.
extern uint32_t exp_table [EXP_TABLE_SIZE];

__attribute__((fastcall))
extern void do_exp(struct regs * regs/*, int8_t * msg*/);

/* function private to ths function */
// void info_printk(void);
// void exp_trap_setup(void);

#endif /* _EXCEPTION_H */
