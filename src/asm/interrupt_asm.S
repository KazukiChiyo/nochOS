# interrupt_asm.S: Implementation of irq linkage.
# - author: Zhengcheng Huang

# Macro used for neatly defining interrupt handlers.
#define DO_INT(name,irq)        \
name:   pushl   $irq            ;\
    jmp     common_interrupt    ;\

# Declared in interrupt.h
# A jump table of interrupt handlers.
.global int_table
int_table:
    .long int_irq0
    .long int_irq1
    .long int_irq2
    .long int_irq3
    .long int_irq4
    .long int_irq5
    .long int_irq6
    .long int_irq7
    .long int_irq8
    .long int_irq9
    .long int_irq10
    .long int_irq11
    .long int_irq12
    .long int_irq13
    .long int_irq14
    .long int_irq15

# All interrupt handlers
DO_INT(int_irq0,0xFFFFFFFF)
DO_INT(int_irq1,0xFFFFFFFE)
DO_INT(int_irq2,0xFFFFFFFD)
DO_INT(int_irq3,0xFFFFFFFC)
DO_INT(int_irq4,0xFFFFFFFB)
DO_INT(int_irq5,0xFFFFFFFA)
DO_INT(int_irq6,0xFFFFFFF9)
DO_INT(int_irq7,0xFFFFFFF8)
DO_INT(int_irq8,0xFFFFFFF7)
DO_INT(int_irq9,0xFFFFFFF6)
DO_INT(int_irq10,0xFFFFFFF5)
DO_INT(int_irq11,0xFFFFFFF4)
DO_INT(int_irq12,0xFFFFFFF3)
DO_INT(int_irq13,0xFFFFFFF2)
DO_INT(int_irq14,0xFFFFFFF1)
DO_INT(int_irq15,0xFFFFFFF0)

###
### Saves registers and calls interrupt handler.
###
### - author: Zhengcheng Huang
###
common_interrupt:
    # Save all registers
    pushl   $0
    movw    %fs,2(%esp)
    pushl   $0
    movw    %es,2(%esp)
    pushl   $0
    movw    %ds,2(%esp)
    pushl   %eax
    pushl   %ebp
    pushl   %edi
    pushl   %esi
    pushl   %edx
    pushl   %ecx
    pushl   %ebx

    # Assembly linkage. Enables `do_irq` to access runtime stack.
    movl    %esp,%ecx

    # Handles the specific interrupt.
    call    do_irq

    # Return from interrupt
    jmp     ret_from_int

###
### Restores registers and return from interrupt.
###
### - author: Zhengcheng Huang
###
ret_from_int:
    # Restores all registers.
    # do_signal(regs)
    movl %esp, %ecx
    call do_signal
ret_from_sig:
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    popl    %eax
    movw    2(%esp),%ds
    addl    $4,%esp
    movw    2(%esp),%es
    addl    $4,%esp
    movw    2(%esp),%fs
    addl    $4,%esp

    # Tear down stack.
    addl    $4,%esp

    # Return from interrupt.
    iret

.global ret_from_int
.global ret_from_sig
