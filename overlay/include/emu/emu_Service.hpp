#pragma once
#include <switch.h>
#include <cstring>

namespace emu {

    enum class EmulationStatus : u32 {
        On,
        Off,
    };

    enum class VirtualSkylanderStatus : u32 {
        Invalid,
        Connected,
        Disconnected
    };

    struct Version {
        u8 major;
        u8 minor;
        u8 micro;
        bool dev_build;

        inline constexpr bool EqualsExceptBuild(const Version &other) {
            return (other.major == this->major) && (other.minor == this->minor) && (other.micro == this->micro);
        }
    };

    bool IsAvailable();

    Result Initialize();
    void Exit();

    Version GetVersion();

    EmulationStatus GetEmulationStatus();
    void SetEmulationStatus(const EmulationStatus status);

    void GetActiveVirtualSkylander(char *out_path, const size_t out_path_size);
    Result SetActiveVirtualSkylander(const char *path, const size_t path_size);
    void ResetActiveVirtualSkylander();

    VirtualSkylanderStatus GetActiveVirtualSkylanderStatus();
    void SetActiveVirtualSkylanderStatus(const VirtualSkylanderStatus status);

    bool IsApplicationIdIntercepted(const u64 app_id);

    inline bool IsCurrentApplicationIdIntercepted() {
        bool intercepted = false;
        u64 process_id = 0;
        if(R_SUCCEEDED(pmdmntGetApplicationProcessId(&process_id))) {
            u64 program_id = 0;
            if(R_SUCCEEDED(pmdmntGetProgramId(&program_id, process_id))) {
                intercepted = IsApplicationIdIntercepted(program_id);
            }
        }
        return intercepted;
    }

    Result GetLastMitmRequestId(u64 *out_id);
    Result GetDebugLog(char *out_log, size_t log_size);
    
    bool GetLoggingStatus();
    void SetLoggingStatus(bool status);
    void ClearDebugLog();

}