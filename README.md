<div align="center">

# WindowsAudioPlugin

Make Windows volume control and wake-up experience smarter.  
This project contains:

- A statically-linked MFC tray application that runs in the user session.
- A Detours-based DLL injected into `explorer.exe` via a global Windows hook.

</div>

---

## Project Goals

- Fix Windows’ default master volume step being effectively fixed at 2% by providing a configurable step (percent-based) through an Explorer hook.
- On resume from sleep/hibernation, play a short sound (and temporarily raise volume if too low) to wake external speakers/amps with auto‑standby (e.g., Genelec 8020, Neumann KH 80 DSP, Focal Alpha 50 Evo).

---

## ✨ Features

- **Configurable volume step (Explorer hook)**
  - Injects a small DLL into `explorer.exe` using `SetWindowsHookEx` (`WH_GETMESSAGE`) purely as an injection vehicle (the hook procedure itself does not modify messages).
  - Inside `explorer.exe`, the DLL uses Microsoft Detours to hook `IAudioEndpointVolume::VolumeStepUp/VolumeStepDown`.
  - The step size is configurable in percent via `MMKV` key `hook_step_percent` (range `1..20`, default `1`).  
    When set to `2`, the hook defers to the original OS implementation (so volume step behaves exactly like Windows’ default).

- **Wake-up actions for auto-standby speakers**
  - On resume from sleep/hibernation the app immediately schedules a wake task that:
    1. Ensures the master volume meets a configurable minimum (default 8%) before playback.
    2. Plays a user-selected MP3 for a configurable duration (default 3 seconds) via `ffplay.exe` to wake auto-standby speakers.
    3. Restores the original volume and mute state once playback completes.

- **Tray UI (MFC)**
  - System tray icon with a context menu:
    - “Enable” (toggle main features on/off, shows checkmark when enabled)
    - “Settings…”
    - “Exit”
  - Left-click or double-click tray icon → opens “Settings…”
  - Right-click tray icon → shows context menu

- **Settings (MMKV-backed)**
  - Run at startup (creates/removes a `.lnk` shortcut in the user’s `Startup` folder)
  - MP3 file path to play on resume
  - Enable/disable wake actions (default: enabled)
  - Enable/disable Explorer hook (default: disabled)
  - Volume step percent (1–20, default: 1)
  - Wake minimum volume percent (1–100, default: 8)
  - All settings are stored via **[MMKV](https://github.com/Tencent/MMKV)** under `%APPDATA%\WindowsAudioPlugin\config.mmkv` (multi-process mode is used by the hook).

---

## 🧩 Architecture

```
┌───────────────────────────────────────────────────────────┐
│  WindowsAudioPlugin.exe (MFC tray app, static /MT)        │
│  • Tray: Enable / Settings… / Exit                         │
│  • Power: WM_POWERBROADCAST + WM_WTSSESSION_CHANGE         │
│      – ensure ≥ configured min (default 8%)                │
│      – play MP3 via ffplay                                 │
│      – restore volume & mute                               │
│  • Settings persisted via MMKV (multi-process)             │
└───────────────┬────────────────────────────────────────────┘
                │ installs WH_GETMESSAGE global hook (inject)
                ▼
┌───────────────────────────────────────────────────────────┐
│  AudioHook64.dll (Detours-based, no MFC, in explorer.exe) │
│  • DllMain: only proceed if process == explorer.exe and    │
│             audioses.dll is loaded                         │
│  • Init:                                                   │
│      – initialize MMKV (multi-process) at %APPDATA%        │
│      – compute IAudioEndpointVolume vtable offsets         │
│      – Detours attach VolumeStepUp/Down                    │
│  • On VolumeStepUp/Down:                                   │
│      – read hook_step_percent from MMKV                    │
│      – if == 2 → call original (native Windows behavior)   │
│      – else → vol = clamp(vol ± percent/100.0)             │
└───────────────────────────────────────────────────────────┘
```

Flow (high level):
- Explorer hook: main app toggles injection; DLL attaches in `explorer.exe` and patches `IAudioEndpointVolume` steps.
- Settings: tray app writes MMKV (single-process); hook DLL reads MMKV (multi-process) for `hook_step_percent`.
- Resume actions: tray app responds to resume, bumps volume to at least configured min, plays MP3 3s, then restores state.
- A dedicated hidden window permanently registers for `WM_WTSSESSION_CHANGE`, so unlock/logon events also trigger the wake sequence without extra plumbing.

Notes:
- The global hook type is `WH_GETMESSAGE` and the hook procedure forwards all messages (it does not modify them). It is used solely to ensure the DLL is mapped into `explorer.exe`.
- The actual audio adjustments happen through COM’s `IAudioEndpointVolume` inside the target process.

---

## 🛠️ Build

### Prerequisites
 - Windows 7 or later
 - Visual Studio 2022 with C++ (MSVC)
 - CMake ≥ 3.20
 - Detours binaries (this repository ships the prebuilt libraries)
 - `ffplay.exe` is included under `tools/ffplay.exe` (32-bit, includes only common audio decoders; build configuration see `tools/readme.txt`). It will be automatically copied next to the app during the build.

### Build with script (x64 only)
```bat
:: From repository root
windows.bat
```
This script configures and builds x64 into:
```
build/
  win64/
    bin/  (WindowsAudioPlugin.exe, AudioHook64.dll)
    lib/
```
Both the MFC runtime and the C/C++ runtime are statically linked (`/MT` in Release, `/MTd` in Debug).

### Manual CMake
```bat
:: x64
mkdir build\win64
cd build\win64
cmake -A x64 ..\..
cmake --build . --config Release
```

---

## ▶️ Run
1. Ensure `ffplay.exe` is available in `tools/` before building (it will be copied to the output `bin` beside `WindowsAudioPlugin.exe`).
2. Launch `WindowsAudioPlugin.exe`. A tray icon will appear.
3. Right-click the tray icon → “Settings…” to configure:
   - (Optional) “Run at startup”
   - “Enable wake actions” (default: on)
   - “Enable hook” (default: off)
   - “Hook step (%)” (1–20, default: 1; 2 means native OS step)
   - “MP3 file” to play on resume (3 seconds via ffplay)
4. Toggle “Enable” in the tray menu to activate/deactivate all features at once.

> The Explorer hook is only armed when “Enable” is on and “Enable hook” is on.  
> Injection uses a `WH_GETMESSAGE` global hook; the hook procedure is a no-op except to trigger DLL mapping.

---

## ⚙️ Configuration Storage (MMKV)

- All settings are stored via **MMKV** at:
  ```
  %APPDATA%\WindowsAudioPlugin\config.mmkv
  ```
- Both the tray app and the injected DLL use `MMKV_MULTI_PROCESS` for shared access.
- Relevant keys:
  - `enabled` (`bool`)
  - `run_at_startup` (`bool`)
  - `wake_actions_enabled` (`bool`)
  - `hook_enabled` (`bool`)
  - `hook_step_percent` (`int`, 1..20, default 1)
  - `mp3_path` (`string`, UTF‑8)

---

## 🔊 Logging & Diagnostics

- The tray app emits debug strings via `OutputDebugString`. You can view them with tools like **DebugView** or Visual Studio’s Output window.
- Power resume events are observed via `WM_POWERBROADCAST` (`PBT_APMRESUMEAUTOMATIC`).

> If you plan to add file-based logging, a natural location is `%APPDATA%\WindowsAudioPlugin\log.txt`.

---

## 🔒 Security & Compatibility Notes

- Injecting into `explorer.exe` requires that your DLL is trusted by the system and that the caller has sufficient permissions.
- Low-level keyboard/mouse hooks (`WH_KEYBOARD_LL`/`WH_MOUSE_LL`) do not load your DLL into other processes. For process-local hooking (like the audio endpoint hack), the message-based hook (`WH_GETMESSAGE`/`WH_CALLWNDPROC`) is a common approach.
- This project is designed for Windows 7+ (where `IAudioEndpointVolume` is available). On very old systems or locked-down environments (e.g., some enterprise policies), injection or Detours may be restricted.

---

## 🧪 Development Tips
- Use `DebugView` to watch `OutputDebugString` traces (e.g., power resume events, step operations).
- If you change the hook DLL, restart Explorer (or reinstall the hook from the tray app) to reload the new DLL into `explorer.exe`.
- When testing `ffplay`, verify the file path in Settings and that `ffplay.exe` is present next to the main executable.

---

## 📜 License

MIT License. See `LICENSE` for details.  
Note: Microsoft Detours is licensed under its own terms; ensure you comply with its license when distributing.

---

## 🙌 Acknowledgements
- [Microsoft Detours](https://github.com/microsoft/Detours)
- [MMKV](https://github.com/Tencent/MMKV)
- [FFmpeg / ffplay](https://ffmpeg.org/)

---



## ❓ FAQ

**Q: Why use `WH_GETMESSAGE` if we don’t need to process messages?**  
A: It’s a stable way to have the system load our DLL into `explorer.exe`. The hook proc is a no-op; once mapped, the DLL’s `DllMain` + `InitAudioHook()` arrange the Detours-based function hooks.

**Q: Does this project handle “all processes’ volume adjustment paths”?**  
A: No. This project focuses on fixing Windows’ default volume step (which is effectively 2%) by adjusting the shell’s `IAudioEndpointVolume` step via a hook in `explorer.exe`. It does not aim to capture or rewrite every application’s custom volume logic across all processes.


