SECTIONS
{
    /* The Pi 4 loads the 64-bit kernel at this specific address */
    . = 0x80000;     

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    . = ALIGN(2048);
<<<<<<< HEAD
    /* Explicitly tell the linker this section is for the table */
    .vectors : {
        __vectors_start = .;
        KEEP(*(.vectors))
        . = ALIGN(2048); /* Ensure the next section doesn't bleed in */
=======
    .vectors : {
        __vectors_start = .;
        KEEP(*(.vectors))
        . = ALIGN(2048); 
>>>>>>> b4434c3 (Update: Enable MMU)
        __vectors_end = .;
    }

    .rodata : { 
        . = ALIGN(8);
        *(.rodata .rodata.*) 
    }
    
    . = ALIGN(16);
    .data : { *(.data) }

<<<<<<< HEAD
=======
    /* * Page Tables Section
     * Must be 4KB aligned for the MMU hardware to accept them.
     */
    . = ALIGN(4096);
    .pgtbl (NOLOAD) : {
        __pgtbl_start = .;
        *(.pgtbl)
        . = ALIGN(4096);
        __pgtbl_end = .;
    }

>>>>>>> b4434c3 (Update: Enable MMU)
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    
<<<<<<< HEAD
    /* * Calculate BSS size in 8-byte double-words for the boot.S loop.
     * We use (end - start) / 8.
     */
    __bss_size = (__bss_end - __bss_start) >> 3;

    /* * Stack Setup: 4KB space.
     * Must be 16-byte aligned for AArch64 stack pointer usage.
     */
=======
    __bss_size = (__bss_end - __bss_start) >> 3;

    /* Stack Setup: 4KB space. */
>>>>>>> b4434c3 (Update: Enable MMU)
    . = ALIGN(16);
    . += 0x1000; 
    stack_top = .;

<<<<<<< HEAD
    /* End of binary marker for debugging */
=======
>>>>>>> b4434c3 (Update: Enable MMU)
    _end = .;
}