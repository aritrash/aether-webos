#![no_std]
#![no_main]

use core::panic::PanicInfo;

mod arch;
mod drivers;
mod kernel;
mod memory;
mod net;

//
// Rust entry called from assembly
//
#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    unsafe {
        core::ptr::write_volatile(0x09000000 as *mut u32, 'X' as u32);
    }
    kernel::init::kernel_init()
}

//
// Panic handler (delegates to kernel later)
//
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    drivers::uart::Uart::puts("Kernel Panic\n");
    loop {}
}