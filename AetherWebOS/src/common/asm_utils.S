.section ".text"

.global enable_interrupts
enable_interrupts:
    msr daifclr, #2    /* Clears the 'I' bit (IRQ) in DAIF */
    isb
    ret

.global disable_interrupts
disable_interrupts:
    msr daifset, #2    /* Sets the 'I' bit (IRQ) in DAIF */
    ret