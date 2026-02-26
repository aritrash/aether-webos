use core::fmt::{self, Write};
use crate::drivers::uart;

pub struct Console;

impl Write for Console {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        uart::puts(s);
        Ok(())
    }
}

pub fn print(args: fmt::Arguments) {
    Console.write_fmt(args).unwrap();
}