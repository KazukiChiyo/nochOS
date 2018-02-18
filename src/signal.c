/* author: Kexuan Zou
   date: 12/3/2017
*/

#include <list.h>
#include <mem.h>
#include <signal.h>
#include <proc.h>
#include <x86_desc.h>
#include <lib.h>
#include <regs.h>
#include <panic.h>
#include <debug.h>

uint32_t sigmask_info[SIG_COUNT];

/**
 * save_sigmask - save status of signal masks
 */
void save_sigmask(void) {
    int i = 0;
    for (; i < SIG_COUNT; i++)
        sigmask_info[i] = GET_SA_MASK(i, cur_proc_pcb);
}


/**
 * restore_saved_sigmask - restore saved status of signal masks
 */
void restore_saved_sigmask(void) {
    int i = 0;
    for (; i < SIG_COUNT; i++)
        GET_SA_MASK(i, cur_proc_pcb) = sigmask_info[i];
}


/**
 * set_other_sigmask - set all other sigmasks except for one
 * @param signum - signum exempt from being set
 */
void set_other_sigmask(int32_t signum) {
    int i = 0;
    for (; i < SIG_COUNT; i++) {
        if (i != signum)
            GET_SA_MASK(i, cur_proc_pcb) = SIG_MASK_SET;
    }
}


/**
 * request_signal - install a new pending signal in the pcb's sigpending struct
 * @param num - signum to insert
 * @param sa_pcb - pcb pointer where sigaction appends to
 */
void request_signal(int32_t signum, pcb_t* sa_pcb) {
    sigaction* new_sig = malloc(sizeof(sigaction));
    new_sig->sig = signum;
    enqueue_signal(new_sig, &(sa_pcb->sigpending));
}


/**
 * get_signal - get the next pending signal in the queue
 * @param ksig - ksignal struct obtained from the queue, this requires dequeue from sigqueue and get sighandler from sighand array
 * @return - 0 if fail; 1 if success
 */
int get_signal(ksignal* ksig) {
    sigaction* action = dequeue_signal(&(cur_proc_pcb->sigpending));
    if (action == NULL)
        return 0;
    ksig->sig = action->sig;
    ksig->sa_handler = GET_SA_HANDLER(action->sig, cur_proc_pcb);
    ksig->sa_mask = GET_SA_MASK(action->sig, cur_proc_pcb);
    free(action); // free the sigaction struct installed by request_signal
    return 1;
}


/**
 * do_signal_dfl - kill the current process. ERR_INT is passed as a return value of sys_halt to indicate that a program halts by exception
 * @param rval - return error value
 * @param regs - register struct passed in
 */
void do_signal_dfl(int32_t rval, struct regs* regs) {
    restore_saved_sigmask();

#if !defined(NO_PANIC_MSG)
    if (rval == ERR_RVAL) {
        do_panic(regs);
    }
#endif

    kill_pid(cur_proc_pcb->pid, rval);
}


/**
 * handle_signal - handle a user defined signal
 * @param ksig - contains info about the signal
 * @param reg - register struct
 */
void handle_signal(ksignal* ksig, struct regs* regs) {
    cli(); // disable interrupts
    uint32_t ret_addr, user_esp;
    restore_saved_sigmask();
    user_esp = regs->esp; // user esp is just the esp common_interrupt pushes in

    /* inject code to call sys_sigreturn
     * add esp, 4
     * mov eax, 10
     * int 0x80 */
    uint8_t payload[SIG_INJECT_SIZE] = {0x83, 0xC4, 0x04, 0xB8,
                                        0x0A, 0x00, 0x00, 0x00,
                                        0xCD, 0x80, 0x00, 0x00};

    /* prepare stack for user space, the user stack looks as follows:
     * return address (points to payload)
     * signum (handler parameter)
     * all regs
     * hardware context
     * payload */
    PUSH_USR(user_esp, payload, SIG_INJECT_SIZE);
    ret_addr = user_esp; // return address is the start of the code injected
    PUSH_USR(user_esp, regs, sizeof(struct regs));
    PUSH_USR(user_esp, &(ksig->sig), sizeof(uint32_t));
    PUSH_USR(user_esp, &(ret_addr), sizeof(uint32_t));

    /* setup new hardware context for iret */
    regs->eip = ksig->sa_handler; // entry point
    regs->xcs = USER_CS; // user code segment
    regs->eflags |= EFLAGS_UNMASK; // set eflags to unmask
    regs->esp = user_esp; // user esp
    regs->xss = USER_DS; // user data segment

    /* proceed to restore all previously saved registers, and iret */
    asm volatile (
        "movl %0,%%esp\n"
        "jmp ret_from_sig\n"
        :
        : "g" (regs)
    ); // control sequence ends here
}


/**
 * do_signal - execute a signal from the pending list
 * @param regs - register struct
 */
__attribute__((fastcall))
void do_signal(struct regs* regs) {
    if (regs->xcs != USER_CS) // if interrupt comes from signal, return
        return;
    ksignal ksig;
    save_sigmask(); // save sigmasks for all signals
    if (get_signal(&ksig) && ksig.sa_mask == SIG_MASK_CLEAR) { // do first pending, unmasked signal
        set_other_sigmask(ksig.sig);
        if (ksig.sa_handler == SIG_DFL) {
            switch(ksig.sig) {
                case DIV_ZERO:
                case SEGFAULT: do_signal_dfl(ERR_RVAL, regs); break;
                case INTERRUPT: do_signal_dfl(SIG_INT, regs); break;
                case ALARM:
                case USER1: break;
                case STACKFAULT: do_signal_dfl(ERR_STACK, regs); break;
                default: break;
            }
        }
        else // handle user signal handler
            handle_signal(&ksig, regs);
    }
    restore_saved_sigmask();
}


/**
 * proc_signal_init - initilize sigaction struct within a pcb
 * @param cur_pcb - pcb to setup signal
 */
void proc_signal_init(pcb_t* cur_pcb) {
    int i = 0;
    init_list_head(&(cur_pcb->sigpending));
    for (; i < SIG_COUNT; i++) {
        GET_SA_HANDLER(i, cur_pcb) = SIG_DFL; // default handler
        GET_SA_MASK(i, cur_pcb) = SIG_MASK_CLEAR; // sigmask is cleared
    }
}
