pub mod boot;

// Force inclusion of boot module
#[allow(dead_code)]
fn _force_link() {
    let _ = boot::_BOOT_INCLUDED;
}