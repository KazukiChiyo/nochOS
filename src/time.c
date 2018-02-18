/* time.c - Functions to handle clock tick.
   author: Kexuan Zou
   date: 11/19/2017
*/

#include <time.h>
#include <pit.h>
#include <lib.h>

volatile time_t system_time;

/* count_start
   description: call to start a timer
   input: time_var - time variable not managed by user
   output: none
   return value: none
   side effect: none
*/
void count_start(uint32_t* time_var) {
    *time_var = system_time.count_ms;
}


/* count_end
   description: call when timer finishes count
   input: time_var - same variable as declared and passed in to count_start
   output: none
   return value: time elapsed
   side effect: none
*/
uint32_t count_end(uint32_t* time_var) {
    uint32_t cur_timestamp = system_time.count_ms;
    return cur_timestamp - *time_var;
}


/* sleep
   description: delay for specifed amount of time (spins cpu)
   input: delay - amount of time to delay
   output: none
   return value: none
   side effect: actively spins cpu
*/
void sleep(int ms) {
    int orig_ms = system_time.count_ms;
    while (system_time.count_ms - orig_ms < ms + 1)
        ;
}


/* system_time_init
   description: sets global system time, initializes pit
   input: none
   output: none
   return value: none
   side effect: none
*/
void system_time_init(void) {
    system_time.count_ms = 0;
    //TODO: get current time from server
    pit_init();
}
