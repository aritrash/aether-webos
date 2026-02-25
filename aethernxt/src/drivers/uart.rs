use core::ptr::{read_volatile, write_volatile};

const UART0_BASE: usize = 0x09000000;

const UART_DR: usize = 0x00;
const UART_FR: usize = 0x18;
const UART_IBRD: usize = 0x24;
const UART_FBRD: usize = 0x28;
const UART_LCRH: usize = 0x2C;
const UART_CR: usize = 0x30;

pub struct Uart;

impl Uart {
    fn reg(offset: usize) -> *mut u32 {
        (UART0_BASE + offset) as *mut u32
    }

    pub fn init() {
        unsafe {
            // Disable UART
            write_volatile(Self::reg(UART_CR), 0x0000);

            // Set baud rate divisors (works fine in QEMU)
            write_volatile(Self::reg(UART_IBRD), 1);
            write_volatile(Self::reg(UART_FBRD), 40);

            // 8 bits, no parity, 1 stop bit
            write_volatile(Self::reg(UART_LCRH), (1 << 4) | (1 << 5) | (1 << 6));

            // Enable UART, TX, RX
            write_volatile(Self::reg(UART_CR), (1 << 0) | (1 << 8) | (1 << 9));
        }
    }

    pub fn putc(c: u8) {
        unsafe {
            while read_volatile(Self::reg(UART_FR)) & (1 << 5) != 0 {}
            write_volatile(Self::reg(UART_DR), c as u32);
        }
    }

    pub fn puts(s: &str) {
        for b in s.bytes() {
            if b == b'\n' {
                Self::putc(b'\r');
            }
            Self::putc(b);
        }
    }
}