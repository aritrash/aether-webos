#ifndef CONFIG_H
#define CONFIG_H

// Default to VIRT if not specified (for QEMU testing)
#ifndef BOARD_VIRT
    #ifndef BOARD_RPI4
        #define BOARD_VIRT
    #endif
#endif

#ifdef BOARD_VIRT
    /* QEMU 'virt' Machine Map (with highmem=off) */
    #define UART0_BASE        0x09000000
    #define GIC_DIST_BASE     0x08000000
    #define GIC_REDIST_BASE   0x080A0000 // GICv3 Redistributor base
    
    // Physical address for ECAM in 32-bit space
    #define PCIE_PHYS_ECAM    0x3f000000LL 
    // Virtual address for the kernel to access ECAM
    #define PCIE_EXT_CFG_DATA 0x80000000LL 
    
    #define RAM_START         0x40000000
    #define PERIPHERAL_START  0x08000000
    #define PERIPHERAL_END    0x3FFFFFFF // Expanded to cover PCIe space
#else
    /* Raspberry Pi 4 (BCM2711) Map */
    #define UART0_BASE        0xFE201000
    #define GIC_DIST_BASE     0xFF841000
    #define PCIE_REG_BASE     0xFD500000
    
    // RPi4 usually maps PCIe Config via a smaller window or specialized bridge
    #define PCIE_PHYS_ECAM    0x600000000LL 
    #define PCIE_EXT_CFG_DATA 0x600000000LL 
    
    #define RAM_START         0x00000000
    #define PERIPHERAL_START  0xFD000000
    #define PERIPHERAL_END    0xFFFFFFFF
#endif

#endif