/// syscall_num.h: Syscall numbers

#ifndef _SYSCALL_NUM_H
#define _SYSCALL_NUM_H

#define SYS_HALT        1
#define SYS_EXECUTE     2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_GETARGS     7
#define SYS_VIDMAP      8
#define SYS_SET_HANDLER 9
#define SYS_SIGRETURN   10
#define SYS_KILL        11
#define SYS_QUERY       12
#define SYS_INFO        13
#define SYS_CREATE      14
#define SYS_RM          15
#define SYS_MKDIR       16
#define SYS_CD          17
#define SYS_SEEK        18
#define SYS_ENCRYPT     19
#define SYS_DECRYPT     20
#define SYS_FILEMODE    21
#define SYS_PWD         22
#define SYS_NET_PACKAGE 23
#define SYS_SHUTDOWN    24
#define SYS_SETUSR      25
#define SYS_GETUSR      26
#define SYS_GETPID      27
#define SYS_TEXTCOLOR   28
#define SYS_MAP_MODEX   29
#define SYS_IPCONFIG    30
#define SYS_GETIP		    31

#define SYS_MAX         31

#endif /* _SYSCALL_NUM_H */
