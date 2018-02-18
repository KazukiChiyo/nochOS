#ifndef _SYSTEM_H
#define _SYSTEM_H

#define AES_FAST_CALC // enable fast (inv) mix column calculation
#define LEGACY_MODE // legacy mode for cpu

#define BITS_BYTE 8
#define BITS_LONG 32
#define LONG_SIZE (BITS_LONG / BITS_BYTE)

#define MODEX_PAGE_SIZE 0x1000 // modex vidmem page size is 4kb
#define MODEX_PAGE_NUM 16 // modex uses 16 consecutive pages
#define KERNEL_TOP 0x400000 // start of kernel page
#define KERNEL_SIZE 0x400000 // kernel page size is 4mb
#define HEAP_SIZE 0x400000 // heap size in total is 4mb
#define HEAP_PHYS_TOP 0x2000000 // heap physical address top is 32MB
#define HEAP_VIRT_TOP 0x800000 // heap virtual address top is 8MB
#define USER_VIRT_TOP 0x8000000 // user virtual memory address starts at 128MB
#define USER_VIRT_BOT 0x8400000 // user virtual memory address ends at 132MB
#define PROG_VIRT_START 0x08048000 // user program starting address starts here
#define VMEM_VIRT_START 0x8400000 // video memory starting address is 132MB
#define MODEX_VIRT_START (VMEM_VIRT_START + MODEX_PAGE_SIZE) // modex mode virtual start address
#define USER_STACK_TOP 0x8200000 // user stack section starts here
#define USER_HEAP_BOT USER_STACK_TOP // user heap section ends here

#define ENOMEM 12 // out of memory
#define	EBUSY 16 // device or resource busy

#define KEYBOARD_IRQ_PIN 1 // pin for keyboard is IRQ1
#define MOUSE_IRQ_PIN 12 // pin for mouse is IRQ12

#define PRW_PORT 0x2000 // power port
#define CMD_SHUTDOWN 0xb004 // command for shutdown

#define MEMORY_MAGIC 0xA5A5A5A5 // memory checker magic number

#define PWR_OFF shutdown()
#define IO_OFF io_disable()
#define IO_ON io_enable()
#define STACK_BARRIER stack_barrier()
#define CHK_USR_ESP chk_usr_esp(regs)
#define USER_HEAP_TOP (cur_proc_pcb->prog_break + LONG_SIZE) // user heap section starts here

#define shutdown()  \
    outw(PRW_PORT, CMD_SHUTDOWN);

#define io_disable()               \
    disable_irq(KEYBOARD_IRQ_PIN); \
    disable_irq(MOUSE_IRQ_PIN);

#define io_enable()               \
    enable_irq(KEYBOARD_IRQ_PIN); \
    enable_irq(MOUSE_IRQ_PIN);

#define SET_BRK(addr)                                    \
    cur_proc_pcb->prog_break = align_addr_long(addr);    \
    *(uint32_t* )cur_proc_pcb->prog_break = MEMORY_MAGIC;

#define stack_barrier()  \
    *(uint32_t* )USER_STACK_TOP = MEMORY_MAGIC;

/**
 * chk_usr_esp - check user esp, if privilege escalation occurs, kill the program immediately
 * @param regs - register struct
 * */
#define chk_usr_esp(regs)  \
    if ((regs->orig_eax) != SYS_HALT && (regs->orig_eax) != SYS_EXECUTE) { \
        if ((regs->esp) <= USER_STACK_TOP || (regs->esp) >= USER_VIRT_BOT - LONG_SIZE) { \
            request_signal(STACKFAULT, cur_proc_pcb); \
            return; \
        } \
    }

/**
 * align_addr_long - align address to 4 bytes
 * @param addr - address to align
 * @return - next aligned address
 */
static inline uint32_t align_addr_long(uint32_t addr) {
    if (!(addr % BITS_LONG)) return addr;
    return (addr + BITS_LONG - addr % BITS_LONG);
}

#endif
