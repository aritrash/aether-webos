use crate::kprintln;

pub fn init() -> ! {
    kprintln!("[OK] AetherNxt Booted");
    kprintln!("[OK] EL1 Online");
    kprintln!("[OK] UART Initialized");

    loop {}
}