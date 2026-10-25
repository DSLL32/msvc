//! Keep host bookkeeping from changing the application's libc errno.
//! This does not specify what the dynamic loader itself does to errno.
pub fn get() -> i32 {
    unsafe { *libc::__errno_location() }
}
pub fn set(value: i32) {
    unsafe { *libc::__errno_location() = value };
}
pub struct Preserve(i32);
impl Preserve {
    pub fn new() -> Self {
        Self(get())
    }
}
impl Drop for Preserve {
    fn drop(&mut self) {
        set(self.0);
    }
}
/// Capture immediately after the target callback, before caller bookkeeping.
pub fn invoke<T>(value: i32, callback: impl FnOnce() -> T) -> (T, i32) {
    set(value);
    let result = callback();
    (result, get())
}
