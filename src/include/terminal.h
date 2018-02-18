#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <types.h>
#include <proc.h>

#define KEY_BUF_SIZE 128 // keyboard buffer size is 128s
#define VIDMEM_SIZE 0x1000 // video memory size is 4kb
#define NUM_SESS 3 // total number of session is 3
#define VIDEO 0xB8000
#define BOUNDARY1 0x02400000
#define BOUNDARY2 0x02500000
#define BOUNDARY3 0x02600000
#define SHELL_CMD_LEN 5 // "shell" command length is 5
#define USR_NAME_LEN 16

/* struct for a new terminal session */
typedef struct sess_t {
    uint32_t vid_x; // x position of video memory
    uint32_t vid_y; // y position of video memory
    uint32_t cached_vidmem; // cached video memory starting address
    volatile int cmd_available; // determine whether 'ENTER' is pressed and so a new command is available
    uint32_t kbd_buf_idx; // keyboard buffer index
    uint8_t kbd_buf[KEY_BUF_SIZE]; // keyboard buffer
    uint8_t cmd_buf[KEY_BUF_SIZE]; // session-specific command buffer
    pcb_t* sa_pcb; // sigaction linkage pcb
} __attribute__((packed)) sess_t;

extern uint8_t* usr_name[USR_NAME_LEN]; // user name
extern sess_t sess_desc[NUM_SESS]; // session descriptor array
extern uint32_t cur_sess_id; // session id for the current session

uint32_t get_cached_vidmem(uint32_t sess_id);
int link_sa_pcb(pcb_t* target_pcb, uint32_t sess_num);
void startup_envir_init(void);
/* terminal_init
   description: initialize terminal; set dependencies for all 3 terminals, and start shell in the first terminal
   input: none
   output: none
   return value: none
   side effect: changes video memory
   author: Kexuan Zou
*/
extern void terminal_init(void);

/* sess_switch
   description: switch from current session (specified in the cur_sess_id field), to the given session id
   input: sess_id - session id to switch to
   output: none
   return value: 0 if success, -1 if fail
   side effect: none
   author: Kexuan Zou
*/
extern int sess_switch(uint32_t sess_id);
extern void scroll_up(void);
extern void scroll_down(void);
void stdin_init(void);

///
/// Creates an stdout file and stores in file array.
///
/// - author: Zhengcheng Huang
/// - side effects: Fills the second entry of file array with stdout.
///
void stdout_init(void);

/* sess_putc
   description: session-specific putc
   input: c - char to put to screen
          sess_id - session id to print to
   output: none
   return value: none
   author: Kexuan Zou
*/
void sess_putc(uint8_t c, uint32_t sess_id);

/* sess_puts
   description: session-specific puts
   input: s - starting address of the string
          sess_id - session id to print to
   output: none
   return value: number of chars written to the screen
   author: Kexuan Zou
*/
int32_t sess_puts(int8_t *s, uint32_t sess_id);

/* get_vidmem_param
   description: get video memory parameter for sess_printf, sess_putc and sess_puts
   input: none
   output: video parameter
   return value: none
   side effect: none
*/
uint32_t get_vidmem_param(void);

/* set_vidmem_param
   description: set video memory parameters for sess_printf, sess_putc and sess_puts
   input: mem_start - starting address of video memory
   output: none
   return value: none
   side effect: none
*/
void set_vidmem_param(uint32_t mem_start);

/* update_cursor
 * Inputs: sess_id - session id
 * Return Value: void
 * Description: update cursor with the corresponding x and y position
 */
void update_cursor(uint32_t sess_id);

/* newline
   description: move to newline
   input: sess_id - session id
   output: none
   return value: none
   side effect: change screen_x, screen_y
   author: Kexuan Zou
*/
void newline(uint32_t sess_id);

/* line_check
   description: check if need to move to next line
   input: sess_id
   output: none
   return value: if currently at line end, return nonzero; else return 0
   side effect: change screen_x, screen_y
   author: Kexuan Zou
*/
int line_check(uint32_t sess_id);

/* backspace
   description: delete a char at (screen_x, screen_y) by adding a ' '
   input: sess_id - session id
   output: none
   return value: if retrace to the previous line, return 1; else return 0
   side effect: erase a single char in video memory
   author: Kexuan Zou
*/
void backspace(uint32_t sess_id);

/* clear_screen
   description: clear the screen (trigerred by ctrl+l)
   input: none
   output: none
   return value: none
   side effect: erase the entire video memory
   author: Kexuan Zou
*/
void clear_screen(uint32_t sess_id);
void write_history(uint32_t y, uint32_t sess_id);
void update_historyptr(uint32_t sess_id);
void load_screen(uint32_t sess_id);

#endif /* _TERMINAL_H */
