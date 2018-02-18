/* author: Kexuan Zou
   date: 12/5/2017
*/

#include <lib.h>
#include <types.h>
#include <panic.h>
#include <terminal.h>
#include <proc.h>
#include <time.h>
#include <system.h>
#include <i8259.h>

char* fault_msg  = "Int %d: Received %s.\n";
char* hw_msg1    = ">>> Hardware info:\n";
char* hw_msg2    = "CPU ID: %d    uptime: %d ms\n";
char* info_msg1  = ">>> All general-purpose registers:\n";
char* info_msg2  = "eax: 0x%x    ebx: 0x%x    ecx: 0x%x    edx: 0x%x\n";
char* info_msg3  = "esi: 0x%x    edi: 0x%x    ebp: 0x%x    esp: 0x%x\n";
char* info_msg4  = "cr0: 0x%x    cr2: 0x%x    cr3: 0x%x    cr4: 0x%x\n";
char* info_msg5  = "eip: 0x%x    cs: 0x%x    eflags: 0x%x    ss: 0x%x\n";
char* proc_msg1  = ">>> Process swapper: %s (pid: %d, threadinfo: N/A)\n";
char* proc_msg2  = "Program break: 0x%x    Stack section: 0x%x\n";
char* stack_msg1 = ">>> Stack:";
char* stack_msg2 = "0x%x  ";
char* call_msg1  = ">>> Call trace:\n";
char* call_msg2  = "  [<%x>]  %x\n";
char* fatal_msg  = "Fatal error, killing current process\n";

/* fault messages packed in an array */
const int8_t* msg[20] = {"Divide Error", "Debug Exception", "Nonmaskable Interrupt", "Breakpoint Exception", "Overflow Exception", "BOUND Range Exceeded Exception", "Invalid Opcode Exception", "Device Not Available Exception", "Double Fault Exception", "Coprocessor Segment Overrun Exception", "Invalid TSS Exception", "Segment Not Present Exception", "Stack Fault Exception", "General Protection Exception", "Page Fault Exception", "Reserved Exception", "x87 FPU Floating-Point Error", "Alignment-Check Exception", "Machine-Check Exception", "SIMD Floating-Point Exception"};


/**
 * do_panic - print panic messages
 * @param regs - info registers to print
 */
void do_panic(struct regs* regs) {
    IO_OFF;
    set_vidmem_param(VIDEO);
    screen_x = screen_y = 0;
    clear_screen(cur_sess_id);
    printf(fault_msg, regs->orig_eax, msg[regs->orig_eax]);
    info_printk(regs);
    sti();
    sleep(PANIC_WAIT);
    set_vidmem_param(VIDEO);
    clear_screen(cur_sess_id);
    IO_ON;
}


/**
 * info_printk - print info onto screen
 * @param regs [description]
 */
void info_printk(struct regs* regs) {
    uint32_t cpuid, cr0, cr2, cr3, cr4;
    int i = 0;
    asm volatile (
        "cpuid\n"
        "movl %%ecx,%0\n"
        "movl %%cr0,%%eax\n"
        "movl %%eax,%1\n"
        "movl %%cr2,%%eax\n"
        "movl %%eax,%2\n"
        "movl %%cr3,%%eax\n"
        "movl %%eax,%3\n"
        "movl %%cr4,%%eax\n"
        "movl %%eax,%1\n"
        : "=g" (cpuid), "=g" (cr0), "=g" (cr2), "=g" (cr3), "=g" (cr4) // output operands
        : // input operands
        : "%eax", "%ecx" // clobber list
    );
    printf(hw_msg1);
    printf(hw_msg2, cpuid, system_time.count_ms);
    printf(info_msg1);
    printf(info_msg2, regs->eax, regs->ebx, regs->ecx, regs->edx);
    printf(info_msg3, regs->esi, regs->edi, regs->ebp, regs->esp);
    printf(info_msg4, cr0, cr2, cr3, cr4);
    printf(info_msg5, regs->eip, regs->xcs, regs->eflags, regs->xss);
    printf(proc_msg1, cur_proc_pcb->command, cur_proc_pcb->pid);
    printf(proc_msg2, cur_proc_pcb->prog_break, USER_STACK_TOP);
    printf(stack_msg1);
    for (; i < PANIC_STACK; i++) {
        if (!(i % PANIC_STACK_LINE)) printf("\n");
        printf(stack_msg2, P_ST(regs->esp + sizeof(uint32_t)*i));
    }
    printf("\n");
    printf(call_msg1);
    for (i = 0; i < PANIC_EIP; i++)
        printf(call_msg2, regs->eip - sizeof(uint32_t)*i, P_ST(regs->eip - sizeof(uint32_t)*i));
    printf(fatal_msg);
}
