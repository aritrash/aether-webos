use crate::kprintln;

pub fn init() -> ! {
    kprintln!("[OK] AetherNxt Booted");
    kprintln!("[OK] EL1 Online");
    kprintln!("[OK] UART Initialized");

    let mut el: u64;
    unsafe {
        core::arch::asm!("mrs {}, CurrentEL", out(reg) el);
    }
    kprintln!("CurrentEL = {}", el >> 2);
    unsafe {
        core::arch::asm!("svc #0");
    }

    loop {}
}