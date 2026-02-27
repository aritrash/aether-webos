ENTRY(_start)

SECTIONS
{
    . = 0x40080000;

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    . = ALIGN(2048);
    .vectors : {
        __vectors_start = .;
        KEEP(*(.vectors))
        __vectors_end = .;
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

   .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
}

    . = ALIGN(16);
    . += 0x4000;
    _stack_top = .;
}