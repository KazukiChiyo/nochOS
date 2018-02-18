/* regs.h: Enables C functions to manipulation register info on the stack.
 * - author: Zhengcheng Huang
 */

#ifndef _REGS_H
#define _REGS_H

 #include <types.h>

///
/// Defines the way the registers are stored on  the stack
/// during a system call.
///
struct regs {
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
    uint32_t xds;
    uint32_t xes;
    uint32_t xfs;

    uint32_t orig_eax;
    uint32_t eip;
    uint32_t xcs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t xss;
};

#endif /* _REGS_H */
