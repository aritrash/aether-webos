#include "utils.h"
#include "timer.h"

// Get the clock frequency (ticks per second)
static uint64_t get_timer_freq() {
    uint64_t val;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

void timer_init() {
    uint64_t freq = get_timer_freq();
    uint64_t ticks = freq / 2; // Trigger every 0.5 seconds

    // Set the compare value: Current Time + Ticks
    asm volatile ("mrs x0, cntpct_el0 \n\t"
                  "add x0, x0, %0    \n\t"
                  "msr cntp_cval_el0, x0" : : "r" (ticks));

    // Enable the timer
    asm volatile ("mov x0, #1 \n\t"
                  "msr cntp_ctl_el0, x0");
}