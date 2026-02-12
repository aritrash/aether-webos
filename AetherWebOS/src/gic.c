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
    // GICv3: Enable Affinity Routing (ARE) for Secure and Non-Secure
    GICD_CTLR = (1 << 4) | (1 << 5); 
#endif

    // 2. Enable Timer IRQ (ID 30)
    GICD_ISENABLER[0] = (1 << TIMER_IRQ_ID);

#ifdef BOARD_RPI4
    /* --- GICv2 (Raspberry Pi 4) --- */
    GICD_ITARGETSR[7] |= (1 << 16); 
    GICC_PMR = 0xFF; 
    GICC_CTLR = 1;   
#else
    /* --- GICv3 (QEMU VIRT) --- */

    // 3. Find and Wake the Redistributor for the current CPU
    // Standard QEMU Virt: GICD=0x08000000, GICR=0x080A0000
    // We probe starting at 0x080A0000 (standard for virt)
    uint64_t redist_base = GIC_DIST_BASE + 0xA0000; 
    uint64_t mpidr;
    asm volatile("mrs %0, mpidr_el1" : "=r" (mpidr));
    uint32_t aff = (mpidr & 0xFFFFFF); // Aff2.Aff1.Aff0

    while (1) {
        // GICR_TYPER is at offset 0x08. It tells us which CPU this redist belongs to.
        uint64_t typer = *(volatile uint64_t*)(redist_base + 0x08);
        if ((typer >> 32) == aff) {
            break; // Found our redistributor
        }
        if (typer & (1ULL << 4)) {
            // Last redistributor in the list and no match? Fallback to base.
            uart_puts("[WARN] GIC: Redistributor affinity mismatch. Using base.\r\n");
            break;
        }
        redist_base += 0x20000; // Move to next redistributor (128KB stride)
    }

    // GICR_WAKER is at offset 0x14 of the Redistributor
    volatile uint32_t* GICR_WAKER = (volatile uint32_t*)(redist_base + 0x14);
    
    uart_puts("[DEBUG] GIC: Waking Redistributor at ");
    uart_put_hex((uint64_t)GICR_WAKER);
    uart_puts("\r\n");

    // Clear ProcessorSleep bit (Bit 1)
    *GICR_WAKER &= ~(1 << 1);
    
    // Wait until ChildrenAsleep (Bit 2) is cleared by hardware
    while (*GICR_WAKER & (1 << 2));

    // 4. Move Timer IRQ to Group 1 (Non-secure)
    GICD_IGROUPR[0] |= (1 << TIMER_IRQ_ID);
    
    // 5. Set Trigger Type to Edge-Triggered
    GICD_ICFGR[1] |= (2 << ((TIMER_IRQ_ID % 16) * 2));

    // 6. Set Interrupt Priority
    GICD_IPRIORITYR[TIMER_IRQ_ID] = 0xA0;

    // 7. CPU Interface (System Registers)
    uint64_t sRE;
    asm volatile("mrs %0, ICC_SRE_EL1" : "=r" (sRE));
    asm volatile("msr ICC_SRE_EL1, %0" : : "r" (sRE | 0x7)); // Enable SRE
    
    asm volatile("msr ICC_PMR_EL1, %0" : : "r" (0xFF));      // Priority Mask
    asm volatile("msr ICC_IGRPEN1_EL1, %0" : : "r" (1));     // Enable Group 1
#endif

    uart_puts("[OK] GIC: Ready to receive interrupts.\r\n");
}