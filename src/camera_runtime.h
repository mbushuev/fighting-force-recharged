#pragma once
#include "orbit_camera.h"
#include "mouse_combat.h"
#include "modern_input_runtime.h"
#include "camera_diagnostics.h"

static ff_camera::Settings g_camera_settings;
static ff_camera::Orbit g_camera_orbit;
static bool g_camera_active, g_camera_mouse_ready, g_camera_reset_button;
static IDirectInputA* g_camera_input;
static IDirectInputDeviceA* g_camera_mouse;
static HWND g_camera_window;
static LARGE_INTEGER g_camera_time, g_camera_frequency;
static unsigned g_camera_samples;
static bool g_free_orbit, g_mouse_combat;

template <typename T> static T& CameraField(DWORD rva) { return *reinterpret_cast<T*>(g_base + rva); }
static bool CameraMayOwnPose() {
    return ff_camera::CanOrbit(CameraField<int>(0x85A6B8), CameraField<int>(0x8745A0),
        CameraField<int>(0x85A668), CameraField<int16_t>(0x86A542), CameraField<unsigned>(0x85B560));
}
static bool CameraHasFocus() {
    return ModernHasFocus();
}
static void ReleaseCameraMouse() {
    if (g_camera_mouse) { g_camera_mouse->Unacquire(); g_camera_mouse->Release(); g_camera_mouse = nullptr; }
    if (g_camera_input) { g_camera_input->Release(); g_camera_input = nullptr; }
    g_camera_window = nullptr; g_camera_mouse_ready = false;
}
static bool PrepareCameraMouse() {
    static ULONGLONG retry_after;
    if (GetTickCount64() < retry_after) return false;
    HWND window = CameraField<HWND>(0x1EC4E4);
    if (!window || !IsWindow(window)) return false;
    if (window != g_camera_window) ReleaseCameraMouse();
    if (g_camera_mouse) return true;
    // Use a separate, nonexclusive relative device. This does not change the
    // game's keyboard/joystick formats or warp the system pointer.
    using Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, IDirectInputA**, IUnknown*);
    auto create = reinterpret_cast<Create>(SystemFunction("DirectInputCreateA"));
    HRESULT result = create ? create(GetModuleHandleW(nullptr), 0x500, &g_camera_input, nullptr) : E_FAIL;
    if (SUCCEEDED(result)) result = g_camera_input->CreateDevice(GUID_SysMouse, &g_camera_mouse, nullptr);
    if (SUCCEEDED(result)) result = g_camera_mouse->SetDataFormat(&c_dfDIMouse);
    if (SUCCEEDED(result)) result = g_camera_mouse->SetCooperativeLevel(window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(result)) {
        char message[96]; sprintf_s(message, "Camera mouse initialization failed: %08lx.\r\n", result); Log(message);
        ReleaseCameraMouse(); retry_after = GetTickCount64() + 2000; return false;
    }
    g_camera_window = window; Log("Independent relative camera mouse initialized.\r\n"); return true;
}
static DIMOUSESTATE ReadCameraMouse(bool foreground) {
    DIMOUSESTATE mouse = {};
    if (!foreground) {
        if (g_camera_mouse_ready && g_camera_mouse) g_camera_mouse->Unacquire();
        g_camera_mouse_ready = false; return mouse;
    }
    if (!PrepareCameraMouse()) return mouse;
    HRESULT result = g_camera_mouse->GetDeviceState(sizeof(mouse), &mouse);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        g_camera_mouse_ready = false;
        if (SUCCEEDED(g_camera_mouse->Acquire())) result = g_camera_mouse->GetDeviceState(sizeof(mouse), &mouse);
    }
    if (FAILED(result)) { g_camera_mouse_ready = false; return {}; }
    // Discard movement accumulated in menus, during loading or focus changes.
    if (!g_camera_mouse_ready) { g_camera_mouse_ready = true; return {}; }
    return mouse;
}
static void GameFocusLost() {
    ReadCameraMouse(false);g_camera_time={};g_camera_reset_button=false;
    g_modern_pad_session.ready=false;g_modern_pad_session.last_poll=0;
    g_modern_confirm.Begin(true,true);g_modern_pause.held=true;g_modern_pause_accept_blocked=true;
    g_modern_quit_button=true;ResetAnalogMotion();
}
#include "camera_geometry.h"
static void PublishOrbitPose(double elapsed=0) {
    const ff_camera::Point player = {CameraField<int32_t>(0x86A528) / 65536.0,
        CameraField<int32_t>(0x86A52C) / 65536.0, CameraField<int32_t>(0x86A530) / 65536.0};
    const auto desired=g_camera_orbit.Follow(player,g_camera_settings);
    const ff_camera::Point focus={player.x,player.y-g_camera_settings.height,player.z};
    const double allowed=CameraObstacleDistance(focus,{desired.x/65536.0,desired.y/65536.0,desired.z/65536.0});
    g_camera_boom_response.distance=g_camera_boom;
    g_camera_boom=g_camera_boom_response.Update(allowed,g_camera_orbit.distance,elapsed);
    auto constrained=g_camera_orbit;constrained.distance=g_camera_boom;
    const auto pose = constrained.Follow(player, g_camera_settings);
    CameraField<int32_t>(0x866220) = CameraField<int32_t>(0x86D31C) = pose.x;
    CameraField<int32_t>(0x866224) = CameraField<int32_t>(0x86D324) = pose.y;
    CameraField<int32_t>(0x866228) = CameraField<int32_t>(0x86D328) = pose.z;
    CameraField<int16_t>(0x87888C) = CameraField<int16_t>(0x86D318) = pose.yaw;
    CameraField<int16_t>(0x87888E) = CameraField<int16_t>(0x874CF2) = pose.pitch;
    CameraField<int32_t>(0x8744A8) = CameraField<int32_t>(0x8651A4) = CameraField<int32_t>(0x8689F8) = 0;
    CameraField<int32_t>(0x866214) = pose.y >> 12;
    CameraField<int32_t>(0x878708) = static_cast<int32_t>(-static_cast<int64_t>(pose.x) >> 12);
    CameraField<int32_t>(0x86BF0C) = static_cast<int32_t>(-static_cast<int64_t>(pose.z) >> 12);
    // Render copies are deliberately left to the native frame publication.
    // Writing them here could mix poses in the optional rendering thread.
}
static void __cdecl GameCameraBegin(int value) {
    g_camera_active = false; g_camera_reset_button = false; g_camera_time = {};
    ClearCameraGeometry();
    ReleaseCameraMouse();
    reinterpret_cast<void(__cdecl*)(int)>(g_base + 0x22BC0)(value);
}
static void __cdecl GameCameraInput() {
    reinterpret_cast<void(__cdecl*)()>(g_base + 0x484F0)();
    CameraField<uint32_t>(0x5523E0)=ModernGameplayActions(CameraField<uint32_t>(0x5523E0));
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    double dt = g_camera_time.QuadPart ? static_cast<double>(now.QuadPart - g_camera_time.QuadPart) / g_camera_frequency.QuadPart : 0;
    g_camera_time = now;
    if(g_camera_trace && g_camera_active && dt>0.075) {
        char message[96];sprintf_s(message,"Camera input interval: %.3f ms.\r\n",dt*1000);Log(message);
    }
    if (!CameraMayOwnPose()) {
        g_camera_active = false; g_camera_reset_button = false; ReadCameraMouse(false); return;
    }
    if (g_free_orbit && !g_camera_active) {
        g_camera_orbit.Begin(CameraField<int16_t>(0x87888C), g_camera_settings);
        g_camera_active = true; g_camera_samples = 0; g_camera_mouse_ready = false;
        Log("Independent single-player orbit owns the gameplay camera.\r\n");
    }
    const bool foreground = CameraHasFocus();
    DIMOUSESTATE mouse = ReadCameraMouse(foreground);
    // 0x4484f0 has populated this update's native player-one action word.
    // Merge mouse actions before actor processing, retaining physical keyboard
    // and joystick actions. The next native poll naturally clears released keys.
    CameraField<uint32_t>(0x5523E0) = MergeMouseCombat(CameraField<uint32_t>(0x5523E0),
        (mouse.rgbButtons[0] & 0x80) != 0, (mouse.rgbButtons[1] & 0x80) != 0, g_mouse_combat && foreground);
    if (!g_free_orbit) return;
    const auto pad=ReadModernPad();
    bool reset = foreground && ((mouse.rgbButtons[2] & 0x80) || (pad.buttons & ff_input::RightThumb));
    if (reset && !g_camera_reset_button)
        g_camera_orbit.Begin(ff_camera::BehindActorYaw(CameraField<int16_t>(0x86A538)), g_camera_settings);
    g_camera_reset_button = reset;
    const auto stick=ff_input::Stick(pad.rx,pad.ry,g_modern_right_deadzone);
    const double zoom=(pad.rt>30 ? (pad.rt-30)/225.0:0)-(pad.lt>30 ? (pad.lt-30)/225.0:0);
    if (foreground) g_camera_orbit.Look(mouse.lX, mouse.lY,stick.x,stick.y, dt,
        mouse.lZ / 120.0+zoom*ff_camera::Clamp(dt,0,0.1)*g_camera_settings.trigger_zoom_speed/60,g_camera_settings);
    PublishOrbitPose(dt);
    // The next native instructions save this exact yaw for character heading;
    // visibility preparation and actor updates then consume the same pose.
}
static void __cdecl GameCameraStage() {
    const int stage = CameraField<int>(0x85A6B8);
    auto table = reinterpret_cast<void(__cdecl**)()>(g_base + 0x8736E0);
    table[stage]();
    // Preserve every original stage call and its progression/trigger effects.
    // Only its pose result is replaced, after actors have moved this update.
    if (g_camera_active && CameraMayOwnPose()) {
        PublishOrbitPose();
        if (g_camera_trace && (++g_camera_samples % 120) == 1) {
            char message[256]; sprintf_s(message,
                "Orbit stage %d tick %u: yaw %.2f pitch %.2f distance %.1f boom %.1f; pose %ld,%ld,%ld; input yaw %d.\r\n",
                stage, CameraField<unsigned>(0x85B560), g_camera_orbit.yaw * 180 / ff_camera::kPi,
                g_camera_orbit.pitch * 180 / ff_camera::kPi, g_camera_orbit.distance, g_camera_boom,
                CameraField<int32_t>(0x866220), CameraField<int32_t>(0x866224), CameraField<int32_t>(0x866228),
                CameraField<int16_t>(0x85A68C)); Log(message);
            ReportCameraTimings();
        }
    }
}
static int CameraSetting(const wchar_t* key, int fallback, int lo, int hi) {
    return static_cast<int>(ff_camera::Clamp(static_cast<int>(GetPrivateProfileIntW(L"Camera", key, fallback, g_config_path)), lo, hi));
}
static void InstallCameraHooks() {
    g_free_orbit = GetPrivateProfileIntW(L"Camera", L"FreeOrbit", 1, g_config_path) != 0;
    g_mouse_combat = GetPrivateProfileIntW(L"Input", L"MouseCombat", 1, g_config_path) != 0;
    if (!g_free_orbit && !g_mouse_combat && !g_modern_keyboard && !g_modern_gamepad) return;
    struct Call { DWORD rva, target; void* replacement; };
    const Call calls[] = {{0x40D6C, 0x22BC0, reinterpret_cast<void*>(GameCameraBegin)},
                         {0x41177, 0x484F0, reinterpret_cast<void*>(GameCameraInput)}};
    for (const auto& call : calls) {
        const auto code = reinterpret_cast<BYTE*>(g_base + call.rva); LONG relative; memcpy(&relative, code + 1, 4);
        if (code[0] != 0xE8 || reinterpret_cast<uintptr_t>(code + 5) + relative != g_base + call.target) {
            Log("Camera call signature differs; native camera retained.\r\n"); return;
        }
    }
    const BYTE expected[] = {0xFF, 0x14, 0x95, 0xE0, 0x36, 0xC7, 0x00};
    auto dispatch = reinterpret_cast<BYTE*>(g_base + 0x4120F);
    if (memcmp(dispatch, expected, sizeof(expected))) { Log("Camera dispatch signature differs; native camera retained.\r\n"); return; }
    auto first = reinterpret_cast<BYTE*>(g_base + calls[0].rva);
    const SIZE_T length = dispatch + sizeof(expected) - first; DWORD previous;
    if (!VirtualProtect(first, length, PAGE_EXECUTE_READWRITE, &previous)) return;
    for (const auto& call : calls) {
        auto code = reinterpret_cast<BYTE*>(g_base + call.rva);
        LONG relative = static_cast<LONG>(reinterpret_cast<BYTE*>(call.replacement) - (code + 5)); memcpy(code + 1, &relative, 4);
    }
    dispatch[0] = 0xE8; LONG relative = static_cast<LONG>(reinterpret_cast<BYTE*>(GameCameraStage) - (dispatch + 5));
    memcpy(dispatch + 1, &relative, 4); dispatch[5] = dispatch[6] = 0x90;
    DWORD ignored; VirtualProtect(first, length, previous, &ignored); FlushInstructionCache(GetCurrentProcess(), first, length);
    g_camera_settings.distance = CameraSetting(L"Distance", 720, 240, 1200);
    g_camera_settings.height = CameraSetting(L"TargetHeight", 160, 128, 240);
    g_camera_settings.ground_clearance = CameraSetting(L"GroundClearance", 128, 32, 128);
    g_camera_settings.mouse_x = CameraSetting(L"MouseHorizontalMilliDegrees", 150, 20, 1000) * ff_camera::kPi / 180000;
    g_camera_settings.mouse_y = CameraSetting(L"MouseVerticalMilliDegrees", 120, 20, 1000) * ff_camera::kPi / 180000;
    g_camera_settings.stick_speed = CameraSetting(L"StickDegreesPerSecond", 120, 20, 360) * ff_camera::kPi / 180;
    g_camera_settings.trigger_zoom_speed = CameraSetting(L"TriggerZoomUnitsPerSecond",720,120,2400);
    g_camera_settings.invert_y = CameraSetting(L"InvertY", 0, 0, 1) != 0;
    g_camera_trace = GetEnvironmentVariableW(L"FIGHTINGFORCE_TRACE_CAMERA", nullptr, 0) != 0;
    QueryPerformanceFrequency(&g_camera_frequency);
    // Resolve optional controller support lazily, outside the loader lock.
    Log("Gameplay camera/input hooks enabled in process memory.\r\n");
    if(g_free_orbit && !InstallCameraGeometryHooks()) {
        g_free_orbit=false;Log("Camera obstruction signatures differ; native camera retained.\r\n");
    }
}
