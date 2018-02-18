/* i8259.c - Functions to interact with the 8259 interrupt controller
   author: Kexuan Zou
   date: 10/18/2017
   external source: http://www.brokenthorn.com/Resources/OSDevPic.html */

#include <i8259.h>
#include <lib.h>

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = IRQ_MASK_SET; /* IRQs 0-7  */
uint8_t slave_mask = IRQ_MASK_SET;  /* IRQs 8-15 */


/* i8259_init
   description: initialize master and slave pics, write control words into both pics
   input: none
   output: none
   return value: none
   side effect: none
*/
void i8259_init(void) {
    unsigned long flags;
    cli_and_save(flags); // critical section begins
    
    /* write ICW1 to command register of both pics */
    outb(ICW1, MASTER_PORT_CTRL);
    outb(ICW1, SLAVE_PORT_CTRL);
    
    /* write ICW2 to interrupt mask register of both pics */
    outb(ICW2_MASTER, MASTER_PORT_DATA);
    outb(ICW2_SLAVE, SLAVE_PORT_DATA);
    
    /* write ICW3 to interrupt mask register of both pics */
    outb(ICW3_MASTER, MASTER_PORT_DATA);
    outb(ICW3_SLAVE, SLAVE_PORT_DATA);
    
    /* write ICW4 to interrupt mask register of both pics */
    outb(ICW4, MASTER_PORT_DATA);
    outb(ICW4, SLAVE_PORT_DATA);
    
    /* disable interrupts on both pics */
    outb(IRQ_MASK_SET, MASTER_PORT_DATA);
    outb(IRQ_MASK_SET, SLAVE_PORT_DATA);
    
    sti();
    restore_flags(flags); // critical section ends
    enable_irq(IRQ_PIN); // enable cascading 
}


/* enable_irq
   description: unmask irq interrupt and write OCW1 to corresponding IMR of the pic
   input: irq_num - irq pin requested to enable
   output: none
   return value: none
   side effect: unmask irq interrupt
*/
void enable_irq(uint32_t irq_num) {
    if (irq_num > SLAVE_IRQ_MAX || irq_num < MASTER_IRQ_MIN) // if requested #IRQ is not valid
        return;

    uint8_t irq_mask; // irq mask
    
    /* if irq is in master pic */
    if (irq_num >= MASTER_IRQ_MIN && irq_num <= MASTER_IRQ_MAX) {
        irq_mask = ~(IRQ_BIT_MASK << irq_num);
        master_mask &= irq_mask;
        outb(master_mask, MASTER_PORT_DATA);
    }
    
    /* if irq is in slave pic */
    else if (irq_num >= SLAVE_IRQ_MIN && irq_num <= SLAVE_IRQ_MAX) {
        irq_mask = ~(IRQ_BIT_MASK << (irq_num - SLAVE_IRQ_MIN));
        slave_mask &= irq_mask;
        outb(slave_mask, SLAVE_PORT_DATA);
    }
}


/* disable_irq
   description: mask irq interrupt and write OCW1 to corresponding IMR of the pic
   input: irq_num - irq pin requested to disable
   output: none
   return value: none
   side effect: mask irq interrupt
*/
void disable_irq(uint32_t irq_num) {
    if (irq_num > SLAVE_IRQ_MAX || irq_num < MASTER_IRQ_MIN) // if requested #IRQ is not valid
        return;

    uint8_t irq_mask; // irq mask
        
    /* if irq is in master pic */
    if (irq_num >= MASTER_IRQ_MIN && irq_num <= MASTER_IRQ_MAX) {
        irq_mask = IRQ_BIT_MASK << irq_num;
        master_mask |= irq_mask;
        outb(master_mask, MASTER_PORT_DATA); // write OCW1 to IMR of master pic
    }
        
    /* if irq is in slave pic */
    else if (irq_num >= SLAVE_IRQ_MIN && irq_num <= SLAVE_IRQ_MAX) {
        irq_mask = IRQ_BIT_MASK << (irq_num - SLAVE_IRQ_MIN);
        slave_mask |= irq_mask;
        outb(slave_mask, SLAVE_PORT_DATA); // write OCW2 to IMR of slave pic
    }
}


/* send_eoi
   description: send OCW2 to specify a EOI
   input: irq_num - irq pin to declare termination of interrupt
   output: none
   return value: none
   side effect: none
*/
void send_eoi(uint32_t irq_num) {
    if (irq_num > SLAVE_IRQ_MAX || irq_num < MASTER_IRQ_MIN) // if requested #IRQ is not valid
        return;
	
    /* if irq is in master pic */
    if (irq_num >= MASTER_IRQ_MIN && irq_num <= MASTER_IRQ_MAX)
        outb(EOI | irq_num, MASTER_PORT_CTRL); // write OCW2 to IMR of master pic
        
    /* if irq is in slave pic */
    else if (irq_num >= SLAVE_IRQ_MIN && irq_num <= SLAVE_IRQ_MAX) {
        outb(EOI | (irq_num - SLAVE_IRQ_MIN), SLAVE_PORT_CTRL); // write OCW2 to IMR of slave pic
        outb(EOI | IRQ_PIN, MASTER_PORT_CTRL); // write OCW2 to IMR of master pic
    }
}
