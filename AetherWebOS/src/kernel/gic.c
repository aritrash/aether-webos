#include "gic.h"
#include "config.h"
#include "uart.h"
#include "utils.h"

void gic_init() {
    uart_puts("[INFO] GIC: Initializing Interrupt Controller...\r\n");

    // 1. Enable the Distributor
#ifdef BOARD_RPI4
    GICD_CTLR = 1;
#else
    // GICv3: Enable Affinity Routing (ARE) and both Groups
    GICD_CTLR = (1 << 4) | (1 << 5) | (1 << 0) | (1 << 1); 
#endif

#ifdef BOARD_RPI4
    /* --- GICv2 (Raspberry Pi 4) --- */
    GICD_ISENABLER[0] = (1 << TIMER_IRQ_ID);
    GICD_ITARGETSR[7] |= (1 << 16); 
    GICC_PMR = 0xFF; 
    GICC_CTLR = 1;   
#else
    /* --- GICv3 (QEMU VIRT) --- */

    // 2. Find and Wake the Redistributor for the current Core
    uint64_t redist_base = GIC_DIST_BASE + 0xA0000; 
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r" (mpidr));
    uint32_t aff = (mpidr & 0xFFFFFF); 

    while (1) {
        uint64_t typer = *(volatile uint64_t*)(redist_base + 0x08);
        if ((typer >> 32) == (aff)) break; // Found our core
        if (typer & (1ULL << 4)) break;    // Last redistributor in list
        redist_base += 0x20000; 
    }

    // GICR_WAKER: Clear ProcessorSleep to allow interrupts to flow
    volatile uint32_t* GICR_WAKER = (volatile uint32_t*)(redist_base + 0x14);
    *GICR_WAKER &= ~(1 << 1); 
    while (*GICR_WAKER & (1 << 2)); // Wait for children to wake

    /* --- PPI/SGI Configuration (SGI_base) --- */
    uint64_t sgi_base = redist_base + 0x10000;

    // 3. Set Group 1 Non-Secure (CRITICAL FIX)
    // GICR_IGROUPR0 (Offset 0x80): Bit must be 1 (Group 1)
    *(volatile uint32_t*)(sgi_base + 0x080) |= (1 << TIMER_IRQ_ID);
    
    // GICR_IGRPMODR0 (Offset 0x88): Bit must be 0 (Non-Secure)
    // Your previous code was setting this to 1, which made it Secure-only.
    *(volatile uint32_t*)(sgi_base + 0x088) &= ~(1 << TIMER_IRQ_ID); 

    // 4. Set Priority (GICR_IPRIORITYR at offset 0x400)
    // PPI priorities are handled in the Redistributor for GICv3
    *(volatile uint8_t*)(sgi_base + 0x400 + TIMER_IRQ_ID) = 0xA0;

    // 5. Set Trigger (Edge-triggered for Timer)
    // GICR_ICFGR1 (Offset 0xC04) handles PPIs 16-31
    *(volatile uint32_t*)(sgi_base + 0xC04) |= (2 << ((TIMER_IRQ_ID % 16) * 2));

    // 6. Enable the Timer Interrupt
    *(volatile uint32_t*)(sgi_base + 0x100) = (1 << TIMER_IRQ_ID);

    /* --- CPU Interface (System Registers) --- */
    uint64_t sre;
    asm volatile("mrs %0, ICC_SRE_EL1" : "=r" (sre));
    asm volatile("msr ICC_SRE_EL1, %0" : : "r" (sre | 0x7)); // Enable System Reg Access
    
    asm volatile("msr ICC_PMR_EL1, %0" : : "r" (0xFF));      // Priority Mask: Allow all
    asm volatile("msr ICC_IGRPEN1_EL1, %0" : : "r" (1));     // Enable Group 1 IRQs
#endif

    uart_puts("[OK] GIC: Ready to receive interrupts.\r\n");
}