/* keyboard.h - Interface of keyboard functionalities.
 * - author: Kexuan Zou, Zhengcheng Huang
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define DISABLE 0
#define ENABLE 1

#include <x86_desc.h>
#include <lib.h>
#include <i8259.h>
#include <types.h>
#include <terminal.h>


/* extern functions used by kernel.c */
extern void keyboard_handler(void);
extern void keyboard_init(void);

#endif /* KEYBOARD */
