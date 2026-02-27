core::arch::global_asm!(
r#"
.section .vectors, "ax"
.align 11
.global __vectors

__vectors:

// 0x000 Sync current EL (SP0)
.align 7
b handle_sync

// 0x080 IRQ current EL (SP0)
.align 7
b handle_irq

// 0x100 FIQ current EL (SP0)
.align 7
b handle_fiq

// 0x180 SError current EL (SP0)
.align 7
b handle_serr

// 0x200 Sync current EL (SPx)
.align 7
b handle_sync

// 0x280 IRQ current EL (SPx)
.align 7
b handle_irq

// 0x300 FIQ current EL (SPx)
.align 7
b handle_fiq

// 0x380 SError current EL (SPx)
.align 7
b handle_serr

// 0x400 Sync lower EL AArch64
.align 7
b handle_sync

// 0x480 IRQ lower EL AArch64
.align 7
b handle_irq

// 0x500 FIQ lower EL AArch64
.align 7
b handle_fiq

// 0x580 SError lower EL AArch64
.align 7
b handle_serr

// 0x600 Sync lower EL AArch32
.align 7
b handle_sync

// 0x680 IRQ lower EL AArch32
.align 7
b handle_irq

// 0x700 FIQ lower EL AArch32
.align 7
b handle_fiq

// 0x780 SError lower EL AArch32
.align 7
b handle_serr


handle_sync:
    bl rust_exception_handler
1:  wfe
    b 1b

handle_irq:
    bl rust_exception_handler
1:  wfe
    b 1b

handle_fiq:
    bl rust_exception_handler
1:  wfe
    b 1b

handle_serr:
    bl rust_exception_handler
1:  wfe
    b 1b
"#
);

fn decode_ec(ec: u64) -> &'static str {
    match ec {
        0x15 => "SVC (Supervisor Call)",
        0x20 => "Instruction Abort (Lower EL)",
        0x21 => "Instruction Abort (Current EL)",
        0x24 => "Data Abort (Lower EL)",
        0x25 => "Data Abort (Current EL)",
        _ => "Unknown",
    }
}

#[no_mangle]
pub extern "C" fn rust_exception_handler() {
    let esr: u64;
    let elr: u64;
    let far: u64;

    unsafe {
        core::arch::asm!("mrs {}, esr_el1", out(reg) esr);
        core::arch::asm!("mrs {}, elr_el1", out(reg) elr);
        core::arch::asm!("mrs {}, far_el1", out(reg) far);
    }

    let ec = (esr >> 26) & 0b111111;
    let il = (esr >> 25) & 1;
    let iss = esr & 0x1FFFFFF;

    crate::kprintln!("\n=== EXCEPTION ===");
    crate::kprintln!("ESR_EL1 = {:#018x}", esr);
    crate::kprintln!("  EC  = {:#x} ({})", ec, decode_ec(ec));
    crate::kprintln!("  IL  = {}", il);
    crate::kprintln!("  ISS = {:#x}", iss);
    crate::kprintln!("ELR_EL1 = {:#018x}", elr);
    crate::kprintln!("FAR_EL1 = {:#018x}", far);

    loop {}
}

