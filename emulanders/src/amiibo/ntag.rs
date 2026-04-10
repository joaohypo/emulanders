#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Manufacturer1 {
    pub uid_p1: [u8; 0x3],
    pub check_byte_1: u8,
    pub uid_p2: [u8; 0x4]
}
const_assert!(core::mem::size_of::<Manufacturer1>() == 0x8);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Manufacturer2 {
    pub check_byte_2: u8,
    pub internal: u8,
    pub static_lock_bytes: [u8; 0x2],
    pub capability_container: [u8; 0x4]
}
const_assert!(core::mem::size_of::<Manufacturer2>() == 0x8);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct DynamicLock {
    pub dyn_lock_bytes: [u8; 0x3],
    pub rfui: u8
}
const_assert!(core::mem::size_of::<DynamicLock>() == 0x4);

#[derive(Copy, Clone, PartialEq, Eq, Debug, Default)]
#[repr(C)]
pub struct Config {
    pub cfg_0: [u8; 0x4],
    pub cfg_1: [u8; 0x4]
}
const_assert!(core::mem::size_of::<Config>() == 0x8);