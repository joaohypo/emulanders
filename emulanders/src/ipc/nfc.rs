use nx::ipc::sf;
use nx::ipc::server;
use nx::result::*;
use nx::ipc::sf::sm;
use nx::ipc::sf::applet;
use nx::version;
use nx::sync;
use crate::emu;

use core::sync::atomic::{AtomicU32, AtomicBool, Ordering};

// Mifare Specific types
pub type DeviceHandle = u64;

// ── Global Mifare NFC State Machine ──────────────────────────────────────────
// DeviceState enum values (from nn::nfc::DeviceState / nfp::DeviceState):
//   0 = Initialized      (interface created, nothing happening)
//   1 = SearchingForTag   (StartDetection called, waiting for tag)
//   2 = TagFound          (activate_event signaled, tag present)
//   3 = TagRemoved
//   4 = TagMounted
//   5 = Unavailable

pub static G_MIFARE_DEVICE_STATE: AtomicU32 = AtomicU32::new(0);
pub static G_MIFARE_DETECTION_ACTIVE: AtomicBool = AtomicBool::new(false);
pub static G_MIFARE_ACTIVATE_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);
pub static G_MIFARE_DEACTIVATE_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);
pub static G_MIFARE_AVAILABILITY_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);

/// Called from emu.rs when a skylander is selected via the overlay.
/// If detection is active (game is on "insert figure" screen), signal the
/// activate event so the game detects the tag.
pub fn notify_skylander_selected() {
    let det = G_MIFARE_DETECTION_ACTIVE.load(Ordering::SeqCst);
    let prev_state = G_MIFARE_DEVICE_STATE.load(Ordering::SeqCst);
    // Only signal if NOT already in TagFound state — prevent re-notification loop
    if prev_state == 2 {
        return; // Already TagFound, don't re-signal
    }
    log!("MifareNFC: notify_skylander_selected (detection_active={}, prev_state={})\n", det, prev_state);
    // nfc:mf:u has NO Mount command, so state goes directly to TagFound(2)
    G_MIFARE_DEVICE_STATE.store(2, Ordering::SeqCst); // TagFound
    if let Some(event) = G_MIFARE_ACTIVATE_EVENT.lock().as_ref() {
        log!("MifareNFC: -> Signaling activate event!\n");
        let _ = event.signal();
    }
}

/// Called from emu.rs when a skylander is deselected.
pub fn notify_skylander_removed() {
    log!("MifareNFC: notify_skylander_removed\n");
    if G_MIFARE_DETECTION_ACTIVE.load(Ordering::SeqCst) {
        G_MIFARE_DEVICE_STATE.store(1, Ordering::SeqCst); // SearchingForTag
    } else {
        G_MIFARE_DEVICE_STATE.store(0, Ordering::SeqCst); // Initialized
    }
    if let Some(event) = G_MIFARE_DEACTIVATE_EVENT.lock().as_ref() {
        let _ = event.signal();
    }
}

// Ensure global events are created (idempotent, called on CreateUserInterface)
pub fn ensure_global_events() -> Result<()> {
    {
        let mut guard = G_MIFARE_ACTIVATE_EVENT.lock();
        if guard.is_none() {
            *guard = Some(nx::wait::SystemEvent::new()?);
        }
    }
    {
        let mut guard = G_MIFARE_DEACTIVATE_EVENT.lock();
        if guard.is_none() {
            *guard = Some(nx::wait::SystemEvent::new()?);
        }
    }
    {
        let mut guard = G_MIFARE_AVAILABILITY_EVENT.lock();
        if guard.is_none() {
            let event = nx::wait::SystemEvent::new()?;
            let _ = event.signal(); // Signal immediately: NFC hardware is "available"
            *guard = Some(event);
        }
    }
    Ok(())
}

// ── IPC Interface Definition (Switchbrew nfc:mf:u IUser, 14 commands) ────────
ipc_sf_define_interface_trait! {
    trait MifareUser {
        initialize [0, version::VersionInterval::all()]: (aruid: applet::AppletResourceUserId, process_id: sf::ProcessId, mcu_data: sf::InMapAliasBuffer<u8>) => () ();
        finalize [1, version::VersionInterval::all()]: () => () ();
        list_devices [2, version::VersionInterval::all()]: (handles: sf::OutPointerBuffer<DeviceHandle>) => (count: u32) (count: u32);
        start_detection [3, version::VersionInterval::all()]: (handle: DeviceHandle) => () ();
        stop_detection [4, version::VersionInterval::all()]: (handle: DeviceHandle) => () ();
        read [5, version::VersionInterval::all()]: (handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>, out_data: sf::OutMapAliasBuffer<u8>) => () ();
        write [6, version::VersionInterval::all()]: (handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>) => () ();
        get_tag_info [7, version::VersionInterval::all()]: (handle: DeviceHandle, out_tag_info: sf::OutFixedPointerBuffer<[u8; 0x58]>) => () ();
        get_activate_event_handle [8, version::VersionInterval::all()]: (handle: DeviceHandle) => (event: sf::CopyHandle) (event: sf::CopyHandle);
        get_deactivate_event_handle [9, version::VersionInterval::all()]: (handle: DeviceHandle) => (event: sf::CopyHandle) (event: sf::CopyHandle);
        get_state [10, version::VersionInterval::all()]: () => (state: u32) (state: u32);
        get_device_state [11, version::VersionInterval::all()]: (handle: DeviceHandle) => (state: u32) (state: u32);
        get_npad_id [12, version::VersionInterval::all()]: (handle: DeviceHandle) => (npad_id: u32) (npad_id: u32);
        get_availability_change_event_handle [13, version::VersionInterval::all()]: () => (event: sf::CopyHandle) (event: sf::CopyHandle);
    }
}

ipc_sf_define_interface_trait! {
    trait MifareUserManager {
        create_user_interface [0, version::VersionInterval::all()]: () => (user_interface: sf::MoveHandle) (user_interface: impl IMifareUserServer + 'static);
    }
}

// ── Implementation ───────────────────────────────────────────────────────────
pub struct UserEmulator {
    info: sm::mitm::MitmProcessInfo,
}

impl IMifareUserServer for UserEmulator {
    fn initialize(&mut self, _aruid: applet::AppletResourceUserId, _process_id: sf::ProcessId, _mcu_data: sf::InMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] MifareUser::Initialize\n", self.info.program_id.0);
        emu::register_intercepted_application_id(self.info.program_id);
        G_MIFARE_DEVICE_STATE.store(0, Ordering::SeqCst); // Initialized
        G_MIFARE_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        // Signal Availability so the game knows NFC hardware is ready
        if let Some(event) = G_MIFARE_AVAILABILITY_EVENT.lock().as_ref() {
            log!("[{:#X}]   -> Signaling AvailabilityChangeEvent\n", self.info.program_id.0);
            let _ = event.signal();
        }
        Ok(())
    }

    fn finalize(&mut self) -> Result<()> {
        log!("[{:#X}] MifareUser::Finalize\n", self.info.program_id.0);
        emu::unregister_intercepted_application_id(self.info.program_id);
        G_MIFARE_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        G_MIFARE_DEVICE_STATE.store(0, Ordering::SeqCst);
        Ok(())
    }

    fn list_devices(&mut self, handles: sf::OutPointerBuffer<DeviceHandle>) -> Result<u32> {
        // No log here - called in tight loop, floods buffer
        if handles.get_size() >= 8 {
            unsafe {
                // DeviceHandle: { id: u32, reserved: [u8; 4] } = 8 bytes
                // NpadIdType::No1 = 0
                let ptr = handles.get_address() as *mut u8;
                core::ptr::write_bytes(ptr, 0, 8);
            }
            Ok(1)
        } else {
            Ok(0)
        }
    }

    fn start_detection(&mut self, _handle: DeviceHandle) -> Result<()> {
        log!("[{:#X}] MifareUser::StartDetection\n", self.info.program_id.0);
        G_MIFARE_DETECTION_ACTIVE.store(true, Ordering::SeqCst);
        G_MIFARE_DEVICE_STATE.store(1, Ordering::SeqCst); // SearchingForTag

        // If a skylander is already loaded when detection starts, signal immediately
        if emu::get_active_virtual_skylander().is_some() {
            log!("[{:#X}]   -> Skylander already loaded, signaling TagFound\n", self.info.program_id.0);
            G_MIFARE_DEVICE_STATE.store(2, Ordering::SeqCst); // TagFound (NOT TagMounted!)
            if let Some(event) = G_MIFARE_ACTIVATE_EVENT.lock().as_ref() {
                let _ = event.signal();
            }
        }
        Ok(())
    }

    fn stop_detection(&mut self, _handle: DeviceHandle) -> Result<()> {
        log!("[{:#X}] MifareUser::StopDetection\n", self.info.program_id.0);
        G_MIFARE_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        G_MIFARE_DEVICE_STATE.store(0, Ordering::SeqCst);
        Ok(())
    }

    fn read(&mut self, _handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>, out_data: sf::OutMapAliasBuffer<u8>) -> Result<()> {
        // MifareReadBlockParameter: { sector_key[6], command(u8), block_index(u8), pad[16] } = 24 bytes
        // MifareReadBlockData:      { data[16], block_index(u8), pad[7] }                    = 24 bytes each
        let param_size = 24usize;
        let result_size = 24usize;
        let num_blocks = in_params.get_size() / param_size;
        log!("[{:#X}] MifareUser::Read (num_blocks={}, in_size={}, out_size={})\n",
             self.info.program_id.0, num_blocks, in_params.get_size(), out_data.get_size());

        let skylander_guard = emu::get_active_virtual_skylander();
        for i in 0..num_blocks {
            let param_offset = i * param_size;
            let result_offset = i * result_size;

            // Parse block_index from the read parameter
            // Hex dump proved: block_index is at offset 0!
            let block_index = unsafe {
                *(in_params.get_address().add(param_offset) as *const u8)
            };

            // Log first 3 block reads for diagnostics
            if i < 3 {
                log!("  Read[{}]: block={} (sector {}, block_in_sector {})\n", i, block_index, block_index / 4, block_index % 4);
            }

            let sector = block_index / 4;
            let block_in_sector = block_index % 4;

            let block_data = if let Some(skylander) = skylander_guard.as_ref() {
                skylander.get_block(sector, block_in_sector)
            } else {
                [0u8; 16]
            };

            // Write MifareReadBlockData: 16 bytes data + 1 byte block_index + 7 bytes pad
            if result_offset + result_size <= out_data.get_size() {
                unsafe {
                    let out_ptr = (out_data.get_address() as *mut u8).add(result_offset);
                    core::ptr::copy_nonoverlapping(block_data.as_ptr(), out_ptr, 16);
                    *out_ptr.add(16) = block_index;
                    core::ptr::write_bytes(out_ptr.add(17), 0, 7); // pad
                }
            }
        }
        Ok(())
    }

    fn write(&mut self, _handle: DeviceHandle, _in_params: sf::InMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] MifareUser::Write\n", self.info.program_id.0);
        // Accept the write but don't persist (read-only emulation for now)
        Ok(())
    }

    fn get_tag_info(&mut self, _handle: DeviceHandle, out_tag_info: sf::OutFixedPointerBuffer<[u8; 0x58]>) -> Result<()> {
        log!("[{:#X}] MifareUser::GetTagInfo\n", self.info.program_id.0);
        // TagInfo struct (0x58 bytes):
        //   0x00: TagId = { uuid[10], uuid_length(u8), reserved[0x15] } = 0x20 bytes
        //   0x20: protocol (u32) - 2 for Mifare
        //   0x24: tag_type (u32) - 4 for Mifare Classic
        //   0x28: reserved (0x30 bytes)
        let mut info = [0u8; 0x58];
        // Fill UID from skylander dump (block 0, bytes 0-3 are the UID for Mifare Classic)
        if let Some(skylander) = emu::get_active_virtual_skylander().as_ref() {
            let uid = skylander.get_uid();
            for (i, &b) in uid.iter().enumerate().take(4) {
                info[i] = b;
            }
            info[10] = 4; // uuid_length = 4 bytes for Mifare Classic
        }
        // Protocol = 2 (Mifare)
        info[0x20] = 0x02;
        // Tag type = 4 (Mifare Classic 1K)
        info[0x24] = 0x04;

        unsafe {
            core::ptr::copy_nonoverlapping(info.as_ptr(), out_tag_info.get_address() as *mut u8, 0x58);
        }
        Ok(())
    }

    fn get_activate_event_handle(&mut self, _handle: DeviceHandle) -> Result<sf::CopyHandle> {
        log!("[{:#X}] MifareUser::GetActivateEventHandle\n", self.info.program_id.0);
        let guard = G_MIFARE_ACTIVATE_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }

    fn get_deactivate_event_handle(&mut self, _handle: DeviceHandle) -> Result<sf::CopyHandle> {
        log!("[{:#X}] MifareUser::GetDeactivateEventHandle\n", self.info.program_id.0);
        let guard = G_MIFARE_DEACTIVATE_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }

    fn get_state(&mut self) -> Result<u32> {
        Ok(1) // State::Initialized (always, once Initialize was called)
    }

    fn get_device_state(&mut self, _handle: DeviceHandle) -> Result<u32> {
        let state = G_MIFARE_DEVICE_STATE.load(Ordering::SeqCst);
        Ok(state)
    }

    fn get_npad_id(&mut self, _handle: DeviceHandle) -> Result<u32> {
        // NpadIdType::No1 = 0
        Ok(0)
    }

    fn get_availability_change_event_handle(&mut self) -> Result<sf::CopyHandle> {
        log!("[{:#X}] MifareUser::GetAvailabilityChangeEventHandle\n", self.info.program_id.0);
        let guard = G_MIFARE_AVAILABILITY_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }
}

impl server::ISessionObject for UserEmulator {
    fn try_handle_request_by_id(&mut self, id: u32, protocol: nx::ipc::CommandProtocol, server_ctx: &mut server::ServerContext) -> Option<Result<()>> {
        let result = IMifareUserServer::try_handle_request_by_id(self, id, protocol, server_ctx);
        // Suppress high-frequency polling: ListDevices(2), GetState(10), GetDeviceState(11), GetNpadId(12)
        if id != 2 && id != 10 && id != 11 && id != 12 {
            if result.is_none() {
                log!("[MifareUser] !! UNHANDLED CMD ID={} !!\n", id);
            } else {
                log!("[MifareUser] handled CMD ID={}\n", id);
            }
        }
        result
    }
}

pub struct UserManager {
    info: sm::mitm::MitmProcessInfo,
}

impl IMifareUserManagerServer for UserManager {
    fn create_user_interface(&mut self) -> Result<impl IMifareUserServer + 'static> {
        log!("[{:#X}] MifareUserManager::CreateUserInterface\n", self.info.program_id.0);
        ensure_global_events()?;
        G_MIFARE_DEVICE_STATE.store(0, Ordering::SeqCst);

        Ok(UserEmulator { info: self.info.clone() })
    }
}

impl server::ISessionObject for UserManager {
    fn try_handle_request_by_id(&mut self, id: u32, protocol: nx::ipc::CommandProtocol, server_ctx: &mut server::ServerContext) -> Option<Result<()>> {
        let result = IMifareUserManagerServer::try_handle_request_by_id(self, id, protocol, server_ctx);
        if result.is_none() {
            log!("[MifareUserMgr] !! UNHANDLED CMD ID={} !!\n", id);
        } else {
            log!("[MifareUserMgr] handled CMD ID={}\n", id);
        }
        result
    }
}

impl server::IMitmServerObject for UserManager {
    fn new(info: sm::mitm::MitmProcessInfo) -> Self {
        Self { info }
    }
}

impl server::IMitmService for UserManager {
    fn get_name() -> sm::ServiceName { sm::ServiceName::new("nfc:mf:u") }
    fn should_mitm(info: sm::mitm::MitmProcessInfo) -> bool {
        emu::record_mitm_request(info.program_id.0);
        emu::is_emulation_on()
    }
}
