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
    // GICv3: Enable Affinity Routing (ARE)
    GICD_CTLR = (1 << 4) | (1 << 5); 
#endif

    // 2. Enable Timer IRQ (ID 30) in Distributor (Fallback)
    GICD_ISENABLER[0] = (1 << TIMER_IRQ_ID);

#ifdef BOARD_RPI4
    /* --- GICv2 (Raspberry Pi 4) --- */
    GICD_ITARGETSR[7] |= (1 << 16); 
    GICC_PMR = 0xFF; 
    GICC_CTLR = 1;   
#else
    /* --- GICv3 (QEMU VIRT) --- */

    // 3. Find and Wake the Redistributor
    uint64_t redist_base = GIC_DIST_BASE + 0xA0000; 
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r" (mpidr));
    uint32_t aff = (mpidr & 0xFFFFFF); 

    while (1) {
        uint64_t typer = *(volatile uint64_t*)(redist_base + 0x08);
        if ((typer >> 32) == aff) break;
        if (typer & (1ULL << 4)) break;
        redist_base += 0x20000; 
    }

    // GICR_WAKER is at offset 0x14
    volatile uint32_t* GICR_WAKER = (volatile uint32_t*)(redist_base + 0x14);
    *GICR_WAKER &= ~(1 << 1); // Clear ProcessorSleep
    while (*GICR_WAKER & (1 << 2));

    /* --- PPI Configuration in Redistributor --- */
    uint64_t sgi_base = redist_base + 0x10000;

    // GICR_IGROUPR0: Set to Group 1
    *(volatile uint32_t*)(sgi_base + 0x080) |= (1 << TIMER_IRQ_ID);
    
    // GICR_IGRPMODR0: Set to Non-Secure (CRITICAL FIX)
    // Offset 0x088 is the Modifier. Without this, it's a Secure interrupt!
    *(volatile uint32_t*)(sgi_base + 0x088) |= (1 << TIMER_IRQ_ID); 

    // Enable it
    *(volatile uint32_t*)(sgi_base + 0x100) = (1 << TIMER_IRQ_ID);
    

    // 4. Global configurations (Some implementations still require these)
    GICD_IGROUPR[0] |= (1 << TIMER_IRQ_ID);
    GICD_ICFGR[1] |= (2 << ((TIMER_IRQ_ID % 16) * 2));
    GICD_IPRIORITYR[TIMER_IRQ_ID] = 0xA0;

    // 5. CPU Interface (System Registers)
    uint64_t sRE;
    asm volatile("mrs %0, ICC_SRE_EL1" : "=r" (sRE));
    asm volatile("msr ICC_SRE_EL1, %0" : : "r" (sRE | 0x7)); 
    
    asm volatile("msr ICC_PMR_EL1, %0" : : "r" (0xFF));      
    asm volatile("msr ICC_IGRPEN1_EL1, %0" : : "r" (1));     
#endif

    uart_puts("[OK] GIC: Ready to receive interrupts.\r\n");
}