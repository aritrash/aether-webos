.section ".text.boot"

.global _start

_start:
    // Read Multiprocessor Affinity Register (MPIDR_EL1)
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF        // Mask out the CPU ID
    cbz     x0, master           // If ID == 0, go to master logic
    
hang: 
    wfe                          // Wait For Event (low power sleep)
    b       hang                 // Park cores 1, 2, and 3 here

master:
    // Set stack pointer
    ldr     x0, =stack_top
    mov     sp, x0

    // Clear BSS (Zero out uninitialized variables)
    ldr     x0, =__bss_start
    ldr     w1, =__bss_size
setup_bss:
    cbz     w1, jump_to_main
    str     xzr, [x0], #8
    subs    w1, w1, #1
    bne     setup_bss

jump_to_main:
    bl      kernel_main          // Jump to our C code
    b       hang                 // If kernel returns, safety hang