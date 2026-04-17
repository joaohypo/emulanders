use crate::skylander;
use crate::fsext;
use alloc::vec::Vec;
use nx::ipc::sf::ncm;
use nx::sync;

use atomic_enum::atomic_enum;

use core::sync::atomic::{AtomicBool, Ordering};
#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, Copy, Clone)]
#[repr(C)]
pub struct Version {
    pub major: u8,
    pub minor: u8,
    pub micro: u8,
    pub is_dev_build: bool,
}

impl Version {
    pub const fn from(major: u8, minor: u8, micro: u8, is_dev_build: bool) -> Self {
        Self {
            major: major,
            minor: minor,
            micro: micro,
            is_dev_build: is_dev_build,
        }
    }
}

#[atomic_enum]
#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, PartialEq, Eq)]
#[repr(u32)]
pub enum EmulationStatus {
    On,
    Off,
}

#[atomic_enum]
#[derive(nx::ipc::sf::Request, nx::ipc::sf::Response, PartialEq, Eq)]
#[repr(u32)]
#[allow(dead_code)]
pub enum VirtualSkylanderStatus {
    Invalid,
    Connected,
    Disconnected,
}

#[cfg(debug_assertions)]
pub const IS_DEV_BUILD: bool = true;

#[cfg(not(debug_assertions))]
pub const IS_DEV_BUILD: bool = false;

pub const CURRENT_VERSION: Version = Version::from(0, 9, 1, IS_DEV_BUILD);

static G_EMULATION_STATUS: AtomicEmulationStatus = AtomicEmulationStatus::new(EmulationStatus::Off);
static G_ACTIVE_VIRTUAL_SKYLANDER_STATUS: AtomicVirtualSkylanderStatus =
    AtomicVirtualSkylanderStatus::new(VirtualSkylanderStatus::Invalid);
static G_INTERCEPTED_APPLICATION_IDS: sync::Mutex<Vec<u64>> = sync::Mutex::new(Vec::new());
static G_ACTIVE_VIRTUAL_SKYLANDER: sync::Mutex<Option<skylander::Skylander>> =
    sync::Mutex::new(None);
static G_LAST_MITM_REQUEST_ID: sync::Mutex<u64> = sync::Mutex::new(0);
static G_DEBUG_LOG: sync::Mutex<alloc::string::String> = sync::Mutex::new(alloc::string::String::new());
static G_IS_LOGGING_ENABLED: AtomicBool = AtomicBool::new(false);

const STATUS_ON_FLAG: &str = "status_on";

pub fn log_debug(msg: &str) {
    if !G_IS_LOGGING_ENABLED.load(Ordering::SeqCst) {
        return;
    }
    let mut log = G_DEBUG_LOG.lock();
    if log.len() + msg.len() > 16384 {
        let new_start = log.len() / 2;
        *log = alloc::string::String::from(&log[new_start..]);
    }
    log.push_str(msg);
}

pub fn get_debug_log() -> alloc::string::String {
    G_DEBUG_LOG.lock().clone()
}

pub fn clear_debug_log() {
    G_DEBUG_LOG.lock().clear();
}

pub fn get_logging_status() -> bool {
    G_IS_LOGGING_ENABLED.load(Ordering::SeqCst)
}

pub fn set_logging_status(status: bool) {
    G_IS_LOGGING_ENABLED.store(status, Ordering::SeqCst);
}

pub fn get_last_mitm_request_id() -> u64 {
    *G_LAST_MITM_REQUEST_ID.lock()
}

pub fn record_mitm_request(id: u64) {
    *G_LAST_MITM_REQUEST_ID.lock() = id;
}

pub fn get_emulation_status() -> EmulationStatus {
    G_EMULATION_STATUS.load(Ordering::SeqCst)
}

pub fn load_emulation_status() {
    let status = if fsext::has_flag(STATUS_ON_FLAG) {
        EmulationStatus::On
    } else {
        EmulationStatus::Off
    };

    G_EMULATION_STATUS.store(status, Ordering::SeqCst);
}

pub fn set_emulation_status(status: EmulationStatus) {
    G_EMULATION_STATUS.store(status, Ordering::SeqCst);
    fsext::set_flag(STATUS_ON_FLAG, status == EmulationStatus::On);
}

pub fn get_active_virtual_skylander_status() -> VirtualSkylanderStatus {
    G_ACTIVE_VIRTUAL_SKYLANDER_STATUS.load(Ordering::SeqCst)
}

pub fn set_active_virtual_skylander_status(status: VirtualSkylanderStatus) {
    G_ACTIVE_VIRTUAL_SKYLANDER_STATUS.store(status, Ordering::SeqCst);
}

pub fn register_intercepted_application_id(application_id: ncm::ProgramId) {
    log!("RegisterInterceptedApplicationId -- application_id: {:#X}\n", application_id.0);
    G_INTERCEPTED_APPLICATION_IDS.lock().push(application_id.0);
}

pub fn unregister_intercepted_application_id(application_id: ncm::ProgramId) {
    log!("UnregisterInterceptedApplicationId -- application_id: {:#X}\n", application_id.0);
    G_INTERCEPTED_APPLICATION_IDS
        .lock()
        .retain(|&id| id != application_id.0);
}

pub fn is_application_id_intercepted(application_id: ncm::ProgramId) -> bool {
    G_INTERCEPTED_APPLICATION_IDS
        .lock()
        .contains(&application_id.0)
}

pub fn get_active_virtual_skylander<'a>() -> sync::MutexGuard<'a, Option<skylander::Skylander>> {
    G_ACTIVE_VIRTUAL_SKYLANDER.lock()
}

pub fn set_active_virtual_skylander(skylander: Option<skylander::Skylander>) {
    let has_skylander = skylander.is_some();
    G_ACTIVE_VIRTUAL_SKYLANDER_STATUS.store(
        if has_skylander {
            VirtualSkylanderStatus::Connected
        } else {
            VirtualSkylanderStatus::Invalid
        },
        Ordering::SeqCst,
    );
    *G_ACTIVE_VIRTUAL_SKYLANDER.lock() = skylander;

    if has_skylander {
        crate::ipc::nfc::notify_skylander_selected();
    } else {
        crate::ipc::nfc::notify_skylander_removed();
    }
}

pub fn is_emulation_on() -> bool {
    get_emulation_status() == EmulationStatus::On
}
