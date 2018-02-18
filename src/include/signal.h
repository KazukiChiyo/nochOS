/* author: Kexuan Zou
   date: 12/3/2017
*/

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <types.h>
#include <list.h>
#include <proc.h>
#include <lib.h>
#include <regs.h>

#define DIV_ZERO 0
#define SEGFAULT 1
#define INTERRUPT 2
#define ALARM 3
#define USER1 4
#define SIGKILL INTERRUPT
#define STACKFAULT 5

#define INT_RVAL 3 // program halts by an exception
#define ERR_STACK 4 // program halts because a stackoverflow occurs
#define SIG_INT 5 // program halts by a interrupt signal
#define ERR_RVAL 256 // program halts because an error occurs

#define SIG_MASK_CLEAR 0UL // signal mask clear
#define SIG_MASK_SET ~0UL // signal mask set

#define SIG_DFL 0x00100100 // default handler

#define SIG_INJECT_SIZE 12

extern uint32_t sigmask_info[SIG_COUNT]; // signal mask info

typedef struct sigaction {
    int32_t sig; // signal number
    struct list_head sig_list; // list_head struct to traverse the node
} __attribute__((packed)) sigaction;

typedef struct ksignal {
    uint32_t sig; // signal number
    uint32_t sa_handler; // signal handler
    uint32_t sa_mask; // mask
} __attribute__((packed)) ksignal;

// get signal handler from a specified pcb
#define GET_SIGHAND(sig, cur_pcb) \
    (cur_pcb->sighand[(sig)])

// get sa_handler from a specified pcb
#define GET_SA_HANDLER(sig, cur_pcb) \
    (cur_pcb->sighand[(sig)].sa_handler)

// get sa_mask from a specified pcb
#define GET_SA_MASK(sig, cur_pcb) \
    (cur_pcb->sighand[(sig)].sa_mask)

/**
 * PUSH_USR - push contents onto user stack specified by esp
 * @param esp - user stack esp
 * @param src - src to push
 * @param size - size to push
 */
#define PUSH_USR(esp, src, size)                                 \
    (esp) -= (size);                                             \
    memcpy((void* )(esp), (const void* )(src), (uint32_t)(size));

void save_sigmask(void);
void restore_saved_sigmask(void);
void set_other_sigmask(int32_t signum);
extern void request_signal(int32_t signum, pcb_t* sa_pcb);
int get_signal(ksignal* ksig);
extern void do_signal_dfl(int32_t rval, struct regs* regs);
void handle_signal(ksignal* ksig, struct regs* regs);
extern __attribute__((fastcall)) void do_signal(struct regs *regs);
extern void proc_signal_init(pcb_t* cur_pcb);

/**
 * sig_enqueue - enqueue a pending signal handler
 * @param node - sigaction node to insert
 * @param head - head of the sigpending list
 * @return - 0 if success; -1 if fail
 */
static inline int enqueue_signal(sigaction* node, list_head* head) {
    if (head == NULL || node == NULL) return -1;
    list_insert_before(&(node->sig_list), head); // insert new node at tail
    return 0;
}


/**
 * sig_dequeue - dequeue a pending signal handler
 * @param head - head of the sigpending list
 * @return - NULL if list is empty; else, sigaction pointer popped from the head
 */
static inline sigaction* dequeue_signal(list_head* head) {
    if (list_is_empty(head)) return NULL;
    list_head* list_first = head->next; // remove node at head
    list_delete(list_first); // delete the node
    return (sigaction* )LIST_ENTRY(list_first, sigaction, sig_list);
}

#endif
