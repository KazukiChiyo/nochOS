#include <terminal.h>
#include <mod_fs.h>
#include <types.h>
#include <lib.h>
#include <proc.h>
#include <paging.h>
#include <mouse.h>
#include <signal.h>
#include <color.h>

uint8_t ATTRIB = SET_COLOR(LIGHTGRAY, BLACK);

pcb_t * cur_proc_pcb; // declared in proc.h
uint8_t* usr_name[USR_NAME_LEN]; // declared in terminal.h
uint8_t proc_status[NUM_PROC]; // declared in proc.h
int32_t history_ptr[NUM_SESS];
int32_t screen_head[NUM_SESS];
int32_t boundary[NUM_SESS];

static uint8_t* sess_mem;

static uint8_t check_scroll(uint32_t sess_id);

/* stdin file operation jump table functions. */
static int32_t stdin_fopen(file_t * self, const int8_t * filename);
static int32_t terminal_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t stdin_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t stdin_fclose(file_t * self);

/* set up jump table for stdin */
static file_op_t stdin_fops = {
    .open = stdin_fopen,
    .read = terminal_fread,
    .write = stdin_fwrite,
    .close = stdin_fclose
};

file_op_t * stdin_fop = &stdin_fops;

/* stdout file operation jump table functions. */
static int32_t stdout_fopen(file_t * self, const int8_t * filename);
static int32_t stdout_fread(file_t * self, void * buf, uint32_t nbytes);
static int32_t terminal_fwrite(file_t * self, const void * buf, uint32_t nbytes);
static int32_t stdout_fclose(file_t * self);

/* set up jump table for stdout */
static file_op_t stdout_fops = {
    .open = stdout_fopen,
    .read = stdout_fread,
    .write = terminal_fwrite,
    .close = stdout_fclose
};

file_op_t * stdout_fop = &stdout_fops;

/* uint8_t disable_cursor();
 * Inputs: void
 * Return Value: void
 * Function: disable cursor on the screen */
void disable_cursor() {
  outb(0x0A,0x3D4);
  outb( 0x20,0x3D5);
}

/* uint8_t enable_cursor();
 * Inputs: cursor_start - starting position of cursor
           cursor_end - ending position of cursor
 * Return Value: void
 * Function: enable cursor on the screen */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
  outb(0x0A,0x3D4);
  outb( (inb(0x3D5) & 0xC0) | cursor_start,0x3D5);

  outb( 0x0B,0x3D4);
  outb((inb(0x3E0) & 0xE0) | cursor_end,0x3D5);
}



/* get_cached_vidmem
   description: get starting address of cached video memory by session id
   input: sess_id - session id
   output: none
   return value: starting address of video memory
   side effect: none
   author: Kexuan Zou
*/
uint32_t get_cached_vidmem(uint32_t sess_id) {
    return VIDEO + VIDMEM_SIZE + VIDMEM_SIZE * sess_id;
}


/**
 * link_sa_pcb - add signaction linker to specified process
 * @param target_pcb - pcb to link
 * @param sess_num - current session number to link to
 * @return - -1 if invalid; 0 if success
 * @author - Kexuan Zou
 */
int link_sa_pcb(pcb_t* target_pcb, uint32_t sess_num) {
    if (target_pcb == NULL || sess_num >= NUM_SESS)
        return -1; // invalid input
    sess_desc[sess_num].sa_pcb = target_pcb;
    return 0;
}


/* startup_envir_init
   description: initialize 3 terminal sessions as placeholders; actual PCB setup is done through execute system call
   input: none
   output: none
   return value: none
   side effect: none
   author: Kexuan Zou
*/
void startup_envir_init(void) {
    int cur_pid; // loop iterator
    pcb_t* cur_pcb;

    //initialize the history pointer, screen head and boundary
    history_ptr[0] = BOUNDARY1;
    history_ptr[1] = BOUNDARY2;
    history_ptr[2] = BOUNDARY3;

    screen_head[0] = BOUNDARY1;
    screen_head[1] = BOUNDARY2;
    screen_head[2] = BOUNDARY3;

    boundary[0] = BOUNDARY1;
    boundary[1] = BOUNDARY2;
    boundary[2] = BOUNDARY3;

    /* initiate environments for three sessions */
    for (cur_pid = 0; cur_pid < NUM_SESS; cur_pid++) {
        cur_pcb = get_pcb_by_pid(cur_pid);
        cur_pcb->pid = cur_pid;
        cur_pcb->parent_pid = NUM_PROC; // mark parent pid to be invalid
        cur_pcb->active_sess = cur_pid; // set active session
        strncpy(cur_pcb->command, "shell", SHELL_CMD_LEN); // shell command length is 5
        proc_status[cur_pid] = PENDING; // set process to be pending
        proc_signal_init(cur_pcb); // set signal handler
    }
}


/* terminal_init
   description: initialize terminal; set dependencies for all 3 terminals, and start shell in the first terminal
   input: none
   output: none
   return value: none
   side effect: changes video memory
   author: Kexuan Zou
*/
void terminal_init(void) {
    int i, j; // loop iterator

    //disable and reenable the cursor to make it look different :)
    disable_cursor();
    enable_cursor(0, NUM_ROWS-1);

    /* initialize session descriptor array */
    usr_name[0] = 0x00;
    for (i = 0; i < NUM_SESS; i++) {
        sess_desc[i].vid_x = sess_desc[i].vid_y = 0;
        sess_desc[i].cached_vidmem = get_cached_vidmem(i);
        map_virtual_4kb_first(sess_desc[i].cached_vidmem, sess_desc[i].cached_vidmem);
        sess_desc[i].cmd_available = 0;
        sess_desc[i].kbd_buf_idx = 0;
        for (j = 0; j < KEY_BUF_SIZE; j++) // clear keyboard buffer
            sess_desc[i].kbd_buf[i] = 0x00;
        for (j = 0; j < KEY_BUF_SIZE; j++) // clear command buffer
            sess_desc[i].cmd_buf[i] = 0x00;
    }

    /* clear content in video memory */
    set_vidmem_param(VIDEO);
    early_clear_screen();

    /* set placeholders for 3 processes */
    startup_envir_init();

    /* startup terminal 0 for current session */
    cur_sess_id = 0;
    proc_status[cur_sess_id] = ACTIVE;
}


/* sess_switch
   description: switch from current session (specified in the cur_sess_id field), to the given session id
   input: sess_id - session id to switch to
   output: none
   return value: 0 if success, -1 if fail
   side effect: none
   author: Kexuan Zou
*/
int sess_switch(uint32_t sess_id) {
    if (sess_id >= NUM_SESS) return -1; // if sess_id is invalid
    if (sess_id == cur_sess_id) return 0; // shourtcut from switching to the same session

    draw_char(mouse_i.old_char, mouse_i.x, mouse_i.y);

    disable_cursor();

    /* remap video memory's virtual address to its physical address */
    map_virtual_4kb_first(VIDEO, VIDEO);

    /* copy video memory content to its cached memory */
    memcpy((uint8_t* )sess_desc[cur_sess_id].cached_vidmem, (uint8_t* )VIDEO, NUM_COLS * NUM_ROWS * 2); // NOTE: attribute byte must also be copied

    /* switch to next session, copy cached memory to video memory */
    memcpy((uint8_t* )VIDEO, (uint8_t* )sess_desc[sess_id].cached_vidmem, NUM_COLS * NUM_ROWS * 2); // NOTE: attribute byte must also be copied

    /* update session id */
    cur_sess_id = sess_id;

    /* if switching to an active session, then current process running must switch back to active and write to video memory */
    if (cur_proc_pcb->active_sess == cur_sess_id)
        map_virtual_4kb_prog(VIDEO, VMEM_VIRT_START);

    /* if switching away from an active session, then current process running must run as daemon and write to its assigned cached video memory */
    else
        map_virtual_4kb_prog(sess_desc[cur_sess_id].cached_vidmem, VMEM_VIRT_START);

    if(history_ptr[sess_id] <= (screen_head[sess_id] + NUM_COLS * (NUM_ROWS - 1))){
        enable_cursor(0, NUM_ROWS-1);
        update_cursor(sess_id);
    }
    set_vidmem_param(VMEM_VIRT_START); // set new video memory dependency
    return 0;
}



/* scroll_up
   description: helper function used by mouse.c to scroll up to see the terminal history
   input: none
   output: none
   return value: none
   side effect: none
   author: ZZ
*/
void scroll_up(void) {
    //if the screen head is on the boundary then don't scroll
    if(screen_head[cur_sess_id] == boundary[cur_sess_id]) return;

    int32_t i;
    //disable cursor when screen includes the last line
    if((screen_head[cur_sess_id] + NUM_COLS * (NUM_ROWS-1)) == history_ptr[cur_sess_id]) disable_cursor();

    screen_head[cur_sess_id] = screen_head[cur_sess_id] - NUM_COLS;

    draw_char(mouse_i.old_char, mouse_i.x, mouse_i.y);

    //redraw screen
    for (i = NUM_ROWS * NUM_COLS - 1; i > (NUM_COLS - 1); i--) {
        *(uint8_t *)(VIDEO + (i << 1)) = *(uint8_t *)(VIDEO + ((i-NUM_COLS) << 1));
        *(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
    }

    for (i = 0; i < NUM_COLS; i++) {
        *(uint8_t *)(VIDEO + (i << 1)) = *(uint8_t *)(screen_head[cur_sess_id] + i);
        *(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
    }

    mouse_i.old_char = get_char(mouse_i.x , mouse_i.y);
    draw_char('+', mouse_i.x, mouse_i.y);
}


/* scroll_down
   description: helper function used by mouse.c to scroll down to see the terminal history
   input: none
   output: none
   return value: none
   side effect: none
   author: ZZ
*/
void scroll_down(void) {
    //when the screen is out of boundary don't scroll
    if(((uint8_t *)screen_head[cur_sess_id] + NUM_COLS * (NUM_ROWS-1)) >= (uint8_t *)history_ptr[cur_sess_id]) return;
    screen_head[cur_sess_id] = screen_head[cur_sess_id] + NUM_COLS;
    if(((uint8_t *)screen_head[cur_sess_id] + NUM_COLS * (NUM_ROWS-1)) == (uint8_t *)history_ptr[cur_sess_id]){
        enable_cursor(0, NUM_ROWS-1);
        update_cursor(cur_sess_id);
    }

    int32_t i;
    draw_char(mouse_i.old_char, mouse_i.x, mouse_i.y);

    //redraw screen
    for (i = 0; i < (NUM_ROWS-1) * NUM_COLS; i++) {
        *(uint8_t *)(VIDEO + (i << 1)) = *(uint8_t *)(VIDEO + ((i+NUM_COLS) << 1));
        *(uint8_t *)(VIDEO + (i << 1) + 1) = *(uint8_t *)(VIDEO + ((i+NUM_COLS) << 1) + 1);
    }
    for (i = (NUM_ROWS-1) * NUM_COLS; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(VIDEO + (i << 1)) = *(uint8_t *)(screen_head[cur_sess_id] + i);
        *(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
    }
    mouse_i.old_char = get_char(mouse_i.x , mouse_i.y);
    draw_char('+', mouse_i.x, mouse_i.y);
}


/* terminal_read
   description: read from cmd_buf
   input: fd - file descriptor
          buf - dest buffer pointer
          nbytes - num of bytes to read
   output: none
   return value: -1 if fail, 0 if success
   side effect: none
   author: Kexuan Zou
*/
static int32_t terminal_fread(file_t * self, void * buf, uint32_t nbytes) {
    sti();
    // if buf is invalid or nbytes is invalid, return -1
    if (buf == NULL || nbytes < 0)
        return -1;

    int i; // loop iterator
    unsigned long flags;

    if (nbytes > KEY_BUF_SIZE) {
        nbytes = KEY_BUF_SIZE;
    }

    // Spin until the next enter.
    while (sess_desc[cur_proc_pcb->active_sess].cmd_available == 0);
    sess_desc[cur_proc_pcb->active_sess].cmd_available = 0;

    cli_and_save(flags); // critical section begins
    for (i = 0; i < nbytes; i++) {
        ((uint8_t *)buf)[i] = sess_desc[cur_proc_pcb->active_sess].cmd_buf[i]; // copy cmd_buf into target buffer
        if (sess_desc[cur_proc_pcb->active_sess].cmd_buf[i] == '\n') { // reaching newline char
            ((uint8_t *)buf)[i] = '\n';
            i++;
            break;
        }
    }
    sti();
    restore_flags(flags); // critical section ends
    return i;
}


/* terminal_write
   description: write to terminal screen
   input: fd - file descriptor
          buf - src buffer pointer
          nbytes - num of bytes to write
   output: none
   return value: -1 if fail, 0 if success
   side effect: none
   author: Kexuan Zou
*/
static int32_t terminal_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    if (buf == NULL || nbytes < 0) return -1; // if buf is invalid or nbytes is invalid, return -1
    int i; // loop iterator

    for (i = 0; i < nbytes; i++) {
        if (line_check(cur_proc_pcb->active_sess) && ((uint8_t *)buf)[i] != '\n') {
            sess_putc(((uint8_t *)buf)[i], cur_proc_pcb->active_sess);
            newline(cur_proc_pcb->active_sess);
            if (i + 1 < nbytes && ((uint8_t *)buf)[i + 1] == '\n') {
                i++;
            }
        } else {
            sess_putc(((uint8_t *)buf)[i], cur_proc_pcb->active_sess);
        }
    }
    if (cur_proc_pcb->active_sess == cur_sess_id)
        update_cursor(cur_proc_pcb->active_sess);
    return 0;
}


///
/// File `write` operation for stdin. Always fails.
///
/// - author: Zhengcheng Huang
///
static int32_t stdin_fwrite(file_t * self, const void * buf, uint32_t nbytes) {
    return -1;
}


///
/// `open` jump table function for stdin.
///
/// - author: Zhengcheng Huang
/// - arguments
///     filename: Ignore for stdin.
/// - return: Always success. Should be reconsidered.
/// - side effects: None. Should be reconsidered.
///
static int32_t stdin_fopen(file_t * self, const int8_t * filename) {
    return -1;
}

///
/// File `close` operation for stdin. Always fails.
///
/// - author: Zhengcheng Huang
static int32_t stdin_fclose(file_t * self) {
    return -1;
}

///
/// File `read` operation for stdout. Always fails.
///
/// - author: Zhengcheng Huang
///
static int32_t stdout_fread(file_t * self, void * buf, uint32_t nbytes) {
    return -1;
}



///
/// File `open` operation for stdout.
///
/// - author: Zhengcheng Huang
///
static int32_t stdout_fopen(file_t * file, const int8_t * filename) {
    return -1;
}

///
/// File `close` operation for stdout. Always fails.
///
/// - author: Zhengcheng Huang
///
static int32_t stdout_fclose(file_t * file) {
    return -1;
}

///
/// Creates an stdin file and stores in file array.
///
/// - author: Zhengcheng Huang
/// - side effects: Fills the first entry of file array with stdin.
///
void stdin_init(void) {
    file_t * file = cur_proc_pcb->fd_array + 0;

    file->f_op = stdin_fop;
    file->f_dentry.d_inode.i_ino = 0;
    file->f_pos = 0;

    cur_proc_pcb->fd_bitmap |= 0x1;
}

///
/// Creates an stdout file and stores in file array.
///
/// - author: Zhengcheng Huang
/// - side effects: Fills the second entry of file array with stdout.
///
void stdout_init(void) {
    file_t * file = cur_proc_pcb->fd_array + 1;

    file->f_op = stdout_fop;
    file->f_dentry.d_inode.i_ino = 0;
    file->f_pos = 0;

    cur_proc_pcb->fd_bitmap |= 0x2;
}

/* sess_putc
   description: session-specific putc
   input: c - char to put to screen
          sess_id - session id to print to
   output: none
   return value: none
   author: Kexuan Zou
*/
void sess_putc(uint8_t c, uint32_t sess_id) {
    if((screen_head[sess_id] + NUM_COLS * (NUM_ROWS-1)) < history_ptr[sess_id]) load_screen(sess_id);

    if(c == '\n' || c == '\r') {
        //write to history
        write_history(sess_desc[sess_id].vid_y, sess_id);
        update_historyptr(sess_id);
        sess_desc[sess_id].vid_y++;
        sess_desc[sess_id].vid_x = 0;
    } else {
        *(uint8_t *)(sess_mem + ((NUM_COLS * sess_desc[sess_id].vid_y + sess_desc[sess_id].vid_x) << 1)) = c;
        *(uint8_t *)(sess_mem + ((NUM_COLS * sess_desc[sess_id].vid_y + sess_desc[sess_id].vid_x) << 1) + 1) = ATTRIB;
        sess_desc[sess_id].vid_x++;
        sess_desc[sess_id].vid_x %= NUM_COLS;
        sess_desc[sess_id].vid_y = (sess_desc[sess_id].vid_y + (sess_desc[sess_id].vid_x / NUM_COLS)) % NUM_ROWS;

        write_history(sess_desc[sess_id].vid_y, sess_id);
    }
    check_scroll(sess_id);
}


/* sess_puts
   description: session-specific puts
   input: s - starting address of the string
          sess_id - session id to print to
   output: none
   return value: number of chars written to the screen
   author: Kexuan Zou
*/
int32_t sess_puts(int8_t* s, uint32_t sess_id) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        sess_putc(s[index], sess_id);
        index++;
    }
    return index;
}


/* set_vidmem_param
   description: set video memory parameters for sess_printf, sess_putc and sess_puts
   input: mem_start - starting address of video memory
   output: none
   return value: none
   side effect: none
*/
void set_vidmem_param(uint32_t mem_start) {
    sess_mem = (uint8_t* )mem_start;
}


/* get_vidmem_param
   description: get video memory parameter for sess_printf, sess_putc and sess_puts
   input: none
   output: video parameter
   return value: none
   side effect: none
*/
uint32_t get_vidmem_param(void) {
    return (uint32_t)sess_mem;
}



/* update_cursor
 * Inputs: sess_id - session id
 * Return Value: void
 * Function: update cursor with the corresponding x and y position */
void update_cursor(uint32_t sess_id) {
  int x, y;
  x = sess_desc[sess_id].vid_x;
  y = sess_desc[sess_id].vid_y;
  uint16_t pos = y * NUM_COLS + x;

  outb(0x0F,0x3D4);
  outb((uint8_t) (pos & 0xFF),0x3D5);
  outb(0x0E,0x3D4);
  outb((uint8_t) ((pos >> 8) & 0xFF),0x3D5);
}


/* enable_scroll
 * Inputs: sess_id - sesssion id
 * Return Value: void
 * Function: enable_scrool vertically scrolls the terminal screen by one line */
void enable_scroll(uint32_t sess_id) {
    int32_t i;
    draw_char(mouse_i.old_char, mouse_i.x, mouse_i.y);
    screen_head[sess_id] = screen_head[sess_id] + NUM_COLS;

    for (i = 0; i < (NUM_ROWS-1) * NUM_COLS; i++) {
        *(uint8_t *)(sess_mem + (i << 1)) = *(uint8_t *)(sess_mem + ((i+NUM_COLS) << 1));
        *(uint8_t *)(sess_mem + (i << 1) + 1) = *(uint8_t *)(sess_mem + ((i+NUM_COLS) << 1) + 1);
    }
    sess_desc[sess_id].vid_y--;
    for (i = (NUM_ROWS-1) * NUM_COLS; i < NUM_ROWS * NUM_COLS; i++) {
            *(uint8_t *)(sess_mem + (i << 1)) = ' ';
            *(uint8_t *)(sess_mem + (i << 1) + 1) = ATTRIB;
    }
    mouse_i.old_char = get_char(mouse_i.x , mouse_i.y);
    draw_char('+', mouse_i.x, mouse_i.y);
}


/* check_scroll
 * Inputs: sess_id - session id
 * Return Value: integer, true or false
 * Function: checks whether screen_y is out of bound, if so, enable scroll */
static uint8_t check_scroll(uint32_t sess_id) {
    if(sess_desc[sess_id].vid_y >= NUM_ROWS){
        enable_scroll(sess_id);
        return 1;
    }
    return 0;
}


/* clear
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(sess_mem + (i << 1)) = ' ';
        *(uint8_t *)(sess_mem + (i << 1) + 1) = ATTRIB;
        *(uint8_t *)(VIDEO + (i << 1)) = ' ';
        *(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
    }
    disable_cursor();
    enable_cursor(0, NUM_ROWS-1);
    screen_head[cur_sess_id] = boundary[cur_sess_id];
    history_ptr[cur_sess_id] = boundary[cur_sess_id];
}


/* newline
   description: move to newline
   input: sess_id - session id
   output: none
   return value: none
   side effect: change screen_x, screen_y
   author: Kexuan Zou
*/
void newline(uint32_t sess_id) {
    if((screen_head[sess_id] + NUM_COLS * (NUM_ROWS-1)) < history_ptr[sess_id]) load_screen(sess_id);
    write_history(sess_desc[sess_id].vid_y, sess_id);
    update_historyptr(sess_id);
    sess_desc[sess_id].vid_x = 0;
    sess_desc[sess_id].vid_y++;
    check_scroll(sess_id);
}


/* line_check
   description: check if need to move to next line
   input: sess_id
   output: none
   return value: if currently at line end, return nonzero; else return 0
   side effect: change screen_x, screen_y
   author: Kexuan Zou
*/
int line_check(uint32_t sess_id) {
  if(sess_desc[sess_id].vid_x == NUM_COLS-1){
    return 1;
  }
  else return 0;
}


/* write_history
   description: write the current line to history page
   input: vid_x, vid_y, line count
   output: none
   return value: none
   side effect: change the contents in history page
   author: ZZ
*/
void write_history(uint32_t y, uint32_t sess_id) {
    int32_t i;
    for (i = 0; i < NUM_COLS; i++) {
      *(uint8_t *)(history_ptr[sess_id] + i) = *(uint8_t *)(sess_mem + ((y * NUM_COLS + i) << 1));
    }
}

/* update_historyptr
   description: update history pointer
   input: sess_id
   output: none
   return value: none
   side effect: change the contents in history page
   author: ZZ
*/
void update_historyptr(uint32_t sess_id) {
    history_ptr[sess_id] += NUM_COLS;
}


/* load_screen
   description: load a new screen from history page
   input: sess_id
   output: none
   return value: none
   side effect: change the contents in video memory
   author: ZZ
*/
void load_screen(uint32_t sess_id) {
    screen_head[cur_sess_id] = (int32_t)history_ptr[cur_sess_id] - NUM_COLS * (NUM_ROWS-1);

    draw_char(mouse_i.old_char, mouse_i.x, mouse_i.y);

    //draw screen
    int i;
    for(i = 0; i < NUM_ROWS * NUM_COLS; i++){
        *(uint8_t *)(VIDEO + (i << 1)) = *(uint8_t *)(screen_head[cur_sess_id] + i);
        *(uint8_t *)(VIDEO + (i << 1) + 1) = ATTRIB;
    }

    mouse_i.old_char = get_char(mouse_i.x , mouse_i.y);
    draw_char('+', mouse_i.x, mouse_i.y);
    enable_cursor(0, NUM_ROWS-1);
}


/* backspace
   description: delete a char at (screen_x, screen_y) by adding a ' '
   input: sess_id - session id
   output: none
   return value: if retrace to the previous line, return 1; else return 0
   side effect: erase a single char in video memory
   author: Kexuan Zou
*/
void backspace(uint32_t sess_id) {
    if((screen_head[sess_id] + NUM_COLS * (NUM_ROWS-1)) < history_ptr[sess_id]) load_screen(sess_id);
    if (sess_desc[sess_id].vid_x)
        sess_desc[sess_id].vid_x--;
    else {
        sess_desc[sess_id].vid_x = NUM_COLS - 1;
        sess_desc[sess_id].vid_y--;
    }
    *(uint8_t *)(sess_mem + ((sess_desc[sess_id].vid_y * NUM_COLS + sess_desc[sess_id].vid_x) << 1)) = ' ';
    *(uint8_t *)(sess_mem + ((sess_desc[sess_id].vid_y * NUM_COLS + sess_desc[sess_id].vid_x) << 1) + 1) = ATTRIB;
    write_history(sess_desc[sess_id].vid_y, sess_id);
    update_cursor(sess_id);
}


/* clear_screen
   description: clear the screen (trigerred by ctrl+l)
   input: none
   output: none
   return value: none
   side effect: erase the entire video memory
   author: Kexuan Zou
*/
void clear_screen(uint32_t sess_id) {
    sess_desc[sess_id].vid_x = sess_desc[sess_id].vid_y = 0;
    clear();
    update_cursor(sess_id);
}
