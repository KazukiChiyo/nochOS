/* mouse.c - Mouse driver for PS/2 mouse. NOTE: This is providing full support for mouse;
   author: ZZ
   date: please guess
   external source: http://wiki.osdev.org/Mouse_Input */

#include <mouse.h>
#include <x86_desc.h>
#include <lib.h>
#include <i8259.h>
#include <idt.h>
#include <interrupt.h>
#include <terminal.h>
#include <syscall.h>
#include <system.h>

mouse_info mouse_i;
static int x_pos;
static int y_pos;
static uint8_t old_char;

static uint8_t read_port(void);
static void command_ready(void);
static void output_to_port(uint8_t command, uint8_t port);
static void clear_port(void);

/* mouse_init
   description: set idt entry for mouse and enable irq 12
   input: none
   output: none
   return value: none
   side effect:
*/

void mouse_init(void){
    int_set_idt(MOUSE_IRQ_PIN); // set idt entry for mouse
    enable_irq(MOUSE_IRQ_PIN); // enable irq for mouse
    uint8_t status;
    x_pos = 320/2;
    y_pos = 200/2;
    old_char = get_char(x_pos, y_pos);

    mouse_i.x = (uint32_t)x_pos;
    mouse_i.y = (uint32_t)y_pos;
    mouse_i.old_char = old_char;
    //printf("old_char: %d\n", old_char);

    //resetting the mouse
    command_ready();
    output_to_port(0xFF, 0x60);

    //clear the read port, wait for 0xAA after resetting
    while(read_port()!=0xAA);

    //get compaq status byte
    output_to_port(0x20, 0x64);
    status = inb(0x60);

    //enable auxiliary device
    output_to_port(0xA8, 0x64);

    //turn on packet streaming
    command_ready();
    output_to_port(0xF4, 0x60);

    status |= 0x02;
    status &= 0xDF;
    output_to_port(0x60, 0x64);
    output_to_port(status, 0x60);

    //now setting sample rate to 200, 100, 80 to enable scrolling
    //see OSDev for reference
    //200
    command_ready();
    output_to_port(0xF3, 0x60);
    command_ready();
    output_to_port(200, 0x60);

    //100
    command_ready();
    output_to_port(0xF3, 0x60);
    command_ready();
    output_to_port(100, 0x60);

    //80
    command_ready();
    output_to_port(0xF3, 0x60);
    command_ready();
    output_to_port(80, 0x60);
    
    //resetting sample rate
    //100
    command_ready();
    output_to_port(0xF3, 0x60);
    command_ready();
    output_to_port(100, 0x60);

    draw_char(MOUSE_ICON, x_pos, y_pos);
}


/* mouse_handler
   description: handle a single keyboard interrupt
   input: none
   output: none
   return value: none
   side effect: write to video memory
*/
void mouse_handler(void){
    unsigned long flags;
    cli_and_save(flags); // critical section begins
    uint8_t packet[3];
    if(!(inb(0x64) & 0x20)){
      sti();
      restore_flags(flags); // critical section ends
      clear_port();
      return;
    }

    packet[0] = read_port();  
    //check for overflow and always1, if they are set, discard the entire packet
    if((packet[0] & 0xc0) || !(packet[0] & 0x08) || (packet[0] & 0x07)){
      sti();
      restore_flags(flags); // critical section ends
      clear_port();
      return;
    }

    packet[1] = read_port();
    packet[2] = read_port();
    packet[3] = read_port();

    draw_char(mouse_i.old_char, x_pos, y_pos);

    //update x pos
    if(packet[0] & 0x10) x_pos += packet[1] | 0xffffff00;
    else x_pos += packet[1];

    //update y pos
    if(packet[0] & 0x20) y_pos -= packet[2] | 0xffffff00;
    else y_pos -= packet[2];

    if(x_pos > 319) x_pos = 319;
    if(x_pos < 0) x_pos = 0;
    if(y_pos > 199) y_pos =199;
    if(y_pos < 0) y_pos =0;

    old_char = get_char(x_pos, y_pos);

    draw_char(MOUSE_ICON, x_pos, y_pos);
    mouse_i.x = (uint32_t)x_pos;
    mouse_i.y = (uint32_t)y_pos;
    mouse_i.old_char = old_char;

    if(packet[3] == 0xff){
      scroll_up();
    }
    else if(packet[3] == 1){
      scroll_down();
    }
    //TODO: left or right button

    sti();
    restore_flags(flags); // critical section ends
    clear_port();
}


/* read_port
   description: helper function to read byte from the port 0x60
   input: none
   output: one byte read from the port
   side effect: none
*/

uint8_t read_port(){
  uint8_t availability;
  while(1){
    availability = inb(0x64);
    if((availability & 0x01) && (availability & 0x20)){
        break;
        //printf("stuck\n");
    }
  }
  return inb(0x60);
}

/* command_ready
   description: sending a command or data type to the port 0x60 must be preceded be sending 0xD4 to 0x64
   input: none
   output: none
   side effect: none //not: 0xD4 doesn't generate ACK
*/

void command_ready(){
  while(inb(0x64) & 0x02);
  outb(0xD4, 0x64);
}

/* output_to_port
   description: sending a command or data type to the port 0x60 must be preceded be sending 0xD4 to 0x64
   input: command
   output: none
   side effect: none
*/

void output_to_port(uint8_t output, uint8_t port){
  while(inb(0x64) & 0x02);
  outb(output, port);
}

/* clear_port
   description: clear up the mouse packets in 0x60 so that keyboard won't stuck
   output: none
   side effect: none
*/

void clear_port(){
  uint8_t availability;
  uint8_t temp;
  while(1){
    availability = inb(0x64);
    if((availability & 0x01) && (availability & 0x20)){
      temp = inb(0x60);
    }
    else{
      break;
    }
  }
}

/* draw_char
    description: draw charactor on the screen
    input: new charactor and position of x and y
    output: none
    side effect: none
*/
void draw_char(uint8_t new_char, uint32_t x_pos, uint32_t y_pos){
  *(uint8_t *)((char *)VIDEO + (y_pos / CHAR_Y * NUM_COLS + x_pos / CHAR_X) *2 +1) = new_char;
  return;
}

/* get_char
    description: get charactor on the screen
    input: position of x and y
    output: none
    side effect: none
*/
uint8_t get_char(uint8_t x_pos, uint8_t y_pos){
  return *(uint8_t *)((char *)VIDEO + (y_pos / CHAR_Y * NUM_COLS + x_pos / CHAR_X) *2 +1);
}
