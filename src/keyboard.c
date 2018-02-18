/* keyboard.c - Keyboard driver for PS/2 keyboard. NOTE: This is only to provide minimal support for keyboard; num pad, multimedia keys, right ctrl, right alt are not supported.
   author: Kexuan Zou
   date: 10/20/2017
   external source: http://wiki.osdev.org/PS/2_Keyboard */

#include <keyboard.h>
#include <x86_desc.h>
#include <lib.h>
#include <i8259.h>
#include <idt.h>
#include <interrupt.h>
#include <terminal.h>
#include <syscall.h>
#include <signal.h>
#include <system.h>

#define READ_INPUT_PORT 0x60 // r / w port for keyboard is 0x60

/* left shift, right shift, and ctrl key have "pressed" and "released" states for combo key inputs (i.e. CTRL + Z is different from SHIFT + Z) */
#define LSHIFT_PRESSED 0x2A
#define LSHIFT_RELEASED 0xAA
#define RSHIFT_PRESSED 0x36
#define RSHIFT_RELEASED 0xB6
#define LCTRL_PRESSED 0x1D
#define LCTRL_RELEASED 0x9D
#define LALT_PRESSED 0x38
#define LALT_RELEASED 0xB8

/* CAPS LOCK key toggles between caps on and caps off */
#define CAPS_LOCK_PRESSED 0x3A
#define CAPS_LOCK_RELEASED 0xBA

/* backspace, enter, and tab keys are function specific keys that need to be handled separately */
#define BACKSPACE 0x0E
#define ENTER 0x1C
#define TAB 0x0F

#define REG_MIN_PRESSED 0x01
#define REG_MAX_PRESSED 0x39
#define REG_MIN_RELEASED 0x81
#define REG_MAX_RELEASED 0xB9
#define NUM_KEY 58

/* function keys */
#define F1_PRESSED 0x3B
#define F2_PRESSED 0x3C
#define F3_PRESSED 0x3D

/* private functions visible to this file */
void keyboard_set_idt(void);
void print_regular_key(uint8_t keycode);
void delete_keystroke(void);
void set_newline(void);
void cmd_cached_store(void);

typedef struct keyboard_state_t {
    uint8_t shift_en; // determine whether shift is pressed or released
    uint8_t caps_en; // determine whether caps lock is pressed or released
    uint8_t caps_status; // check whether caps lock is active
    uint8_t ctrl_en; // determine whether ctrl is pressed or released
    uint8_t alt_en; // determine whether alt is pressed or released
    uint8_t reg_key_en; // determine whether a regular key is pressed or released
} keyboard_state_t;

/* initialize a global instance of the struct */
static keyboard_state_t key_info;

/* regular key-mapping to map scancodes with ascii keys(0x00 - 0x32) in normal case; unused keys are mapped to 0x00 */
static uint8_t key_normal[NUM_KEY] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
                                      'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0',
                                      'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\',
                                      'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0',
                                      '*', '\0', ' '};

/* regular key-mapping to map scancodes with ascii keys(0x00 - 0x32) when shift key is pressed;; unused keys are mapped to 0x00 */
static uint8_t key_shift[NUM_KEY] = {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
                                     'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0', '\0',
                                     'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', '\0', '|',
                                     'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\0',
                                     '*', '\0', ' '};

/* regular key-mapping to map scancodes with ascii keys(0x00 - 0x32) when caps lock is pressed;; unused keys are mapped to 0x00 */
static uint8_t key_capslock[NUM_KEY] = {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
                                        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0', '\0',
                                        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', '\0', '\\',
                                        'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', '\0',
                                        '*', '\0', ' '};

/* regular key-mapping to map scancodes with ascii keys(0x00 - 0x32) when both shift and caps lock are pressed;; unused keys are mapped to 0x00 */
static uint8_t key_shiftcaps[NUM_KEY] = {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
                                         'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0', '\0',
                                         'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '|', '~', '\0', '|',
                                         'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', '\0',
                                         '*', '\0', ' '};


/* keyboard_init
   description: set idt entry for keyboard, initialize key_info struct, enable irq for keyboard.
   input: none
   output: none
   return value: none
   side effect: unmask keyboard irq pin
*/
void keyboard_init(void) {
    int i; // keyboard buffer iterator
    int_set_idt(KEYBOARD_IRQ_PIN); // set idt entry for keyboard

    /* initialize key_info struct */
    key_info.shift_en = DISABLE;
    key_info.caps_en = DISABLE;
    key_info.caps_status = DISABLE;
    key_info.ctrl_en = DISABLE;
    key_info.alt_en = DISABLE;
    key_info.reg_key_en = DISABLE;

    for (i = 0; i < KEY_BUF_SIZE; i++)
        sess_desc[cur_sess_id].kbd_buf[i] = 0x00;
    sess_desc[cur_sess_id].kbd_buf_idx = 0;
    enable_irq(KEYBOARD_IRQ_PIN); // enable irq for keyboard
}


/* keyboard_handler
   description: handle a single keyboard interrupt
   input: none
   output: none
   return value: none
   side effect: write to video memory
*/
void keyboard_handler(void) {
    uint8_t keycode = inb(READ_INPUT_PORT);
    uint32_t prev;
    if (keycode == 0x00) return; // if keycode is null, return
    switch(keycode) {
        /* left shift key is pressed or released, toggle shift_en */
        case LSHIFT_PRESSED:
            key_info.shift_en = ENABLE;
            break;
        case LSHIFT_RELEASED:
            key_info.shift_en = DISABLE;
            break;

        /* right shift key is pressed or released, toggle shift_en */
        case RSHIFT_PRESSED:
            key_info.shift_en = ENABLE;
            break;
        case RSHIFT_RELEASED:
            key_info.shift_en = DISABLE;
            break;

        /* left ctrl key is pressed or released, toggle ctrl_en */
        case LCTRL_PRESSED:
            key_info.ctrl_en = ENABLE;
            break;
        case LCTRL_RELEASED:
            key_info.ctrl_en = DISABLE;
            break;

        /* left alt key is pressed or released, toggle alt_en */
        case LALT_PRESSED:
            key_info.alt_en = ENABLE;
            break;
        case LALT_RELEASED:
            key_info.alt_en = DISABLE;
            break;

        /* caps lock is pressed or released, toggle caps_en; NOTE: whenever a caps lock registers a keystroke, toggle caps_status */
        case CAPS_LOCK_PRESSED:
            key_info.caps_en = ENABLE;
            key_info.caps_status = ~(key_info.caps_status);
            break;
        case CAPS_LOCK_RELEASED:
            key_info.caps_en = DISABLE;
            break;

        /* backspace is pressed */
        case BACKSPACE:
            delete_keystroke();
            break;

        /* enter is pressed */
        case ENTER:
            prev = get_vidmem_param();
            set_vidmem_param(VIDEO);
            set_newline();
            update_cursor(cur_sess_id);
            sess_desc[cur_sess_id].cmd_available = 1;
            set_vidmem_param(prev);
            break;

        /* tab is pressed */
        case TAB:
            break;

        /* possible terminal switch can occur */
        case F1_PRESSED:
            if (key_info.alt_en) {
                sess_switch(0);
                break;
            }
            else break;

        case F2_PRESSED:
            if (key_info.alt_en) {
                sess_switch(1);
                break;
            }
            else break;

        case F3_PRESSED:
            if (key_info.alt_en) {
                sess_switch(2);
                break;
            }
            else break;

        /* if key is an ascii key, print it on the screen (for now) */
        default:
            if (keycode >= REG_MIN_PRESSED && keycode <= REG_MAX_PRESSED) { // if a regular-mapping key is pressed
                key_info.reg_key_en = ENABLE;
                print_regular_key(keycode);
                break;
            }
            else if (keycode >= REG_MIN_RELEASED && keycode <= REG_MAX_RELEASED) { // if a regular-mapping key is released
                key_info.reg_key_en = DISABLE;
                break;
            }
            else break;
    }
}


/* print_regular_key
   description: print a regular array-mapping key to video memory
   input: keycode - scancode of that key to print
   output: none
   return value: none
   side effect: write a single char to video memory, append to keyboard buffer
*/
void print_regular_key(uint8_t keycode) {
    int i;
    uint8_t key = 0x00;
    if (!(key_info.shift_en) && !(key_info.caps_en) && !(key_info.caps_status)) // normal key mapping
        key = key_normal[keycode];
    if (!(key_info.shift_en) && !(key_info.caps_en) && key_info.caps_status) // caps lock is on
        key = key_capslock[keycode];
    else if (key_info.shift_en && !(key_info.caps_en) && !(key_info.caps_status)) // only shift is pressed
        key = key_shift[keycode];
    else if (key_info.shift_en && !(key_info.caps_en) && key_info.caps_status) // shift and caps lock is on
        key = key_shiftcaps[keycode];
    else if (!(key_info.shift_en) && key_info.caps_en && !(key_info.caps_status)) // caps lock is toggled off
        key = key_normal[keycode];
    else if (!(key_info.shift_en) && key_info.caps_en && key_info.caps_status) // caps lock is toggled on
        key = key_capslock[keycode];
    else if (key_info.shift_en && key_info.caps_en && !(key_info.caps_status)) // only shift is pressed
        key = key_shift[keycode];
    else if (key_info.shift_en && key_info.caps_en && key_info.caps_status) // both shift and caps lock is pressed
        key = key_shiftcaps[keycode];
    if (key_info.ctrl_en && key == 'l') { // if ctrl+l is pressed, clear the screen
        uint32_t prev = get_vidmem_param();
        set_vidmem_param(VIDEO);
        clear_screen(cur_sess_id);
        set_vidmem_param(prev);
        for (i = 0; i < KEY_BUF_SIZE; i++) { // clear keyboard buffer
            sess_desc[cur_sess_id].kbd_buf[i] = 0x00;
            sess_desc[cur_sess_id].cmd_buf[0] = 0x00;
        }
        sess_desc[cur_sess_id].kbd_buf_idx = 0;
        sess_desc[cur_sess_id].cmd_available = 1;
        return;
    }
    if (key_info.ctrl_en && key == 'c') { // if ctrl+c is pressed, kill the process
        request_signal(INTERRUPT, sess_desc[cur_sess_id].sa_pcb);
        return;
    }
    if (key == 0x00) return; // if key pressed mapped to a null key, just return
    /* if keyboard buffer is not full, append key into keyboard buffer, print that char to terminal */
    if (sess_desc[cur_sess_id].kbd_buf_idx < KEY_BUF_SIZE - 1) {
        sess_desc[cur_sess_id].kbd_buf[sess_desc[cur_sess_id].kbd_buf_idx] = key; // store key to keyboard buffer
        sess_desc[cur_sess_id].kbd_buf_idx++;
        uint32_t prev = get_vidmem_param();
        set_vidmem_param(VIDEO);
        if (line_check(cur_sess_id)) {
            sess_putc(key, cur_sess_id);
            newline(cur_sess_id);
        }
        else
            sess_putc(key, cur_sess_id);

        update_cursor(cur_sess_id);
        set_vidmem_param(prev);
    }
}


/* delete_keystroke
   description: delete a single keystroke, remove that key from keyboard buffer
   input: none
   output: none
   return value: none
   side effect: erase a single char to video memory, delete from keyboard buffer
*/
void delete_keystroke(void) {
    if (sess_desc[cur_sess_id].kbd_buf_idx) {
        uint32_t prev = get_vidmem_param();
        set_vidmem_param(VIDEO);

        backspace(cur_sess_id);
        sess_desc[cur_sess_id].kbd_buf_idx--;
        sess_desc[cur_sess_id].kbd_buf[sess_desc[cur_sess_id].kbd_buf_idx] = 0x00; // delete key from keyboard buffer
        set_vidmem_param(prev);
    }
}


/* set_newline
   description: set newline, execute the command in keyboard buffer
   input: none
   output: none
   return value: none
   side effect: insert a newline to video memory, execute commandin keyboard buffer
*/
void set_newline(void) {
    int i; // loop iterator
    if (sess_desc[cur_sess_id].kbd_buf_idx < KEY_BUF_SIZE) {
        sess_desc[cur_sess_id].kbd_buf[sess_desc[cur_sess_id].kbd_buf_idx] = '\n';
        cmd_cached_store(); // store command to cmd buffer
        for (i = 0; i < KEY_BUF_SIZE; i++) // clear keyboard buffer
            sess_desc[cur_sess_id].kbd_buf[i] = 0x00;
        sess_desc[cur_sess_id].kbd_buf_idx = 0;
    }
    newline(cur_sess_id);
}


/* cmd_cach_store
   description: store content from kbd_buf to cmd_buf
   input: none
   output: none
   return value: none
   side effect: changes video memory
*/
void cmd_cached_store(void) {
    strncpy((int8_t *)sess_desc[cur_sess_id].cmd_buf, (int8_t *)sess_desc[cur_sess_id].kbd_buf, KEY_BUF_SIZE);
}
