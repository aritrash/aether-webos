#[macro_export]
macro_rules! kprint {
    ($($arg:tt)*) => {
        $crate::drivers::console::print(format_args!($($arg)*));
    };
}

#[macro_export]
macro_rules! kprintln {
    () => ($crate::kprint!("\n"));
    ($fmt:expr) => ($crate::kprint!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (
        $crate::kprint!(concat!($fmt, "\n"), $($arg)*)
    );
}