#include "gic.h"

void gic_init() {
    // 1. Enable the Distributor
    GICD_CTLR = 1;

    // 2. Enable Timer IRQ (ID 30)
    // Each ISENABLER register handles 32 interrupts. 30 is in index 0.
    GICD_ISENABLER[0] = (1 << TIMER_IRQ_ID);

    // 3. Target Timer IRQ to CPU Core 0
    // Each ITARGETSR register handles 4 interrupts (8 bits each).
    // IRQ 30 is the 3rd byte of index 7 (30 / 4 = 7).
    GICD_ITARGETSR[7] |= (1 << 16); 

    // 4. Enable CPU Interface
    GICC_PMR = 0xFF; // Priority mask: allow all interrupts
    GICC_CTLR = 1;   // Enable
}