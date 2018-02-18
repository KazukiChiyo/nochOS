/* mouse.h - Interface of mouse functionalities.
 * - author: Zixu Zhao
 */

#ifndef _MOUSE_H
#define _MOUSE_H

#define READ_INPUT_PORT 0x60 // r / w port for keyboard is 0x60
#define ACK 0xFA //acknowledge
#define CHAR_X 4
#define CHAR_Y 8
#define MOUSE_ICON '+'

#include <x86_desc.h>
#include <lib.h>
#include <i8259.h>
#include <types.h>
#include <terminal.h>

typedef struct mouse_info{
  uint32_t x;
  uint32_t y;
  uint8_t old_char;
}mouse_info;

extern mouse_info mouse_i;

/* extern functions used by kernel.c */
extern void mouse_handler(void);
extern void mouse_init(void);
extern uint8_t get_char(uint8_t x_pos, uint8_t y_pos);
extern void draw_char(uint8_t new_char, uint32_t x_pos, uint32_t y_pos);

#endif /* MOUSE */
