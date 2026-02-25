//
// AArch64 Boot Assembly
//

core::arch::global_asm!(
    r#"
    .section .text._start
    .global _start

_start:
    ldr x0, =_stack_top
    mov sp, x0
    bl rust_main

1:
    wfe
    b 1b
"#
);

extern "C" {
    static _stack_top: u8;
}

#[no_mangle]
pub static _BOOT_INCLUDED: u8 = 0;