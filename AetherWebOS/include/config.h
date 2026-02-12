#ifndef CONFIG_H
#define CONFIG_H

// Default to VIRT if not specified (for QEMU testing)
#ifndef BOARD_VIRT
#ifndef BOARD_RPI4
#define BOARD_VIRT
#endif
#endif

#ifdef BOARD_VIRT
    /* QEMU 'virt' Machine Map */
    #define UART0_BASE        0x09000000
    #define GIC_DIST_BASE     0x08000000
    #define GIC_CPUI_BASE     0x08010000
    #define PCIE_REG_BASE     0x3EF00000 // Root Complex Registers
    #define PCIE_EXT_CFG_DATA 0x80000000 
    #define PCIE_PHYS_ECAM    0x10000000// ECAM space
    #define RAM_START         0x40000000
    #define PERIPHERAL_START  0x08000000
    #define PERIPHERAL_END    0x0BFFFFFF
#else
    /* Raspberry Pi 4 (BCM2711) Map */
    #define UART0_BASE        0xFE201000
    #define GIC_DIST_BASE     0xFF841000
    #define PCIE_REG_BASE     0xFD500000
    #define PCIE_EXT_CFG_DATA 0x600000000
    #define RAM_START         0x00000000
    #define PERIPHERAL_START  0xFD000000
    #define PERIPHERAL_END    0xFFFFFFFF
#endif

#endif