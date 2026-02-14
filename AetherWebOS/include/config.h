#ifndef CONFIG_H
#define CONFIG_H

#ifndef BOARD_VIRT
    #ifndef BOARD_RPI4
        #define BOARD_VIRT
    #endif
#endif

#ifdef BOARD_VIRT
    /* QEMU 'virt' Machine Map */
    #define UART0_BASE        0x09000000
    #define GIC_DIST_BASE     0x08000000
    #define GIC_REDIST_BASE   0x080A0000 
    
    // Physical address for ECAM
    #define PCIE_PHYS_ECAM    0x3f000000LL 

    /* * FIX 1: Move ECAM Virtual Address.
     * Let's put the Static ECAM at 0x70000000.
     * This leaves 0x80000000 entirely free for Adrija's ioremap (vmalloc).
     */
    #define PCIE_EXT_CFG_DATA 0x70000000LL 
    
    #define RAM_START         0x40000000
    
    /* * FIX 2: Broaden Peripheral Start.
     * Your Heap is at 0x01000000. Start at 0x00000000 to ensure 
     * the MMU maps the low-memory heap area.
     */
    #define PERIPHERAL_START  0x00000000
    #define PERIPHERAL_END    0x3FFFFFFF 
#else
    /* Raspberry Pi 4 (BCM2711) Map */
    // (Keep as is, but ensure similar logic if testing on hardware)
    #define UART0_BASE        0xFE201000
    #define GIC_DIST_BASE     0xFF841000
    #define PCIE_REG_BASE     0xFD500000
    #define PCIE_PHYS_ECAM    0x600000000LL 
    #define PCIE_EXT_CFG_DATA 0x600000000LL 
    #define RAM_START         0x00000000
    #define PERIPHERAL_START  0xFD000000
    #define PERIPHERAL_END    0xFFFFFFFF
#endif

#endif