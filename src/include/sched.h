/* author: Kexuan Zou
   date: 11/14/2017
*/

#ifndef _SCHED_H
#define _SCHED_H
#include <types.h>

#define TIME_QUANTUM 30 // time quantum is 20 ms

uint32_t load_shell_img(uint32_t cur_pid);
extern void do_sched(void);

#endif
