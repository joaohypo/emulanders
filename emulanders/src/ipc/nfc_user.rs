use nx::ipc::sf;
use nx::ipc::server;
use nx::result::*;
use nx::ipc::sf::sm;
use nx::ipc::sf::applet;
use nx::version;
use nx::sync;
use crate::emu;

use core::sync::atomic::{AtomicU32, AtomicBool, Ordering};

// ── Shared state ─────────────────────────────────────────────────────────────
// We reuse the same globals from nfc.rs for state, but we need our OWN events
// because nfc:user is a separate session from nfc:mf:u.

pub static G_NFC_USER_DEVICE_STATE: AtomicU32 = AtomicU32::new(0);
pub static G_NFC_USER_DETECTION_ACTIVE: AtomicBool = AtomicBool::new(false);
pub static G_NFC_USER_ACTIVATE_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);
pub static G_NFC_USER_DEACTIVATE_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);
pub static G_NFC_USER_AVAILABILITY_EVENT: sync::Mutex<Option<nx::wait::SystemEvent>> = sync::Mutex::new(None);

fn ensure_nfc_user_events() -> Result<()> {
    {
        let mut guard = G_NFC_USER_ACTIVATE_EVENT.lock();
        if guard.is_none() {
            *guard = Some(nx::wait::SystemEvent::new()?);
        }
    }
    {
        let mut guard = G_NFC_USER_DEACTIVATE_EVENT.lock();
        if guard.is_none() {
            *guard = Some(nx::wait::SystemEvent::new()?);
        }
    }
    {
        let mut guard = G_NFC_USER_AVAILABILITY_EVENT.lock();
        if guard.is_none() {
            *guard = Some(nx::wait::SystemEvent::new()?);
        }
    }
    Ok(())
}

/// Called from emu.rs when a skylander is selected via overlay.
pub fn notify_skylander_selected() {
    let det = G_NFC_USER_DETECTION_ACTIVE.load(Ordering::SeqCst);
    let prev = G_NFC_USER_DEVICE_STATE.load(Ordering::SeqCst);
    log!("NfcUser: notify_skylander_selected (det={}, prev_state={})\n", det, prev);
    // Always set TagFound and signal - the game might be waiting
    G_NFC_USER_DEVICE_STATE.store(2, Ordering::SeqCst); // TagFound
    if let Some(event) = G_NFC_USER_ACTIVATE_EVENT.lock().as_ref() {
        log!("NfcUser: -> Signaling activate event!\n");
        let _ = event.signal();
    }
}

pub fn notify_skylander_removed() {
    log!("NfcUser: notify_skylander_removed\n");
    if G_NFC_USER_DETECTION_ACTIVE.load(Ordering::SeqCst) {
        G_NFC_USER_DEVICE_STATE.store(1, Ordering::SeqCst); // SearchingForTag
    } else {
        G_NFC_USER_DEVICE_STATE.store(0, Ordering::SeqCst);
    }
    if let Some(event) = G_NFC_USER_DEACTIVATE_EVENT.lock().as_ref() {
        let _ = event.signal();
    }
}

// ── DeviceHandle ─────────────────────────────────────────────────────────────
pub type DeviceHandle = u64;

// ── nfc:user IUser interface (nn::nfc::detail::IUser) ────────────────────────
// Command IDs from switchbrew (FW 4.0.0+, our target is 21.2):
//   400 = Initialize
//   401 = Finalize
//   402 = GetState
//   403 = IsNfcEnabled
//   404 = ListDevices
//   405 = GetDeviceState
//   406 = GetNpadId
//   407 = AttachAvailabilityChangeEvent
//   408 = StartDetection (takes DeviceHandle + NfcProtocol)
//   409 = StopDetection
//   410 = GetTagInfo
//   411 = AttachActivateEvent
//   412 = AttachDeactivateEvent
//   413 = ReadMifare
//   414 = WriteMifare
//   415 = SendCommandByPassThrough
//   416 = KeepPassThroughSession
//   417 = ReleasePassThroughSession
//
// Pre-4.0.0 commands (0-3) are also defined but won't be called on FW 21.2.

ipc_sf_define_interface_trait! {
    trait NfcUser {
        // Pre-4.0.0 (legacy, kept for completeness)
        initialize_old [0, version::VersionInterval::all()]: (aruid: applet::AppletResourceUserId, process_id: sf::ProcessId, mcu_data: sf::InMapAliasBuffer<u8>) => () ();
        finalize_old [1, version::VersionInterval::all()]: () => () ();
        get_state_old [2, version::VersionInterval::all()]: () => (state: u32) (state: u32);
        is_nfc_enabled_old [3, version::VersionInterval::all()]: () => (enabled: bool) (enabled: bool);

        // 4.0.0+ commands
        initialize [400, version::VersionInterval::all()]: (aruid: applet::AppletResourceUserId, process_id: sf::ProcessId, mcu_data: sf::InMapAliasBuffer<u8>) => () ();
        finalize [401, version::VersionInterval::all()]: () => () ();
        get_state [402, version::VersionInterval::all()]: () => (state: u32) (state: u32);
        is_nfc_enabled [403, version::VersionInterval::all()]: () => (enabled: bool) (enabled: bool);
        list_devices [404, version::VersionInterval::all()]: (out_devices: sf::OutMapAliasBuffer<DeviceHandle>) => (count: u32) (count: u32);
        get_device_state [405, version::VersionInterval::all()]: (handle: DeviceHandle) => (state: u32) (state: u32);
        get_npad_id [406, version::VersionInterval::all()]: (handle: DeviceHandle) => (npad_id: u32) (npad_id: u32);
        attach_availability_change_event [407, version::VersionInterval::all()]: () => (event: sf::CopyHandle) (event: sf::CopyHandle);
        start_detection [408, version::VersionInterval::all()]: (handle: DeviceHandle, protocol: u32) => () ();
        stop_detection [409, version::VersionInterval::all()]: (handle: DeviceHandle) => () ();
        get_tag_info [410, version::VersionInterval::all()]: (handle: DeviceHandle, out_tag_info: sf::OutFixedPointerBuffer<[u8; 0x58]>) => () ();
        attach_activate_event [411, version::VersionInterval::all()]: (handle: DeviceHandle) => (event: sf::CopyHandle) (event: sf::CopyHandle);
        attach_deactivate_event [412, version::VersionInterval::all()]: (handle: DeviceHandle) => (event: sf::CopyHandle) (event: sf::CopyHandle);
        read_mifare [413, version::VersionInterval::all()]: (handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>, out_data: sf::OutMapAliasBuffer<u8>) => () ();
        write_mifare [414, version::VersionInterval::all()]: (handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>) => () ();
    }
}

ipc_sf_define_interface_trait! {
    trait NfcUserManager {
        create_user_interface [0, version::VersionInterval::all()]: () => (user: sf::MoveHandle) (user: impl INfcUserServer + 'static);
    }
}

// ── Implementation ───────────────────────────────────────────────────────────
pub struct NfcUserEmulator {
    info: sm::mitm::MitmProcessInfo,
}

impl INfcUserServer for NfcUserEmulator {
    // Legacy commands (pre 4.0.0)
    fn initialize_old(&mut self, _aruid: applet::AppletResourceUserId, _process_id: sf::ProcessId, _mcu_data: sf::InMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] NfcUser::InitializeOld\n", self.info.program_id.0);
        Ok(())
    }
    fn finalize_old(&mut self) -> Result<()> { Ok(()) }
    fn get_state_old(&mut self) -> Result<u32> { Ok(1) }
    fn is_nfc_enabled_old(&mut self) -> Result<bool> { Ok(true) }

    // Modern commands (4.0.0+)
    fn initialize(&mut self, _aruid: applet::AppletResourceUserId, _process_id: sf::ProcessId, _mcu_data: sf::InMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] NfcUser::Initialize\n", self.info.program_id.0);
        emu::register_intercepted_application_id(self.info.program_id);
        G_NFC_USER_DEVICE_STATE.store(0, Ordering::SeqCst);
        G_NFC_USER_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        Ok(())
    }

    fn finalize(&mut self) -> Result<()> {
        log!("[{:#X}] NfcUser::Finalize\n", self.info.program_id.0);
        G_NFC_USER_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        G_NFC_USER_DEVICE_STATE.store(0, Ordering::SeqCst);
        Ok(())
    }

    fn get_state(&mut self) -> Result<u32> { Ok(1) } // Initialized
    fn is_nfc_enabled(&mut self) -> Result<bool> { Ok(true) }

    fn list_devices(&mut self, out_devices: sf::OutMapAliasBuffer<DeviceHandle>) -> Result<u32> {
        // Return 1 device with NpadId = 0 (Player 1)
        if out_devices.get_size() >= 8 {
            unsafe { *(out_devices.get_address() as *mut u64) = 0u64; }
            Ok(1)
        } else {
            Ok(0)
        }
    }

    fn get_device_state(&mut self, _handle: DeviceHandle) -> Result<u32> {
        Ok(G_NFC_USER_DEVICE_STATE.load(Ordering::SeqCst))
    }

    fn get_npad_id(&mut self, _handle: DeviceHandle) -> Result<u32> {
        Ok(0) // NpadIdType::No1
    }

    fn attach_availability_change_event(&mut self) -> Result<sf::CopyHandle> {
        log!("[{:#X}] NfcUser::AttachAvailabilityChangeEvent\n", self.info.program_id.0);
        let guard = G_NFC_USER_AVAILABILITY_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }

    fn start_detection(&mut self, _handle: DeviceHandle, _protocol: u32) -> Result<()> {
        log!("[{:#X}] NfcUser::StartDetection (protocol={})\n", self.info.program_id.0, _protocol);
        G_NFC_USER_DETECTION_ACTIVE.store(true, Ordering::SeqCst);
        G_NFC_USER_DEVICE_STATE.store(1, Ordering::SeqCst); // SearchingForTag

        // If skylander already selected, signal immediately
        if emu::get_active_virtual_skylander().is_some() {
            log!("[{:#X}]   -> Skylander already loaded, signaling TagFound\n", self.info.program_id.0);
            G_NFC_USER_DEVICE_STATE.store(2, Ordering::SeqCst); // TagFound
            if let Some(event) = G_NFC_USER_ACTIVATE_EVENT.lock().as_ref() {
                let _ = event.signal();
            }
        }
        Ok(())
    }

    fn stop_detection(&mut self, _handle: DeviceHandle) -> Result<()> {
        log!("[{:#X}] NfcUser::StopDetection\n", self.info.program_id.0);
        G_NFC_USER_DETECTION_ACTIVE.store(false, Ordering::SeqCst);
        G_NFC_USER_DEVICE_STATE.store(0, Ordering::SeqCst);
        Ok(())
    }

    fn get_tag_info(&mut self, _handle: DeviceHandle, mut out_tag_info: sf::OutFixedPointerBuffer<[u8; 0x58]>) -> Result<()> {
        log!("[{:#X}] NfcUser::GetTagInfo\n", self.info.program_id.0);
        if let Some(ref sky) = *emu::get_active_virtual_skylander() {
            let tag_buf = out_tag_info.as_maybeuninit_mut()?;
            let mut info = [0u8; 0x58];
            // UID (first 4 bytes of the dump, standard for Mifare Classic 1K)
            let uid = &sky.data[..4];
            info[0] = uid[0]; info[1] = uid[1]; info[2] = uid[2]; info[3] = uid[3];
            // UUID length = 4
            info[8] = 4;
            // Protocol = 1 (NfcProtocol_TypeA / Mifare)
            info[9] = 1;
            // Tag type = 4 (Mifare Classic 1K)
            info[10] = 4;
            tag_buf[0].write(info);
        }
        Ok(())
    }

    fn attach_activate_event(&mut self, _handle: DeviceHandle) -> Result<sf::CopyHandle> {
        log!("[{:#X}] NfcUser::AttachActivateEvent\n", self.info.program_id.0);
        let guard = G_NFC_USER_ACTIVATE_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }

    fn attach_deactivate_event(&mut self, _handle: DeviceHandle) -> Result<sf::CopyHandle> {
        log!("[{:#X}] NfcUser::AttachDeactivateEvent\n", self.info.program_id.0);
        let guard = G_NFC_USER_DEACTIVATE_EVENT.lock();
        Ok(sf::Handle::from(guard.as_ref().unwrap().client_handle))
    }

    fn read_mifare(&mut self, _handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>, out_data: sf::OutMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] NfcUser::ReadMifare (in_size={}, out_size={})\n",
             self.info.program_id.0, in_params.get_size(), out_data.get_size());

        if let Some(ref sky) = *emu::get_active_virtual_skylander() {
            // MifareReadBlockParameter = { sector_key: MifareKey(6+1 = 7 bytes), pad, block_index: u8 } = ~10 bytes per entry
            // MifareReadBlockData = { data: [u8; 16], block_index: u8, pad } = ~18 bytes per entry
            let param_size = in_params.get_size();
            let out_size = out_data.get_size();

            if param_size > 0 && out_size > 0 {
                let param_ptr = in_params.get_address() as *const u8;
                let out_ptr = out_data.get_address() as *mut u8;

                // Each MifareReadBlockParameter is 16 bytes:
                //   [0..6]  = sector_key (6 bytes)
                //   [6]     = key_command (1 byte, 0x60=KeyA, 0x61=KeyB)
                //   [7]     = padding
                //   [8]     = block_index
                //   [9..16] = padding
                let entry_in_size: usize = 16;
                // Each MifareReadBlockData is 24 bytes:
                //   [0..16] = data (16 bytes)
                //   [16]    = block_index
                //   [17..24]= padding
                let entry_out_size: usize = 24;

                let count = param_size / entry_in_size;
                for i in 0..count {
                    if (i + 1) * entry_out_size > out_size {
                        break;
                    }
                    let param_entry = unsafe { core::slice::from_raw_parts(param_ptr.add(i * entry_in_size), entry_in_size) };
                    let block_index = param_entry[8] as usize;
                    let data_offset = block_index * 16;

                    log!("  ReadMifare: block={}\n", block_index);

                    let out_entry = unsafe { core::slice::from_raw_parts_mut(out_ptr.add(i * entry_out_size), entry_out_size) };
                    // Zero out
                    for b in out_entry.iter_mut() { *b = 0; }

                    // Copy 16 bytes from dump
                    if data_offset + 16 <= sky.data.len() {
                        out_entry[..16].copy_from_slice(&sky.data[data_offset..data_offset + 16]);
                    }
                    // Set block_index in response
                    out_entry[16] = block_index as u8;
                }
            }
        }
        Ok(())
    }

    fn write_mifare(&mut self, _handle: DeviceHandle, in_params: sf::InMapAliasBuffer<u8>) -> Result<()> {
        log!("[{:#X}] NfcUser::WriteMifare (in_size={})\n", self.info.program_id.0, in_params.get_size());
        // For now, accept writes silently
        Ok(())
    }
}

impl server::ISessionObject for NfcUserEmulator {
    fn try_handle_request_by_id(&mut self, id: u32, protocol: nx::ipc::CommandProtocol, server_ctx: &mut server::ServerContext) -> Option<Result<()>> {
        let result = INfcUserServer::try_handle_request_by_id(self, id, protocol, server_ctx);
        // Suppress high-frequency commands: GetState(402), IsNfcEnabled(403), GetDeviceState(405), ListDevices(404)
        if id != 402 && id != 403 && id != 404 && id != 405 {
            if result.is_none() {
                log!("[NfcUser] !! UNHANDLED CMD ID={} !!\n", id);
            } else {
                log!("[NfcUser] handled CMD ID={}\n", id);
            }
        }
        result
    }
}

// ── Manager ──────────────────────────────────────────────────────────────────
pub struct GenericUserManager {
    info: sm::mitm::MitmProcessInfo,
}

impl INfcUserManagerServer for GenericUserManager {
    fn create_user_interface(&mut self) -> Result<impl INfcUserServer + 'static> {
        log!("[{:#X}] NfcUserManager::CreateUserInterface\n", self.info.program_id.0);
        ensure_nfc_user_events()?;
        G_NFC_USER_DEVICE_STATE.store(0, Ordering::SeqCst);
        Ok(NfcUserEmulator { info: self.info.clone() })
    }
}

impl server::ISessionObject for GenericUserManager {
    fn try_handle_request_by_id(&mut self, id: u32, protocol: nx::ipc::CommandProtocol, server_ctx: &mut server::ServerContext) -> Option<Result<()>> {
        let result = INfcUserManagerServer::try_handle_request_by_id(self, id, protocol, server_ctx);
        if result.is_none() {
            log!("[NfcUserMgr] !! UNHANDLED CMD ID={} !!\n", id);
        }
        result
    }
}

impl server::IMitmServerObject for GenericUserManager {
    fn new(info: sm::mitm::MitmProcessInfo) -> Self { Self { info } }
}

impl server::IMitmService for GenericUserManager {
    fn get_name() -> sm::ServiceName { sm::ServiceName::new("nfc:user") }
    fn should_mitm(info: sm::mitm::MitmProcessInfo) -> bool {
        emu::record_mitm_request(info.program_id.0);
        emu::is_emulation_on()
    }
}
