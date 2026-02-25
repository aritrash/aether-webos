#include "utils.h"
#include "timer.h"

static uint64_t _timer_freq;
static volatile uint64_t _uptime_ms = 0;

static uint64_t get_timer_freq() {
    uint64_t val;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

void timer_init() {
    _timer_freq = get_timer_freq();
    
    // Set for 10ms (Frequency / 100)
    uint64_t interval = _timer_freq / 100;
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (interval));

    // Enable=1, Unmask=0
    asm volatile ("msr cntp_ctl_el0, %0" : : "r" (1));
}

void handle_timer_irq() {
    // 1. Increment global uptime
    _uptime_ms += 10; 

    // 2. RELOAD: If you miss this, the timer never fires again!
    // Set for another 10ms (assuming 100Hz)
    uint64_t frq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r" (frq));
    asm volatile ("msr cntp_tval_el0, %0" : : "r" (frq / 100)); 
}

uint64_t get_system_uptime_ms() {
    return _uptime_ms;
}