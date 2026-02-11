// src/vectors.S
.section ".vectors", "ax"

// Each entry in the vector table must be exactly 128 bytes (2^7).
.macro ventry label
    .align 7
    b \label
.endm

/* * The vector table MUST be aligned to a 2KB boundary (2^11).
 */
.align 11
.global exception_vector_table
exception_vector_table:
    /* Current EL with SP0 */
    ventry sync_invalid_el1t
    ventry irq_invalid_el1t
    ventry fiq_invalid_el1t
    ventry error_invalid_el1t

    /* Current EL with SPx */
    ventry sync_el1h
    ventry irq_invalid_el1h
    ventry fiq_invalid_el1h
    ventry error_invalid_el1h

    /* Lower EL using AArch64 */
    ventry sync_invalid_el0_64
    ventry irq_invalid_el0_64
    ventry fiq_invalid_el0_64
    ventry error_invalid_el0_64

    /* Lower EL using AArch32 */
    ventry sync_invalid_el0_32
    ventry irq_invalid_el0_32
    ventry fiq_invalid_el0_32
    ventry error_invalid_el0_32

/* Primary Handler for Synchronous Exceptions */
sync_el1h:
    /* 1. Save all registers x0-x30 to the stack (Trap Frame) */
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

    /* DEBUG: Direct UART Write '!' to confirm handler entry */
    ldr x4, =0xFE201000      // UART0 Base (RPi4 Physical)
    mov w5, #33              // ASCII '!'
    str w5, [x4]             

    /* 2. Prepare arguments for C handler */
    mrs x0, esr_el1   // Arg 0: Syndrome
    mrs x1, elr_el1   // Arg 1: Faulting PC
    mrs x2, far_el1   // Arg 2: Faulting Address
    mov x3, sp        // Arg 3: Pointer to saved registers (trap_frame_t)

    /* 3. Call C handler */
    bl handle_sync_exception

    /* Restore ALL General Purpose Registers */
    ldr x30, [sp, #16 * 15]
    ldp x28, x29, [sp, #16 * 14]
    ldp x26, x27, [sp, #16 * 13]
    ldp x24, x25, [sp, #16 * 12]
    ldp x22, x23, [sp, #16 * 11]
    ldp x20, x21, [sp, #16 * 10]
    ldp x18, x19, [sp, #16 * 11]
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

/* --- Catch-all for Unimplemented Exceptions --- */
invalid_exception_handler:
    wfe
    b invalid_exception_handler

/* Map all unused vector labels */
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
    dsb sy           // Ensure memory/UART ops complete
    ldr x0, =exception_vector_table
    msr vbar_el1, x0 // Set Vector Base for EL1
    isb              // Flush pipeline
    ret

irq_invalid_el1h:
    /* 1. Save Context */
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

    /* 2. Call the C Handler */
    /* We pass the same args as Sync for consistency, though IRQ doesn't use FAR */
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    mov x3, sp
    bl handle_irq_exception

    /* 3. Restore Context */
    ldr x30, [sp, #16 * 15]
    ldp x28, x29, [sp, #16 * 14]
    ldp x26, x27, [sp, #16 * 13]
    ldp x24, x25, [sp, #16 * 12]
    ldp x22, x23, [sp, #16 * 11]
    ldp x20, x21, [sp, #16 * 10]
    ldp x18, x19, [sp, #16 * 9]
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