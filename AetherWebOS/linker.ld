SECTIONS
{
    /* The Pi 4 loads the 64-bit kernel at this specific address */
    . = 0x80000;     

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

    /* Stack Setup: 4KB space. */
    . = ALIGN(16);
    . += 0x1000; 
    stack_top = .;

    _end = .;
}