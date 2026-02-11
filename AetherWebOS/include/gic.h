#ifndef GIC_H
#define GIC_H

#include <stdint.h>

// BCM2711 GIC-400 Base Addresses
#define GIC_DIST_BASE  0xFF841000
#define GIC_CPU_BASE   0xFF842000

// Distributor Registers
#define GICD_CTLR       (*(volatile uint32_t *)(GIC_DIST_BASE + 0x000))
#define GICD_ISENABLER  ((volatile uint32_t *)(GIC_DIST_BASE + 0x100))
#define GICD_ITARGETSR  ((volatile uint32_t *)(GIC_DIST_BASE + 0x800))

// CPU Interface Registers
#define GICC_CTLR       (*(volatile uint32_t *)(GIC_CPU_BASE + 0x000))
#define GICC_PMR        (*(volatile uint32_t *)(GIC_CPU_BASE + 0x004))
#define GICC_IAR        (*(volatile uint32_t *)(GIC_CPU_BASE + 0x00C))
#define GICC_EOIR       (*(volatile uint32_t *)(GIC_CPU_BASE + 0x010))

// The ARM Physical Timer IRQ ID is usually 30
#define TIMER_IRQ_ID    30

void gic_init();

#endif