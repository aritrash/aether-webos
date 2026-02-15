// src/vectors.S
.section ".vectors", "ax"

.macro ventry label
    .align 7
    b \label
.endm

/* Vector table MUST be 2KB aligned */
.align 11
.global exception_vector_table
exception_vector_table:
    /* Current EL with SP0 (Should not happen in AETHER OS) */
    ventry sync_invalid_el1t
    ventry irq_invalid_el1t
    ventry fiq_invalid_el1t
    ventry error_invalid_el1t

    /* Current EL with SPx (This is where AETHER kernel exceptions land) */
    ventry sync_el1h
    ventry irq_el1h
    ventry fiq_invalid_el1h
    ventry error_invalid_el1h

    /* Lower EL (AArch64/AArch32) */
    ventry sync_invalid_el0_64
    ventry irq_invalid_el0_64
    ventry fiq_invalid_el0_64
    ventry error_invalid_el0_64
    ventry sync_invalid_el0_32
    ventry irq_invalid_el0_32
    ventry fiq_invalid_el0_32
    ventry error_invalid_el0_32

/* Context Saving Macro */
.macro kernel_entry
    sub sp, sp, #256
    stp x0, x1, [sp, #16 * 0]
    stp x2, x3, [sp, #16 * 1]
    stp x4, x5, [sp, #16 * 2]
    stp x6, x7, [sp, #16 * 3]
    stp x8, x9, [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]
    str x30, [sp, #16 * 15]
.endm

/* Context Restoration Macro */
.macro kernel_exit
    ldr x30, [sp, #16 * 15]
    ldp x28, x29, [sp, #16 * 14]
    ldp x26, x27, [sp, #16 * 13]
    ldp x24, x25, [sp, #16 * 12]
    ldp x22, x23, [sp, #16 * 11]
    ldp x20, x21, [sp, #16 * 10]
    ldp x18, x19, [sp, #16 * 9]   // <--- FIXED: Was #16 * 11
    ldp x16, x17, [sp, #16 * 8]
    ldp x14, x15, [sp, #16 * 7]
    ldp x12, x13, [sp, #16 * 6]
    ldp x10, x11, [sp, #16 * 5]
    ldp x8, x9, [sp, #16 * 4]
    ldp x6, x7, [sp, #16 * 3]
    ldp x4, x5, [sp, #16 * 2]
    ldp x2, x3, [sp, #16 * 1]
    ldp x0, x1, [sp, #16 * 0]
    add sp, sp, #256
    eret
.endm

sync_el1h:
    kernel_entry
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    mov x3, sp
    bl handle_sync_exception
    kernel_exit

irq_el1h:
    kernel_entry
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    mov x3, sp
    bl handle_irq_exception
    kernel_exit

/* Panic Handler: Writes 'P' to UART */
invalid_exception_handler:
#ifdef BOARD_RPI4
    ldr x4, =0xFE201000
#else
    ldr x4, =0x09000000
#endif
    mov w5, #80 // 'P'
    str w5, [x4]
    wfe
    b invalid_exception_handler

sync_invalid_el1t:
irq_invalid_el1t:
fiq_invalid_el1t:
error_invalid_el1t:
fiq_invalid_el1h:
error_invalid_el1h:
sync_invalid_el0_64:
irq_invalid_el0_64:
fiq_invalid_el0_64:
error_invalid_el0_64:
sync_invalid_el0_32:
irq_invalid_el0_32:
fiq_invalid_el0_32:
error_invalid_el0_32:
    b invalid_exception_handler

.global exceptions_init
exceptions_init:
    ldr x0, =exception_vector_table
    msr vbar_el1, x0
    isb
    ret