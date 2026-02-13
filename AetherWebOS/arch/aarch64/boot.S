.section ".text.boot"
.global _start

_start:
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, master
    
hang: 
    wfe
    b       hang

master:
    /* Check Current EL */
    mrs     x0, CurrentEL
    and     x0, x0, #0b1100
    lsr     x0, x0, #2
    cmp     x0, #2
    b.ne    setup_stack        /* If not EL2, skip the drop */

    /* --- Transition from EL2 to EL1 --- */
    /* 1. Enable EL1 access to the timer */
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr

    /* 2. Set EL1 to AArch64 (Execution state) */
    mov     x0, #(1 << 31)     /* RW=1 (EL1 is AArch64) */
    orr     x0, x0, #(1 << 1)  /* SWIO hardwired on Pi */
    msr     hcr_el2, x0

    /* 3. Prepare SPSR_EL2 (Saved Program Status Register) */
    /* Mask all interrupts and set mode to EL1h (using SP_EL1) */
    mov     x0, #0x3c5         /* 0b1111000101: D,A,I,F masked, EL1h */
    msr     spsr_el2, x0

    /* 4. Set the return address to our stack setup label */
    adr     x0, setup_stack
    msr     elr_el2, x0

    /* 5. Drop the bomb */
    eret

setup_stack:
    ldr     x0, =stack_top
    bic     x0, x0, #0xF
    mov     sp, x0
    isb

    /* Clear BSS */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_size
setup_bss:
    cbz     x1, jump_to_main
    str     xzr, [x0], #8
    subs    x1, x1, #1
    bne     setup_bss

jump_to_main:
    bl      kernel_main
    b       hang