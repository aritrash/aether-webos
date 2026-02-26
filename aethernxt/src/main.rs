#![no_std]
#![no_main]

mod arch;
mod drivers;
mod kernel;
mod macros;

use core::panic::PanicInfo;

#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    kernel::init()
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    crate::drivers::uart::puts("Kernel Panic!\n");
    loop {}
}