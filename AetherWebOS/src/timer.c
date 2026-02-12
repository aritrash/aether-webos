#include "utils.h"
#include "timer.h"

static uint64_t get_timer_freq() {
    uint64_t val;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

void timer_init() {
    uint64_t freq = get_timer_freq();
    
    // Set timer to fire in 0.5 seconds
    // It's often safer to use tval (Timer Value) for relative offsets
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (freq / 2));

    /*
     * Enable the timer (Bit 0) and UNMASK the interrupt (Bit 1 = 0)
     * IMASK is Bit 1. If IMASK=1, the interrupt is masked.
     * Setting to 1 means: Enable=1, IMASK=0, ISTRIATUS=0
     */
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (1));
}