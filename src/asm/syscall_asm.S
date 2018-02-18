# syscall_asm.S: Implements syscall handler
# - author: Zhengcheng Huang

#include <syscall_num.h>

#define DO_SYS(name,sysno)  \
name:                       \
    pushl   $sysno;         \
    jmp common_syscall

.global syscall_handler
syscall_handler:
    # Check syscall number validity.
    cmpl    $SYS_MAX,%eax
    ja      syscall_invalid_eax
    cmpl    $0,%eax
    je      syscall_invalid_eax

    jmp     *jumptable(,%eax,4)

DO_SYS(sys_halt_handler,SYS_HALT)
DO_SYS(sys_execute_handler,SYS_EXECUTE)
DO_SYS(sys_read_handler,SYS_READ)
DO_SYS(sys_write_handler,SYS_WRITE)
DO_SYS(sys_open_handler,SYS_OPEN)
DO_SYS(sys_close_handler,SYS_CLOSE)
DO_SYS(sys_getargs_handler,SYS_GETARGS)
DO_SYS(sys_vidmap_handler,SYS_VIDMAP)
DO_SYS(sys_set_handler_handler,SYS_SET_HANDLER)
DO_SYS(sys_sigreturn_handler,SYS_SIGRETURN)
DO_SYS(sys_kill_handler,SYS_KILL)
DO_SYS(sys_query_handler,SYS_QUERY)
DO_SYS(sys_info_handler,SYS_INFO)
DO_SYS(sys_create_handler,SYS_CREATE)
DO_SYS(sys_rm_handler,SYS_RM)
DO_SYS(sys_mkdir_handler,SYS_MKDIR)
DO_SYS(sys_cd_handler,SYS_CD)
DO_SYS(sys_seek_handler,SYS_SEEK)
DO_SYS(sys_encrypt_handler,SYS_ENCRYPT)
DO_SYS(sys_decrypt_handler,SYS_DECRYPT)
DO_SYS(sys_filemode_handler,SYS_FILEMODE)
DO_SYS(sys_pwd_handler,SYS_PWD)
DO_SYS(sys_net_package_handler,SYS_NET_PACKAGE)
DO_SYS(sys_shutdown_handler, SYS_SHUTDOWN)
DO_SYS(sys_setusr_handler, SYS_SETUSR)
DO_SYS(sys_getusr_handler, SYS_GETUSR)
DO_SYS(sys_getpid_handler, SYS_GETPID)
DO_SYS(sys_textcolor_handler, SYS_TEXTCOLOR)
DO_SYS(sys_map_modex_handler, SYS_MAP_MODEX)
DO_SYS(sys_ipconfig_handler,SYS_IPCONFIG)
DO_SYS(sys_getip_handler,SYS_GETIP)

syscall_invalid_eax:
    xorl    %eax,%eax
    decl    %eax
    iret

common_syscall:
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

    call    do_sys

    jmp     ret_from_int

jumptable:
    .long 0
    .long sys_halt_handler
    .long sys_execute_handler
    .long sys_read_handler
    .long sys_write_handler
    .long sys_open_handler
    .long sys_close_handler
    .long sys_getargs_handler
    .long sys_vidmap_handler
    .long sys_set_handler_handler
    .long sys_sigreturn_handler
    .long sys_kill_handler
    .long sys_query_handler
    .long sys_info_handler
    .long sys_create_handler
    .long sys_rm_handler
    .long sys_mkdir_handler
    .long sys_cd_handler
    .long sys_seek_handler
    .long sys_encrypt_handler
    .long sys_decrypt_handler
    .long sys_filemode_handler
    .long sys_pwd_handler
    .long sys_net_package_handler
    .long sys_shutdown_handler
    .long sys_setusr_handler
    .long sys_getusr_handler
    .long sys_getpid_handler
    .long sys_textcolor_handler
    .long sys_map_modex_handler
    .long sys_ipconfig_handler
    .long sys_getip_handler
