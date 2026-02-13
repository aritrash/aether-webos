#ifndef PCIE_H
#define PCIE_H

#include <stdint.h>
#include "config.h" // Crucial: Pulls BOARD-specific addresses

/* * PCIE_REG_BASE and PCIE_EXT_CFG_DATA are now defined in config.h 
 * to support both RPi4 and QEMU Virt.
 */

// PCIe Register Offsets
#define PCIE_RC_REVISION     ((volatile uint32_t*)(PCIE_REG_BASE + 0x406c))
#define PCIE_STATUS          ((volatile uint32_t*)(PCIE_REG_BASE + 0x4068))
#define PCIE_INTR2_CPU_MASK  ((volatile uint32_t*)(PCIE_REG_BASE + 0x4310))

// Configuration Space (ECAM) 
// On RPi4, this remains 0x600000000. On Virt, this is 0x40000000.
// Defined in config.h
void pcie_init(void);
uint32_t pcie_read_config(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg);

#endif