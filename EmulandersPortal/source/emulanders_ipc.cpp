#include "emulanders_ipc.h"
#include <string.h>

static Service g_emulandersSrv;
static int g_emulandersRefCnt = 0;

Result emulandersInitialize() {
    if (g_emulandersRefCnt++ > 0) return 0;
    
    Result rc = smGetService(&g_emulandersSrv, "emulande");
    if (R_FAILED(rc)) {
        g_emulandersRefCnt--;
    }
    return rc;
}

void emulandersExit() {
    if (--g_emulandersRefCnt == 0) {
        serviceClose(&g_emulandersSrv);
    }
}

bool emulandersIsInitialized() {
    return g_emulandersRefCnt > 0;
}

Result emulandersGetEmulationStatus(EmulationStatus* out_status) {
    if (!emulandersIsInitialized()) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    u32 out = 0;
    Result rc = serviceDispatchOut(&g_emulandersSrv, 1, out);
    if (R_SUCCEEDED(rc) && out_status) {
        *out_status = static_cast<EmulationStatus>(out);
    }
    return rc;
}

Result emulandersSetEmulationStatus(EmulationStatus status) {
    if (!emulandersIsInitialized()) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    u32 in = static_cast<u32>(status);
    return serviceDispatchIn(&g_emulandersSrv, 2, in);
}

Result emulandersGetActiveSkylander(std::string& out_path) {
    if (!emulandersIsInitialized()) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    char path_buf[512] = {0};
    Result rc = serviceDispatch(&g_emulandersSrv, 3,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { path_buf, sizeof(path_buf) } }
    );
    
    if (R_SUCCEEDED(rc)) {
        path_buf[511] = '\0'; // Guarantee null termination
        out_path = std::string(path_buf);
    }
    return rc;
}

Result emulandersSetActiveSkylander(const std::string& path) {
    if (!emulandersIsInitialized()) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return serviceDispatch(&g_emulandersSrv, 4,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { path.c_str(), path.length() } }
    );
}

Result emulandersResetActiveSkylander() {
    if (!emulandersIsInitialized()) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return serviceDispatch(&g_emulandersSrv, 5);
}
