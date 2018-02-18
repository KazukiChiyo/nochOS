/* author: Kexuan Zou
   date: 12/5/2017
*/

#ifndef _PANIC_H
#define _PANIC_H

#include <regs.h>
#include <types.h>
#include <system.h>

#define PANIC_STACK_LINE 6 // stack entries per line is 6
#define PANIC_STACK 12 // shows 12 stack entries in a panic message
#define PANIC_EIP 8 // shows 4 instructions in a panic nessage
#define PANIC_WAIT 5000 // panic screen waits for 5000 ms

#define P_ST(p)                            \
(                                          \
    ((p) == NULL) ?                        \
        0UL : (uint32_t)*((uint32_t* )(p)) \
)

extern void do_panic(struct regs* regs);
void info_printk(struct regs* regs);

#endif
