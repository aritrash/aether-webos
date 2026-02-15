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
    b.ne    setup_fpu         /* If already EL1, go to FPU setup */

    /* --- Transition from EL2 to EL1 --- */
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr

    mov     x0, #(1 << 31)     /* RW=1 (EL1 is AArch64) */
    orr     x0, x0, #(1 << 1)  
    msr     hcr_el2, x0

    mov     x0, #0x3c5         /* D,A,I,F masked, EL1h */
    msr     spsr_el2, x0

    adr     x0, setup_fpu
    msr     elr_el2, x0
    eret

setup_fpu:
    /* CRITICAL: Enable FP/SIMD access to prevent "Unknown Instruction" traps */
    mov     x0, #(3 << 20)     /* CPACR_EL1.FPEN = 0b11 (No trap) */
    msr     cpacr_el1, x0
    isb

setup_stack:
    ldr     x0, =stack_top
    bic     x0, x0, #0xF
    mov     sp, x0

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