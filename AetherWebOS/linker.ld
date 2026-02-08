SECTIONS
{
    /* The Pi 4 loads the 64-bit kernel at this specific address */
    . = 0x80000;     

    .text : {
        KEEP(*(.text.boot)) /* Ensure our boot assembly is at the very front */
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    
    /* Calculate BSS size for our assembly code to use */
    __bss_size = (__bss_end - __bss_start) >> 3;

    . = ALIGN(16);
    . += 0x1000; /* 4KB of stack space */
    stack_top = .;
}