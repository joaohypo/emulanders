#pragma once

#include <switch.h>
#include <string>

enum class EmulationStatus : u32 {
    On = 0,
    Off = 1
};

Result emulandersInitialize();
void emulandersExit();
bool emulandersIsInitialized();

Result emulandersGetEmulationStatus(EmulationStatus* out_status);
Result emulandersSetEmulationStatus(EmulationStatus status);

Result emulandersGetActiveSkylander(std::string& out_path);
Result emulandersSetActiveSkylander(const std::string& path);
Result emulandersResetActiveSkylander();
