// src/arch/aarch64/boot.rs

core::arch::global_asm!(
r#"
.section .text.boot
.global _start

_start:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbz x0, master

1:
    wfe
    b 1b

master:
    mrs x0, CurrentEL
    and x0, x0, #0b1100
    lsr x0, x0, #2
    cmp x0, #2
    b.ne setup_fpu

    mrs x0, cnthctl_el2
    orr x0, x0, #3
    msr cnthctl_el2, x0
    msr cntvoff_el2, xzr

    mov x0, #(1 << 31)
    orr x0, x0, #(1 << 1)
    msr hcr_el2, x0

    mov x0, #0x3c5
    msr spsr_el2, x0

    adr x0, setup_fpu
    msr elr_el2, x0
    eret

setup_fpu:
    mov x0, #(3 << 20)
    msr cpacr_el1, x0
    isb

    ldr x0, =_stack_top
    mov sp, x0

    // ACtivate Vectors
    ldr x0, =__vectors
    msr vbar_el1, x0
    isb

    // Clear BSS
    ldr x0, =__bss_start
    ldr x1, =__bss_end

clear_bss:
    cmp x0, x1
    b.ge bss_done
    str xzr, [x0], #8
    b clear_bss

bss_done:
    bl rust_main

hang:
    wfe
    b hang
"#
);