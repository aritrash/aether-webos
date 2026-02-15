/* src/vectors.S */
.section ".vectors", "ax"

// --- THE MISSING LINKER PIECE ---
.global exceptions_init
exceptions_init:
    ldr x0, =exception_vector_table
    msr vbar_el1, x0    // Set Vector Base Address Register
    isb                 // Instruction Synchronization Barrier
    ret

.macro ventry label
    .align 7
    b \label
.endm

.align 11
.global exception_vector_table
exception_vector_table:
    /* Current EL with SP0 */
    ventry invalid_exception_handler
    ventry invalid_exception_handler
    ventry invalid_exception_handler
    ventry invalid_exception_handler

    /* Current EL with SPx */
    ventry sync_el1h
    ventry irq_el1h
    ventry invalid_exception_handler
    ventry invalid_exception_handler

    /* Lower EL (AArch64) */
    .rept 4
    ventry invalid_exception_handler
    .endr

    /* Lower EL (AArch32) */
    .rept 4
    ventry invalid_exception_handler
    .endr

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

.macro kernel_exit
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
.endm

.global sync_el1h
sync_el1h:
    kernel_entry
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    mov x3, sp
    bl handle_sync_exception
    kernel_exit

.global irq_el1h
irq_el1h:
    kernel_entry
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    mov x3, sp
    bl handle_irq_exception
    kernel_exit

.global invalid_exception_handler
invalid_exception_handler:
#ifdef BOARD_RPI4
    ldr x4, =0xFE201000
#else
    ldr x4, =0x09000000
#endif
    mov w5, #69 // 'E'
    str w5, [x4]
1:  wfe
    b 1b