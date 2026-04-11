#include <emu/emu_Service.hpp>

namespace emu {

    namespace {

        #define EMU_EMULANDERS_SERVICE_NAME "emulande"
        constexpr auto EmulandersServiceName = smEncodeName(EMU_EMULANDERS_SERVICE_NAME);

        Service g_EmulandersService;

        inline bool smAtmosphereHasService(const SmServiceName name) {
            auto has = false;
            tipcDispatchInOut(smGetServiceSessionTipc(), 65100, name, has);
            return has;
        }

    }

    bool IsAvailable() {
        return smAtmosphereHasService(EmulandersServiceName);
    }

    Result Initialize() {
        if(serviceIsActive(&g_EmulandersService)) {
            return 0;
        }
        return smGetService(&g_EmulandersService, EMU_EMULANDERS_SERVICE_NAME);
    }

    void Exit() {
        serviceClose(&g_EmulandersService);
    }

    Version GetVersion() {
        Version ver = {};
        serviceDispatchOut(&g_EmulandersService, 0, ver);
        return ver;
    }

    EmulationStatus GetEmulationStatus() {
        EmulationStatus status = EmulationStatus::Off;
        serviceDispatchOut(&g_EmulandersService, 1, status);
        return status;
    }

    void SetEmulationStatus(const EmulationStatus status) {
        serviceDispatchIn(&g_EmulandersService, 2, status);
    }

    void GetActiveVirtualSkylander(char *out_path, const size_t out_path_size) {
        serviceDispatch(&g_EmulandersService, 3,
            .buffer_attrs = {
                SfBufferAttr_HipcMapAlias | SfBufferAttr_Out
            },
            .buffers = {
                { out_path, out_path_size }
            },
        );
    }

    Result SetActiveVirtualSkylander(const char *path, const size_t path_size) {
        return serviceDispatch(&g_EmulandersService, 4,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
            .buffers = { { path, path_size } },
        );
    }

    void ResetActiveVirtualSkylander() {
        serviceDispatch(&g_EmulandersService, 5);
    }

    VirtualSkylanderStatus GetActiveVirtualSkylanderStatus() {
        VirtualSkylanderStatus status = VirtualSkylanderStatus::Invalid;
        serviceDispatchOut(&g_EmulandersService, 6, status);
        return status;
    }

    void SetActiveVirtualSkylanderStatus(const VirtualSkylanderStatus status) {
        serviceDispatchIn(&g_EmulandersService, 7, status);
    }

    bool IsApplicationIdIntercepted(const u64 app_id) {
        bool intercepted;
        serviceDispatchInOut(&g_EmulandersService, 8, app_id, intercepted);
        return intercepted;
    }

    Result GetLastMitmRequestId(u64 *out_id) {
        return serviceDispatchOut(&g_EmulandersService, 9, *out_id);
    }

    Result GetDebugLog(char *out_log, size_t log_size) {
        return serviceDispatch(&g_EmulandersService, 10,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { out_log, log_size } },
        );
    }

    bool GetLoggingStatus() {
        bool status = false;
        serviceDispatchOut(&g_EmulandersService, 11, status);
        return status;
    }

    void SetLoggingStatus(bool status) {
        serviceDispatchIn(&g_EmulandersService, 12, status);
    }

    void ClearDebugLog() {
        serviceDispatch(&g_EmulandersService, 13);
    }

}