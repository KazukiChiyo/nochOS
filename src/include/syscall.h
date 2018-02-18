/// syscall.h: Interface for system calls.
/// - author: Zhengcheng Huang, Kexuan Zou

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <types.h>
#include <syscall_num.h>
#include <proc.h>

#define MAX_PID 5

typedef struct proc_info {
    int8_t cmd[ARG_WORD_SIZE];
    uint32_t status;
    uint32_t uptime;
} __attribute__((packed)) proc_info;

typedef struct system_info {
    uint32_t num_active;
    uint32_t num_idle;
    uint32_t uptime;
} __attribute__((packed)) system_info;



extern int32_t syscall_handler(void);

extern int32_t sys_halt(int32_t status);
extern int32_t sys_execute(const int8_t * command);
extern int32_t sys_getargs(int8_t* buf, int32_t nbytes);
extern int32_t sys_vidmap(uint8_t** screen_start);
extern int32_t sys_set_handler(int32_t signum, void* handler_address);
extern int32_t sys_sigreturn(void);
extern int32_t sys_read(uint32_t fd, void * buf, uint32_t nbytes);
extern int32_t sys_write(uint32_t fd, const void * buf, uint32_t nbytes);
extern int32_t sys_open(const int8_t * filename);
extern int32_t sys_close(uint32_t fd);
extern int32_t sys_kill(uint32_t pid, int32_t ignore);
extern int32_t sys_query(uint32_t pid, proc_info* obj);
extern int32_t sys_info(system_info* obj);
extern int32_t sys_create(const int8_t * fname);
extern int32_t sys_rm(const int8_t * fname);
extern int32_t sys_mkdir(const int8_t * dir_name);
extern int32_t sys_cd(const int8_t * dir_name);
extern int32_t sys_seek(uint32_t fd, int32_t offset, int32_t whence);
extern int32_t sys_encrypt(const int8_t * fname);
extern int32_t sys_decrypt(const int8_t * fname);
extern int32_t sys_filemode(uint32_t fd);
extern int32_t sys_pwd(int8_t * path);
extern int32_t sys_net_package(void * buf, uint32_t nbytes);
extern int32_t sys_shutdown(void);
extern int32_t sys_setusr(const uint8_t* usr);
extern int32_t sys_getusr(uint8_t* buf);
extern int32_t sys_getpid(void);
extern int32_t sys_textcolor(uint8_t attr);
extern int32_t sys_map_modex(uint8_t** screen_start);
extern int32_t sys_ipconfig(void *buf);
extern int32_t sys_getip(void);

#endif /* _SYSCALL_H */
