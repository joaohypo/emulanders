#![no_std]
#![no_main]

#[macro_use]
extern crate nx;

extern crate alloc;

extern crate emulanders;

use nx::diag::abort;
use nx::fs;
use nx::ipc::server;
use nx::rc;
use nx::result::*;
use nx::thread;
use nx::util;

use emulanders::*;

use core::panic;

rrt0_define_default_module_name!();

const CUSTOM_HEAP_SIZE: usize = 0x10000;
static mut CUSTOM_HEAP: [u8; CUSTOM_HEAP_SIZE] = [0; CUSTOM_HEAP_SIZE];

#[unsafe(no_mangle)]
#[allow(static_mut_refs)]
pub fn initialize_heap(_override_heap: util::PointerAndSize) -> util::PointerAndSize {
    unsafe {
        // SAFETY: CUSTOM_HEAP must only ever be referenced from here, and nowhere else.
        util::PointerAndSize::new(&raw mut CUSTOM_HEAP as _, CUSTOM_HEAP.len())
    }
}

#[unsafe(no_mangle)]
pub fn main() -> Result<()> {
    thread::set_current_thread_name("emulanders.Main");
    fs::initialize_fspsrv_session()?;
    fs::mount_sd_card("sdmc")?;

    if let Err(rc) = fsext::ensure_directories() {
        log!("Error creating directories: {:?}\n", rc);
    }

    if let Err(e) = nx::rand::initialize() {
        log!("Error initializing rand provider: {:?}\n", e);
    }

    emu::load_emulation_status();

    const POINTER_BUF_SIZE: usize = 0x1000;
    type Manager = server::ServerManager<POINTER_BUF_SIZE>;

    log!("Servicing IPC...\n");

    let mut manager = Manager::new()?;
    manager.register_mitm_service_server::<ipc::nfc::UserManager>()?;
    manager.register_service_server::<ipc::emu::EmulandersServer>()?;

    if let Err(e) = manager.loop_process() {
        log!("Error occured running server manager loop: {:?}", e);
    }

    panic!("exiting MitM Servers is not supported.");
}

#[panic_handler]
fn panic_handler(_info: &panic::PanicInfo) -> ! {
    // get_current_thread_name() usually returns a static str or implements Display in this crate structure.
    log!("Panic! at emulanders thread\n");
    abort::abort(abort::AbortLevel::SvcBreak(), rc::ResultPanicked::make())
}
