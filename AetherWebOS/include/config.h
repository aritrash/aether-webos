#ifndef CONFIG_H
#define CONFIG_H

#ifndef BOARD_VIRT
    #ifndef BOARD_RPI4
        #define BOARD_VIRT
    #endif
#endif

#ifdef BOARD_VIRT
    /* --- QEMU 'virt' Machine Map --- */
    #define UART0_BASE         0x09000000
    #define GIC_DIST_BASE      0x08000000
    
    /**
     * TIMER_IRQ_ID: Physical Timer (Non-Secure)
     * For QEMU 'virt', this is typically Interrupt ID 30.
     */
    #define TIMER_IRQ_ID       30

    /**
     * PCIE_PHYS_BASE: The actual hardware address in QEMU.
     * Note: 0x3f000000 is the standard for 'highmem=off'.
     */
    #define PCIE_PHYS_BASE     0x3f000000LL 

    /**
     * PCIE_EXT_CFG_DATA: The VIRTUAL address our kernel uses.
     * Virtual 0x70000000 -> Physical 0x3f000000
     */
    #define PCIE_EXT_CFG_DATA  0x70000000
    
    /* System RAM start for QEMU Virt */
    #define RAM_START          0x40000000
    
    /* Peripheral address range for identity mapping */
    #define PERIPHERAL_START   0x00000000
    #define PERIPHERAL_END     0x3FFFFFFF

    

#else
    /* --- Raspberry Pi 4 (BCM2711) Map --- */
    #define UART0_BASE         0xFE201000
    #define GIC_DIST_BASE      0xFF841000
    
    /**
     * TIMER_IRQ_ID: Physical Timer on RPi4. 
     * Usually maps to ID 97 in the GIC.
     */
    #define TIMER_IRQ_ID       97

    /* Pi4 PCIe Controller Registers */
    #define PCIE_REG_BASE      0xFD500000
    
    /* Pi4 ECAM is mapped at 0x6_0000_0000 (36-bit Physical) */
    #define PCIE_PHYS_BASE     0x600000000LL 
    #define PCIE_EXT_CFG_DATA  0x600000000LL 
    
    #define RAM_START          0x00000000
    #define PERIPHERAL_START   0xFD000000
    #define PERIPHERAL_END     0xFFFFFFFF
#endif

/* General Memory Layout Constants */
#define PAGE_SIZE              4096
#define HEAP_START             0x41000000 // Offset inside RAM for safety
#define HEAP_SIZE              (16 * 1024 * 1024)

#endif
