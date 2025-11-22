#pragma once

void InitAudioHook();
void UninitAudioHook();

// Exported install/uninstall wrapper inside DLL (implemented in AudioHook.cpp, exported via .def)
extern "C" void HookInstall();
extern "C" void HookUninstall();

