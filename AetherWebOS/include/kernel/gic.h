#ifndef GIC_H
#define GIC_H

#include <stdint.h>
#include "config.h"

/* * GIC Distributor (GICD) and CPU Interface (GICC) offsets
 * flow from config.h based on the BOARD flag.
 */

// --- Distributor Registers ---
#define GICD_CTLR       (*(volatile uint32_t *)(GIC_DIST_BASE + 0x000))
#define GICD_ISENABLER  ((volatile uint32_t *)(GIC_DIST_BASE + 0x100))
#define GICD_IGROUPR    ((volatile uint32_t *)(GIC_DIST_BASE + 0x080))
#define GICD_ICFGR      ((volatile uint32_t *)(GIC_DIST_BASE + 0xC00))
#define GICD_IPRIORITYR ((volatile uint8_t *)(GIC_DIST_BASE + 0x400))

#ifdef BOARD_RPI4
    // GICv2 Specific (Memory Mapped Interface for Raspberry Pi 4)
    #define GICD_ITARGETSR  ((volatile uint32_t *)(GIC_DIST_BASE + 0x800))
    #define GICC_CTLR       (*(volatile uint32_t *)(GIC_CPU_BASE + 0x000))
    #define GICC_PMR        (*(volatile uint32_t *)(GIC_CPU_BASE + 0x004))
    #define GICC_IAR        (*(volatile uint32_t *)(GIC_CPU_BASE + 0x00C))
    #define GICC_EOIR       (*(volatile uint32_t *)(GIC_CPU_BASE + 0x010))
#endif

// Generic Timer Interrupt ID on AArch64
#define TIMER_IRQ_ID    30

void gic_init();

#endif