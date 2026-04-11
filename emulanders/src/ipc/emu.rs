use alloc::string::ToString;
use nx::ipc::sf::ncm;
use nx::ipc::sf::sm;
use nx::result::*;
use nx::ipc::sf;
use nx::ipc::server;
use nx::service;
use nx::version;
use crate::emu;

ipc_sf_define_default_client_for_interface!(EmulandersService);
ipc_sf_define_interface_trait! {
    trait EmulandersService {
        get_version [0, version::VersionInterval::all()]: () => (version: emu::Version) (version: emu::Version);
        get_emulation_status [1, version::VersionInterval::all()]: () => (status: emu::EmulationStatus) (status: emu::EmulationStatus);
        set_emulation_status [2, version::VersionInterval::all()]: (status: emu::EmulationStatus) => () ();
        get_active_virtual_skylander [3, version::VersionInterval::all()]: (out_path: sf::OutMapAliasBuffer<u8>) => () ();
        set_active_virtual_skylander [4, version::VersionInterval::all()]: (path: sf::InMapAliasBuffer<u8>) => () ();
        reset_active_virtual_skylander [5, version::VersionInterval::all()]: () => () ();
        get_active_virtual_skylander_status [6, version::VersionInterval::all()]: () => (status: emu::VirtualSkylanderStatus) (status: emu::VirtualSkylanderStatus);
        set_active_virtual_skylander_status [7, version::VersionInterval::all()]: (status: emu::VirtualSkylanderStatus) => () ();
        is_application_id_intercepted [8, version::VersionInterval::all()]: (application_id: ncm::ProgramId) => (is_intercepted: bool) (is_intercepted: bool);
        get_last_mitm_request_id [9, version::VersionInterval::all()]: () => (id: u64) (id: u64);
        get_debug_log [10, version::VersionInterval::all()]: (out_log: sf::OutMapAliasBuffer<u8>) => () ();
        get_logging_status [11, version::VersionInterval::all()]: () => (status: bool) (status: bool);
        set_logging_status [12, version::VersionInterval::all()]: (status: bool) => () ();
        clear_debug_log [13, version::VersionInterval::all()]: () => () ();
    }
}

pub struct EmulandersServer;

impl IEmulandersServiceServer for EmulandersServer {
    fn get_version(&mut self) -> Result<emu::Version> {
        log!("GetVersion -- (...)\n");
        Ok(emu::CURRENT_VERSION)
    }

    fn get_emulation_status(&mut self) -> Result<emu::EmulationStatus> {
        let status = emu::get_emulation_status();
        Ok(status)
    }

    fn set_emulation_status(&mut self, status: emu::EmulationStatus) -> Result<()> {
        log!("SetEmulationStatus -- status: {:?}\n", status);
        emu::set_emulation_status(status);
        Ok(())
    }

    fn get_active_virtual_skylander(&mut self, mut out_path: sf::OutMapAliasBuffer<u8>) -> Result<()> {
        log!("GetActiveVirtualSkylander -- (...)\n");
        if let Some(s) = emu::get_active_virtual_skylander().as_ref() {
            out_path.set_string(s.path.clone());
        } else {
            out_path.set_string(alloc::string::String::new());
        }
        Ok(())
    }

    fn set_active_virtual_skylander(&mut self, path_buf: sf::InMapAliasBuffer<u8>) -> Result<()> {
        let path = unsafe { core::str::from_utf8_unchecked(core::slice::from_raw_parts(path_buf.get_address(), path_buf.get_size())) };
        log!("SetActiveVirtualSkylander -- path: '{}'\n", path);
        let skylander = crate::skylander::Skylander::load(path.to_string())?;
        emu::set_active_virtual_skylander(Some(skylander));
        Ok(())
    }

    fn reset_active_virtual_skylander(&mut self) -> Result<()> {
        log!("ResetActiveVirtualSkylander -- (...)\n");
        emu::set_active_virtual_skylander(None);
        Ok(())
    }

    fn get_active_virtual_skylander_status(&mut self) -> Result<emu::VirtualSkylanderStatus> {
        let status = emu::get_active_virtual_skylander_status();
        Ok(status)
    }

    fn set_active_virtual_skylander_status(&mut self, status: emu::VirtualSkylanderStatus) -> Result<()> {
        log!("SetActiveVirtualSkylanderStatus -- status: {:?}\n", status);
        emu::set_active_virtual_skylander_status(status);
        Ok(())
    }

    fn is_application_id_intercepted(&mut self, application_id: ncm::ProgramId) -> Result<bool> {
        Ok(emu::is_application_id_intercepted(application_id))
    }

    fn get_last_mitm_request_id(&mut self) -> Result<u64> {
        Ok(emu::get_last_mitm_request_id())
    }

    fn get_debug_log(&mut self, mut out_log: sf::OutMapAliasBuffer<u8>) -> Result<()> {
        out_log.set_string(emu::get_debug_log());
        Ok(())
    }

    fn get_logging_status(&mut self) -> Result<bool> {
        Ok(emu::get_logging_status())
    }

    fn set_logging_status(&mut self, status: bool) -> Result<()> {
        emu::set_logging_status(status);
        Ok(())
    }

    fn clear_debug_log(&mut self) -> Result<()> {
        emu::clear_debug_log();
        Ok(())
    }
}

impl server::ISessionObject for EmulandersServer {
    fn try_handle_request_by_id(&mut self, req_id: u32, protocol: nx::ipc::CommandProtocol, server_ctx: &mut server::ServerContext) -> Option<Result<()>> {
        <Self as IEmulandersServiceServer>::try_handle_request_by_id(self, req_id, protocol, server_ctx)
    }
}

impl server::IServerObject for EmulandersServer {
    fn new() -> Self {
        Self
    }
}

impl server::IService for EmulandersServer {
    fn get_name() -> sm::ServiceName {
        sm::ServiceName::new("emulande")
    }

    fn get_max_sesssions() -> i32 {
        20
    }
}

impl service::IService for EmulandersService {
    fn get_name() -> sm::ServiceName {
        sm::ServiceName::new("emulande")
    }

    fn as_domain() -> bool {
        false
    }

    fn post_initialize(&mut self) -> Result<()> {Ok(())
    }
}