#ifndef _PROC_H
#define _PROC_H

#include <types.h>
#include <vfs.h>
#include <list.h>
#include <system.h>

#define SIG_COUNT 6 // only 5 signals are supported
#define MAX_OPEN_FILES 8
#define KSTACK_SIZE 0x2000 // kernel stack for each program is 8kb
#define NUM_PROC 6 // support 6 processes
#define PCB_MASK 0xFFFFE000 // filters out lower 13 bits
#define KERNEL_BOT (KERNEL_TOP + KERNEL_SIZE) // end of kernel page
#define CMD_WORD_SIZE 4 // command word size for both file specifier and entry point is 4
#define ENTRY_POINT_POS 24
#define ARG_WORD_SIZE 128 // keyboard buffer has size 128

#define INACTIVE 0 // the session is not initialized
#define ACTIVE 1 // the session is currently active
#define DAEMON 2 // the session is switched to background
#define PENDING 3 // the session is initialized as a placeholder
#define IDLE 4 // the session is not active

typedef struct sa_hand {
    uint32_t sa_handler;
    uint32_t sa_mask;
} __attribute__((packed)) sa_hand;

/* struct for process control block */
typedef struct pcb_t {
    uint32_t pid; // pid for current process
    uint32_t parent_pid; // pid for its parent process
    file_t fd_array[MAX_OPEN_FILES]; // process-specific file descriptor
    uint32_t fd_bitmap; // Indicates which files are available (0 for available).
    uint32_t parent_esp; // esp for parent process
    uint32_t kernel_esp; // kernel esp loadpoint for current process
    uint32_t kernel_ebp; // kernel ebp loadpoint for current process
    int8_t command[ARG_WORD_SIZE]; // command field relative to this process
    int8_t arg[ARG_WORD_SIZE]; // argment field relative to this process
    uint32_t uptime; // uptime of this process
    uint32_t active_sess; // active session id;
    uint32_t prog_break; // program break
    struct sa_hand sighand[SIG_COUNT]; // signal handler descriptor
    struct list_head sigpending; // a list of pending signals
} __attribute__((packed)) pcb_t;

/* NOTE: the memory occupied by a union will be large enough to hold the largest member of the union, so struct has size 0x2000 aka. 8kb */
typedef union proc_stack {
    pcb_t pcb; // pcb struct stays at the top
    uint32_t stack[KSTACK_SIZE]; // kernel stack space builds bottom-up
} proc_stack;

extern pcb_t* cur_proc_pcb; // pointer to pcb of the current process
extern uint8_t proc_status[NUM_PROC]; // status for each process

/* functions private to this file */

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
void file_io_init(int8_t * fin, int8_t * fout);

/* get_pcb_by_pid
   description: return pcb pointer of a process specified by its pid.
   input: pid - pid of the process
   output: none
   return value: target pcb pointer
   side effect: none
*/
pcb_t* get_pcb_by_pid(uint32_t pid);

/* get_esp0_by_pid
   description: return starting esp (esp0) for a given pid; used by tss
   input: pid - pid of the process
   output: none
   return value: esp0 of that process
   side effect: none
*/
uint32_t get_esp0_by_pid(uint32_t pid);

/* get_cur_pcb
   description: get current pcb based on its %esp, since each process has a ordered stack layout in kernel stack.
   input: none
   output: none
   return value: pointer to current pcb corresponds to %esp
   side effect: none
*/
pcb_t* get_cur_pcb(void);

/* get_parent_pcb
   description: get parent pcb pointer specified by an input pcb pointer
   input: cur_pcb - current pcb pointer
   output: none
   return value: its parent pcb pointer
   side effect: none
*/
pcb_t* get_parent_pcb(pcb_t* cur_pcb);

/* get_phys_addr_by_pid
   description: get physical address of a process by its pid; processes are loaded in memory by after kernel space (4mb - 8mb) by pid
   input: pid - pid of the porcess
   output: none
   return value: starting physical address
   side effect: none
*/
uint32_t get_phys_addr_by_pid(uint32_t pid);

/* parse_cmd
   description: parse the command and output it
   input: src - command passed into the function
          cmd - command to copy to
   output: none
   return value: -1 if fail to parse, otherwise index of the end of command field
   side effect: none
*/
int32_t parse_cmd(const int8_t* src, int8_t* cmd);

/**
 * clear_args - clears argument buffer within a pcb struct
 * @param cur_pcb - current pcb pointer
 */
void clear_args(pcb_t* cur_pcb);

/* parse_arg
   description: parse the command and store its argument into corresponding pcb's arg field
   input: src - command passed into the function
          cur_pcb - pointer to the current pcb
          start_idx - starting index of argument field
   output: none
   return value: none
   side effect: modify arg field of pcb
*/
void parse_arg(const int8_t* src, pcb_t* cur_pcb, int8_t * fout, uint32_t start_idx);

///
/// Initializes the file descriptor array by setting flags of all file
/// objects to 0.
///
void fd_array_init(void);

/* query_proc_status
   description: query how many process have the specified status
   input: status - runtime status of the process
   output: none
   return value: number of processes with the given status
   side effect: none
*/
uint32_t query_proc_status(uint8_t status);

/* get_next_pid
   description: get next process's pid given current pid and status of target process
   input: cur_pid - current pid
          status - runtime status of the process
   output: none
   return value: next available process's pid
   side effect: none
*/
uint32_t get_next_pid(uint32_t cur_pid, uint8_t status);

/* child_pcb_init
   description: initialize a child pcb given its parent pcb pointer. 
   NOTE: this function only sets pid, parent_pid, session id, and clears arg
   input: parent_pcb - parent pcb pointer
   output: none
   return value: child pcb pointer; if fail, return NULL
   side effect: none
*/
pcb_t* child_pcb_init(pcb_t* parent_pcb);
void kill_pid(uint32_t pid, int32_t exit_code);

/**
 * fd_avail - check if a fd number if available
 * @param  fd - fd number
 * @return - 1 if available, 0 if not available
 */
static inline uint8_t fd_avail(uint32_t fd) {
    return !(cur_proc_pcb->fd_bitmap & (1 << fd));
}

#endif
