/* sched.c - Scheduler for the OS. This scheduler uses round-robin algorithm and time quantum for each process is 15ms.
   author: Kexuan Zou
   date: 11/14/2017
*/

#include <sched.h>
#include <types.h>
#include <lib.h>
#include <paging.h>
#include <proc.h>
#include <vfs.h>
#include <mod_fs.h>
#include <ext2.h>
#include <terminal.h>
#include <x86_desc.h>
#include <pit.h>
#include <i8259.h>

pcb_t* cur_proc_pcb; // declared in proc.h
uint8_t proc_status[NUM_PROC]; // declared in proc.h
sess_t sess_desc[NUM_SESS]; // declared in terminal.h
uint32_t cur_sess_id; // declared in terminal.h


/* load_shell_img
   description: get entry point of a shell program, load it into its designated memory, map virtual address for the program image.
   input: cur_pid - current pid
   output: none
   return value: shell program image's entry point
   side effect: load program inage in its user space, map virtual memory
*/
uint32_t load_shell_img(uint32_t cur_pid) {
    uint32_t entry_point;
    file_t program;
    uint8_t cmd_buf[CMD_WORD_SIZE];

    program.f_op = ext2_file_fop;
    program.f_op->open(&program, "/bin/shell");
    program.f_pos = ENTRY_POINT_POS;
    program.f_op->read(&program, cmd_buf, CMD_WORD_SIZE);
    entry_point = (uint32_t)cmd_buf[0] | ((uint32_t)cmd_buf[1] << 8) | ((uint32_t)cmd_buf[2] << 16) | ((uint32_t)cmd_buf[3] << 24);

    /* initilize a 4mb page for user program; maps program physical address to virtual user space, copy data to the virtual memory space */
    uint32_t prog_phys_addr = get_phys_addr_by_pid(cur_pid);
    map_virtual_4mb(prog_phys_addr, USER_VIRT_TOP);

    program.f_pos = 0;
    program.f_op->read(&program, (void *)PROG_VIRT_START, program.f_dentry.d_inode.i_size);
    program.f_op->close(&program);

    return entry_point;
}


/* do_sched
   description: switch to the next process in the queue. in the first round terminal 2 and 3 are assigned their PCBs, but shell programs are neither loaded nor being run; in this case, do_sched executes shell.
   input: none
   output: none
   return value: none
   side effect: none
*/
void do_sched(void) {
    /* if all procsses are inactive, return */
    if (query_proc_status(INACTIVE) == NUM_PROC) return;
    pcb_t* this_pcb, * next_pcb;
    uint32_t this_pid, next_pid;
    uint32_t entry_point;
    uint32_t prog_phys_addr;

    /* save current pcb */
    this_pcb = cur_proc_pcb;
    this_pid = this_pcb->pid;

    if (proc_status[this_pid] == ACTIVE)
        proc_status[this_pid] = DAEMON; // set current process as daemon

    /* if shell program still awaits to load, switch to next pending process */
    if (query_proc_status(PENDING)) {
        next_pid = get_next_pid(this_pid, PENDING);

        /* if pending process exists but get_next_pid fails, then the process itself is marked pending (and in this case must be a existing shell) */
        next_pid = (next_pid == NUM_PROC) ? this_pid : next_pid;
        next_pcb = get_pcb_by_pid(next_pid);
        entry_point = load_shell_img(next_pcb->pid);

        /* if switching to an active session, then next process to schedule must switch back to active and write to video memory */
        if (next_pcb->active_sess == cur_sess_id)
            map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);

        /* if switching away from active session, then current process running must run as daemon and write to its assigned cached video memory */
        else
            map_virtual_4kb_prog(sess_desc[next_pcb->active_sess].cached_vidmem, VMEM_VIRT_START);

        /* modify tss. ss0 is kernel stack segment, esp0 points to the starting address of current process's kernel stack */
        tss.esp0 = get_esp0_by_pid(next_pcb->pid);

        proc_status[next_pid] = ACTIVE;
        cur_proc_pcb = next_pcb;

        /* set up file descriptor, enables stdin and stdout */
        fd_array_init();
        file_io_init("", "");

        /* set video memory */
        set_vidmem_param(VMEM_VIRT_START);
        cur_proc_pcb->uptime += TIME_QUANTUM;
        send_eoi(PIT_IRQ_PIN);

        /* jump to loaded shell program */
        asm volatile (
            "pushl %0\n" // push SS
            "pushl %1\n" // push ESP
            "pushfl\n" // push EFLAGS
            "orl $0x200,(%%esp)\n"
            "pushl %2\n" // push CS
            "pushl %3\n" // push return address
            "iret\n"
            : // output operands
            : "g"(USER_DS), "g"(USER_VIRT_BOT - 4), "g"(USER_CS), "g"(entry_point) // input operands
        );
    }

    /* if shell programs are all active, switch to next active process */
    else {
        next_pid = get_next_pid(this_pid, DAEMON);
        next_pcb = get_pcb_by_pid(next_pid);

        /* remap virtual address to the next prmgram's user space */
        prog_phys_addr = get_phys_addr_by_pid(next_pid);
        map_virtual_4mb(prog_phys_addr, USER_VIRT_TOP);

        /* if switching to an active session, then next process to schedule must switch back to active and write to video memory */
        if (next_pcb->active_sess == cur_sess_id)
            map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);

        /* if switching away from active session, then current process running must run as daemon and write to its assigned cached video memory */
        else
            map_virtual_4kb_prog(sess_desc[next_pcb->active_sess].cached_vidmem, VMEM_VIRT_START);

        /* modify tss. ss0 is kernel stack segment, esp0 points to the starting address of current process's kernel stack */
        tss.esp0 = get_esp0_by_pid(next_pcb->pid);

        cur_proc_pcb = next_pcb;

        /* set video memory */
        set_vidmem_param(VMEM_VIRT_START);
        cur_proc_pcb->uptime += TIME_QUANTUM;
        send_eoi(PIT_IRQ_PIN);

        /* save current ebp. kernel_esp, which points to the top of reg_struct, has already been saved by do_irq(). restore next process's ebp and esp */
        asm volatile (
            "movl %%ebp,%0\n"
            :"=g"(this_pcb->kernel_ebp)
        );
        asm volatile (
            "movl %0,%%esp\n"
            "movl %1,%%ebp\n"
            "jmp ret_from_int\n"
            :
            : "g" (cur_proc_pcb->kernel_esp), "g" (cur_proc_pcb->kernel_ebp) // input operands
        ); // control sequence ends here, no need for return value
    }
}
