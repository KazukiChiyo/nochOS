/* author: Kexuan Zou
   date: 11/19/2017
*/
#ifndef _TIME_H
#define _TIME_H
#include <types.h>

#define TIME_MS 1
#define TIME_SEC 1000
#define TIME_MIN (TIME_SEC * 60)
#define TIME_HR (TIME_MIN * 60)

/* time struct for PIT */
typedef struct time_t {
    uint32_t count_ms; // time count in milisecond
} time_t;

extern volatile time_t system_time; // an instance of the time struct

void count_start(uint32_t* time_var);
uint32_t count_end(uint32_t* time_var);
extern void sleep(int ms);
extern void system_time_init(void);

#endif
