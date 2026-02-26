use core::ptr::{read_volatile, write_volatile};

const UART_BASE: usize = 0x09000000;

const DR: usize = 0x00;
const FR: usize = 0x18;

#[inline(always)]
fn reg(offset: usize) -> *mut u32 {
    (UART_BASE + offset) as *mut u32
}

pub fn putc(c: u8) {
    unsafe {
        // Wait until TX FIFO not full (TXFF bit = 5)
        while read_volatile(reg(FR)) & (1 << 5) != 0 {}
        write_volatile(reg(DR), c as u32);
    }
}

pub fn puts(s: &str) {
    for byte in s.bytes() {
        if byte == b'\n' {
            putc(b'\r');
        }
        putc(byte);
    }
}