
#include <types.h>
#include <lib.h>
#include <i8259.h>
#include <keyboard.h>
#include <rtc.h>
#include <network.h>
#include <pit.h>
#include <sched.h>
#include <proc.h>
#include <debug.h>
#include <sched.h>
#include <mouse.h>
#include <interrupt.h>
#include <panic.h>
#include <system.h>

pcb_t * cur_proc_pcb; // declared in proc.h
#include <network.h>
///
/// Handles an interrupt for specific irq number.
///s
/// - author: Zhengcheng Huang
/// - arguments
///     regs: Register information stored on top of the stack.
/// - side effects: Call the actual handler, which may have any side effect.
///
__attribute__((fastcall))
void do_irq(struct regs *regs) {
    int irq = ~(regs->orig_eax);
    switch (irq) {

        // Case where interrupt is from keyboard.
        case KEYBOARD_IRQ_PIN:
            keyboard_handler();
            send_eoi(KEYBOARD_IRQ_PIN);
            return;

        // Case where interrupt is from rtc.
        case RTC_IRQ_PIN:
            rtc_handler();
            send_eoi(RTC_IRQ_PIN);
            return;

        case NET_WORK_PIN:
        	fire();
        	send_eoi(NET_WORK_PIN);
        	return;

        // Case where interrupt is from pit.
        case PIT_IRQ_PIN:
        #ifndef RUN_TESTS
            if (cur_proc_pcb != NULL)
                cur_proc_pcb->kernel_esp = (uint32_t)regs;
        #endif
            pit_handler();
            send_eoi(PIT_IRQ_PIN);
            return;

        // Case where interrupt is from mouse.
        case MOUSE_IRQ_PIN:
            mouse_handler();
            send_eoi(MOUSE_IRQ_PIN);
            return;
        default:
            break;
    }
}
