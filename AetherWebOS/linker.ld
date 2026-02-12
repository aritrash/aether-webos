SECTIONS
{
    /* * We use a symbol for the start address so we can toggle between:
     * RPi4: 0x80000
     * VIRT: 0x40080000 (standard load point for QEMU virt)
     */
    . = KERNEL_START;     

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    . = ALIGN(2048);
    .vectors : {
        __vectors_start = .;
        KEEP(*(.vectors))
        . = ALIGN(2048); 
        __vectors_end = .;
    }

    .rodata : { 
        . = ALIGN(8);
        *(.rodata .rodata.*) 
    }
    
    . = ALIGN(16);
    .data : { *(.data) }

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    
    __bss_size = (__bss_end - __bss_start) >> 3;

    . = ALIGN(16);
    . += 0x1000; 
    stack_top = .;

    /* Align the Page Tables to 4KB for the MMU */
    . = ALIGN(4096);
    __pgtbl_start = .;
    .pgtbl : {
        *(.pgtbl)
    }
    __pgtbl_end = .;

    _end = .;
}