/* interrupt.c - Initialize exceptions (intel pre-defined first 20 interrupts)
   author: Kexuan Zou, Zhengcheng Huang
   date: 10/19/2017 */

#include <x86_desc.h>
#include <lib.h>
#include <types.h>
#include <terminal.h>
#include <regs.h>
#include <signal.h>
#include <exception.h>
#include <proc.h>

pcb_t * cur_proc_pcb; // declared in proc.h

/// This jump table stores pointers to exception handlers.
uint32_t exp_table [EXP_TABLE_SIZE];

__attribute__((fastcall))
void do_exp(struct regs *regs/*, int8_t * msg*/) {
    // exp_trap_setup();
    // printf(fault_msg, regs->orig_eax, msg);
    // info_printk();

    /* request a pending signal based on its exception number */
    if (regs->orig_eax == DIV_ZERO)
        request_signal(DIV_ZERO, cur_proc_pcb);
    else
        request_signal(SEGFAULT, cur_proc_pcb);
    return;
}
