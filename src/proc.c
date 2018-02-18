/* proc.c - Implementations of process-specific functions.
   author: Kexuan Zou
   date: 10/28/2017
*/

#include <mod_fs.h>
#include <ext2.h>
#include <types.h>
#include <proc.h>
#include <paging.h>
#include <terminal.h>
#include <lib.h>
#include <terminal.h>
#include <regs.h>
#include <x86_desc.h>
#include <system.h>
#include <mem.h>

pcb_t* cur_proc_pcb = NULL; // declared in proc.h
uint8_t proc_status[NUM_PROC]; // declared in proc.h

/* get_pcb_by_pid
   description: return pcb pointer of a process specified by its pid.
   input: pid - pid of the process
   output: none
   return value: target pcb pointer
   side effect: none
*/
pcb_t* get_pcb_by_pid(uint32_t pid) {
    return (pcb_t* ) (KERNEL_BOT - KSTACK_SIZE * (pid + 1));
}


/* get_esp0_by_pid
   description: return starting esp (esp0) for a given pid; used by tss
   input: pid - pid of the process
   output: none
   return value: esp0 of that process
   side effect: none
*/
uint32_t get_esp0_by_pid(uint32_t pid) {
    return (uint32_t) (KERNEL_BOT - KSTACK_SIZE * pid - 4);
}


/* get_phys_addr_by_pid
   description: get physical address of a process by its pid; processes are loaded in memory by after kernel space (4mb - 8mb) by pid
   input: pid - pid of the porcess
   output: none
   return value: starting physical address
   side effect: none
*/
uint32_t get_phys_addr_by_pid(uint32_t pid) {
    return KERNEL_BOT + pid * PROG_PAGE_SIZE;
}


/* get_cur_pcb
   description: get current pcb based on its %esp, since each process has a ordered stack layout in kernel stack.
   input: none
   output: none
   return value: pointer to current pcb corresponds to %esp
   side effect: none
*/
pcb_t * get_cur_pcb(void) {
    pcb_t * pcb;

    /* AND pcb with mask to obtain current pcb */
    asm volatile (
        "movl %%esp,%0\n" // the current stack pointer
        "andl %1,%0\n"    // use stack pointer to find PCB
        : "=g" (pcb) // output operands
        : "g" (PCB_MASK) // input operands
        : "cc" // clobber list
    );
    return pcb;
}


/* get_parent_pcb
   description: get parent pcb pointer specified by an input pcb pointer
   input: cur_pcb - current pcb pointer
   output: none
   return value: its parent pcb pointer
   side effect: none
*/
pcb_t* get_parent_pcb(pcb_t* cur_pcb) {
    if (cur_pcb == NULL) return NULL;
    uint32_t parent_pid = cur_pcb->parent_pid;
    return get_pcb_by_pid(parent_pid);
}


/* parse_cmd
   description: parse the command and output it
   input: src - command passed into the function
          cmd - command to copy to
   output: none
   return value: -1 if fail to parse, otherwise index of the end of command field
   side effect: none
*/
int32_t parse_cmd(const int8_t* src, int8_t* cmd) {
    if (src == NULL) return -1;
    uint32_t i; // loop iterator

    for (i = 0; ; i++) {
        if (src[i] == ' ' || src[i] == '\0' || src[i] == '\n') { // if a terminator is reached
            cmd[i] = '\0';
            return i;
        }
        cmd[i] = src[i];
    }
    return -1;
}



/**
 * clear_args - clears command and argument buffer within a pcb struct
 * @param cur_pcb - current pcb pointer
 */
void clear_args(pcb_t* cur_pcb) {
    int i = 0;
    for (; i < ARG_WORD_SIZE; i++) {
        cur_pcb->command[i] = 0x00;
        cur_pcb->arg[i] = 0x00;
    }
}


/* parse_arg
   description: parse the command and store its argument into corresponding pcb's arg field
   input: src - command passed into the function
          cur_pcb - pointer to the current pcb
          start_idx - starting index of argument field
   output: none
   return value: none
   side effect: modify arg field of pcb
*/
void parse_arg(const int8_t* src, pcb_t* cur_pcb, int8_t * fout, uint32_t start_idx) {
    uint32_t i; // loop iterator
    if (strlen(src) <= start_idx)
        return;

    if (src[start_idx] == '>' && src[start_idx + 1] == ' ')
        --start_idx;

    for (i = 0; ; i++, start_idx++) {
        if (src[start_idx] == '\0' || src[start_idx] == '\n') { // if a terminator is reached
            cur_pcb->arg[i] = '\0';
            return;
        }
        if (src[start_idx] == ' ') {
            ++start_idx;
            cur_pcb->arg[i] = '\0';
            break;
        }
        cur_pcb->arg[i] = src[start_idx];
    }

    if (src[start_idx] == '>' && src[start_idx + 1] == ' ') {
        start_idx += 2;
        for (i = 0; ; ++i, ++start_idx) {
            if (src[start_idx] == '\0' || src[start_idx] == '\n' || src[start_idx] == ' ') {
                fout[i] = '\0';
                return;
            }
            fout[i] = src[start_idx];
        }
    }
}


///
/// Initializes the file descriptor array by setting flags of all file
/// objects to 0.
///
void fd_array_init(void) {
    cur_proc_pcb->fd_bitmap = 0x0;
}


/* query_proc_status
   description: query how many process have the specified status
   input: status - runtime status of the process
   output: none
   return value: number of processes with the given status
   side effect: none
*/
uint32_t query_proc_status(uint8_t status) {
    uint32_t i, retval;
    for (i = 0, retval = 0; i < NUM_PROC; i++) {
        if (proc_status[i] == status) retval++;
    }
    return retval;
}


/* get_next_pid
   description: get next process's pid given current pid and status of target process
   input: cur_pid - current pid
          status - runtime status of the process
   output: none
   return value: next available process's pid
   side effect: none
*/
uint32_t get_next_pid(uint32_t cur_pid, uint8_t status) {
    uint32_t i = cur_pid + 1; // loop iterator
    for (; i < NUM_PROC; i++) {
        if (proc_status[i] == status) return i;
    }
    for (i = 0; i < cur_pid; i++) {
        if (proc_status[i] == status) return i;
    }
    return NUM_PROC;
}


/* child_pcb_init
   description: initialize a child pcb given its parent pcb pointer.
   NOTE: this function only sets pid, parent_pid, session id, and clears arg
   input: parent_pcb - parent pcb pointer
   output: none
   return value: child pcb pointer; if fail, return NULL
   side effect: none
*/
pcb_t* child_pcb_init(pcb_t* parent_pcb) {
    uint32_t child_pid; // child pid
    pcb_t* child_pcb;
    int i; // loop iterator

    /* if the process does not have a parent it is a startup process */
    if (parent_pcb == NULL) {
        child_pid = 0;
        goto cont;
    }

    /* if the process has a parent and valid pid is assigned*/
    for (i = 0; i < NUM_PROC; i++) {
        if (proc_status[i] == INACTIVE) { // if vacant spot is found
            child_pid = (uint32_t) i;
            goto cont;
        }
    }
    return NULL; // if no valid pid can be obtained return -1

cont:
    child_pcb = get_pcb_by_pid(child_pid); // get child pcb

    /* set up child pcb struct */
    proc_status[child_pid] = ACTIVE; // set child process to active
    child_pcb->pid = child_pid;
    if (parent_pcb != NULL) {
        child_pcb->parent_pid = parent_pcb->pid;
        child_pcb->active_sess = parent_pcb->active_sess;
    }
    clear_args(child_pcb);
    child_pcb->uptime = 0;
    child_pcb->prog_break = 0UL;
    return child_pcb;
}

///
/// Initializes the input and output files for current process.
/// By default, the files are stdin and stdout respectively.
///
/// - author: Zhengcheng Huang
/// - arguments
///     fin:
///         Filename of the input file.
///         If an empty string is given, use stdin as the input file.
///     fout:
///         Filename of the output file.
///         If an empty string is given, use stdout as the output file.
///
void file_io_init(int8_t * fin, int8_t * fout) {
    file_t * filep;
    dentry_t * base, * dent;
    base = (dentry_t *)malloc(sizeof(dentry_t));
    dent = (dentry_t *)malloc(sizeof(dentry_t));

    // Initialize input file.
    if (fin[0] == '\0')
        stdin_init();
    else {
        filep = cur_proc_pcb->fd_array;
        filep->f_op = ext2_file_fop;
        if (-1 == filep->f_op->open(filep, fin)) {
            stdin_init();
        }
    }

    // Initialize output file.
    if (fout[0] == '\0')
        stdout_init();
    else {
        filep = cur_proc_pcb->fd_array + 1;
        filep->f_op = ext2_file_fop;
        if (-1 == filep->f_op->open(filep, fout)) {
            // Create the file.
            parse_path(fout, base, dent);
            base->d_inode.i_op->create(&base->d_inode, dent, 0x81FF);
            free_dentry(dent);
            filep->f_op->open(filep, fout);
        }
    }

    cur_proc_pcb->fd_bitmap = 0x3;
}


/**
 * kill_pid - kill process with a given pid and an exit code; this function returns to its parent by changing current pcb to its parent
 * @param pid - pid of desired process to kill
 * @param exit_code - exit code of that program
 */
void kill_pid(uint32_t pid, int32_t exit_code) {
    struct regs * parent_regs;
    pcb_t* cur_pcb = get_pcb_by_pid(pid);

    /* if try to halt root process */
    if (cur_pcb->parent_pid == NUM_PROC) {
        cur_pcb->uptime = 0; // reset the timer
        proc_status[pid] = PENDING; // mark current process as pending
        sti(); // enable interrupts
        while(1); // spin until time quantum passes
    }
    pcb_t* child_pcb = cur_pcb; // set child pcb

    cur_pcb = get_parent_pcb(child_pcb); // set current pcb to be parent of child pcb
    proc_status[child_pcb->pid] = INACTIVE; // child process is finished
    proc_status[cur_pcb->pid] = ACTIVE; // resume execution of parent process

    /* relink sigaction linkage pcb field */
    link_sa_pcb(cur_pcb, cur_sess_id);

    /* rempas program virtual page (128mb - 132mb) to current process */
    uint32_t prog_phys_addr;
    if (cur_pcb != NULL) {
        prog_phys_addr = get_phys_addr_by_pid(cur_pcb->pid);
        map_virtual_4mb(prog_phys_addr, USER_VIRT_TOP);
    }

    /* if switching to current session from daemon session, then current process running must switch back to active and write to video memory */
    if (cur_pcb->active_sess == cur_sess_id)
        map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);

    /* if switching away from current session, then current process running must run as daemon and write to its assigned cached video memory */
    else
        map_virtual_4kb_prog(sess_desc[cur_pcb->active_sess].cached_vidmem, VMEM_VIRT_START);

    /* set tss esp0 to be parent esp */
    tss.esp0 = get_esp0_by_pid(child_pcb->parent_pid);

    /* set video memory */
    set_vidmem_param(VMEM_VIRT_START);

    /* set current pcb */
    cur_proc_pcb = cur_pcb;

    /* return to parent. restores parent esp and ebp, return exit_code */
    parent_regs = (struct regs *)child_pcb->parent_esp;
    parent_regs->eax = exit_code;
    asm volatile (
        "movl %0,%%esp\n"
        "jmp ret_from_sig\n"
        : // output operands
        : "g" (child_pcb->parent_esp)
    ); // control sequence ends here, no need for return value
}
