/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _I8259_H
#define _I8259_H

#include <types.h>

/* Ports that each PIC sits on */
#define MASTER_PORT_CTRL    0x20
#define MASTER_PORT_DATA    0x21
#define SLAVE_PORT_CTRL     0xA0
#define SLAVE_PORT_DATA     0xA1

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1                0x11
#define ICW2_MASTER         0x20
#define ICW2_SLAVE          0x28
#define ICW3_MASTER         0x04
#define ICW3_SLAVE          0x02
#define ICW4                0x01

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI                 0x60

#define MASTER_IRQ_MIN      0
#define MASTER_IRQ_MAX      7
#define SLAVE_IRQ_MIN       8
#define SLAVE_IRQ_MAX       15
#define IRQ_BIT_MASK        0x01
#define IRQ_MASK_SET        0xFF

//cascaded signals uses IRQ2. src: https://en.wikipedia.org/wiki/Interrupt_request_(PC_architecture)
#define IRQ_PIN 2

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);

#endif /* _I8259_H */
