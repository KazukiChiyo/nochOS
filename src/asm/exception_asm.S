
.global exp_table

#define DO_EXP(name,expno)  \
name:                       \
    pushl   $expno;         \
    jmp     common_exception

#define DO_EXP_ERRNO(name,expno)    \
name:                               \
    addl    $4,%esp;                \
    pushl   $expno;                 \
    jmp     common_exception;

#define DO_EXP_MSG(name,msg)    \
name: .string msg

DO_EXP(int_de,0)
DO_EXP(int_db,1)
DO_EXP(int_nmi,2)
DO_EXP(int_bp,3)
DO_EXP(int_of,4)
DO_EXP(int_br,5)
DO_EXP(int_ud,6)
DO_EXP(int_nm,7)
DO_EXP_ERRNO(int_df,8)
DO_EXP(int_cso,9)
DO_EXP_ERRNO(int_ts,10)
DO_EXP_ERRNO(int_np,11)
DO_EXP_ERRNO(int_ss,12)
DO_EXP_ERRNO(int_gp,13)
DO_EXP_ERRNO(int_pf,14)
DO_EXP(int_rs,15)
DO_EXP(int_mf,16)
DO_EXP_ERRNO(int_ac,17)
DO_EXP(int_mc,18)
DO_EXP(int_xf,19)

DO_EXP_MSG(int_de_msg,"Divide Error")
DO_EXP_MSG(int_db_msg,"Debug Exception")
DO_EXP_MSG(int_nmi_msg,"Nonmaskable Interrupt")
DO_EXP_MSG(int_bp_msg,"Breakpoint Exception")
DO_EXP_MSG(int_of_msg,"Overflow Exception")
DO_EXP_MSG(int_br_msg,"BOUND Range Exceeded Exception")
DO_EXP_MSG(int_ud_msg,"Invalid Opcode Exception")
DO_EXP_MSG(int_nm_msg,"Device Not Available Exception")
DO_EXP_MSG(int_df_msg,"Double Fault Exception")
DO_EXP_MSG(int_cso_msg,"Coprocessor Segment Overrun Exception")
DO_EXP_MSG(int_ts_msg,"Invalid TSS Exception")
DO_EXP_MSG(int_np_msg,"Segment Not Present Exception")
DO_EXP_MSG(int_ss_msg,"Stack Fault Exception")
DO_EXP_MSG(int_gp_msg,"General Protection Exception")
DO_EXP_MSG(int_pf_msg,"Page Fault Exception")
DO_EXP_MSG(int_rs_msg,"Reserved Exception")
DO_EXP_MSG(int_mf_msg,"x87 FPU Floating-Point Error")
DO_EXP_MSG(int_ac_msg,"Alignment-Check Exception")
DO_EXP_MSG(int_mc_msg,"Machine-Check Exception")
DO_EXP_MSG(int_xf_msg,"SIMD Floating-Point Exception")

# Declared in exception.h.
# Jump table of exception handlers.
    .align 32
exp_table:
    .long   int_de
    .long   int_db
    .long   int_nmi
    .long   int_bp
    .long   int_of
    .long   int_br
    .long   int_ud
    .long   int_nm
    .long   int_df
    .long   int_cso
    .long   int_ts
    .long   int_np
    .long   int_ss
    .long   int_gp
    .long   int_pf
    .long   int_rs
    .long   int_mf
    .long   int_ac
    .long   int_mc
    .long   int_xf

    .align 32
exp_msg_table:
    .long   int_de_msg
    .long   int_db_msg
    .long   int_nmi_msg
    .long   int_bp_msg
    .long   int_of_msg
    .long   int_br_msg
    .long   int_ud_msg
    .long   int_nm_msg
    .long   int_df_msg
    .long   int_cso_msg
    .long   int_ts_msg
    .long   int_np_msg
    .long   int_ss_msg
    .long   int_gp_msg
    .long   int_pf_msg
    .long   int_rs_msg
    .long   int_mf_msg
    .long   int_ac_msg
    .long   int_mc_msg
    .long   int_xf_msg

common_exception:
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
    ; movl    40(%esp),%edx
    ; movl    exp_msg_table(,%edx,4),%edx

    call    do_exp

    jmp     ret_from_int
