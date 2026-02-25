use crate::drivers::uart::Uart;

pub fn kernel_init() -> ! {
    Uart::init();
    Uart::puts("AetherNxt Booted\n");

    loop {
        unsafe { core::arch::asm!("wfe"); }
    }
}