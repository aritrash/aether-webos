SECTIONS
{
    . = KERNEL_START;     

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    /* Page Tables must be 4KB aligned for the MMU Hardware Walker */
    . = ALIGN(4096);
    __pgtbl_start = .;
    .pgtbl : {
        *(.pgtbl)
    }
    __pgtbl_end = .;

    /* Exception Vectors MUST be 2048-byte aligned */
    . = ALIGN(2048);
    .vectors : {
        __vectors_start = .;
        KEEP(*(.vectors))
        __vectors_end = .;
    }

    .rodata : { 
        . = ALIGN(16);
        *(.rodata .rodata.*) 
    }
    
    .data : { 
        . = ALIGN(16);
        *(.data .data.*) 
    }

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    
    __bss_size = (__bss_end - __bss_start) >> 3;

    /* Stack setup */
    . = ALIGN(16);
    . += 0x4000; /* Increased to 16KB for safety during xHCI init */
    stack_top = .;

    _end = .;
}