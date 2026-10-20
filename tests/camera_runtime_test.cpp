#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/keyboard_read.h"
static uintptr_t g_base;
static wchar_t g_config_path[] = L"camera-runtime-test-missing.ini";
static void Log(const char*) {}
static FARPROC SystemFunction(const char* name) {
    static HMODULE runtime = LoadLibraryExW(L"dinput.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return runtime ? GetProcAddress(runtime, name) : nullptr;
}
#include "../src/camera_runtime.h"
static int failures, stage_calls, input_calls, begin_value;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
static void __cdecl NativeBegin(int value) { begin_value = value; }
static void __cdecl NativeInput() { ++input_calls; }
static void __cdecl NativeStage() {
    ++stage_calls;
    CameraField<int32_t>(0x866220) = 123; CameraField<int16_t>(0x87888C) = 456;
    CameraField<int32_t>(0x8744A8) = 789;
}
static void Branch(DWORD rva, BYTE opcode, uintptr_t target) {
    BYTE* code = reinterpret_cast<BYTE*>(g_base + rva); code[0] = opcode;
    DWORD relative = static_cast<DWORD>(target - (g_base + rva + 5)); memcpy(code + 1, &relative, 4);
}
int main() {
    g_base = reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, 0x900000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    CHECK(g_base != 0); if (!g_base) return 1;
    Branch(0x22BC0, 0xE9, reinterpret_cast<uintptr_t>(NativeBegin));
    Branch(0x484F0, 0xE9, reinterpret_cast<uintptr_t>(NativeInput));
    Branch(0x40D6C, 0xE8, g_base + 0x22BC0); Branch(0x41177, 0xE8, g_base + 0x484F0);
    const BYTE dispatch[] = {0xFF, 0x14, 0x95, 0xE0, 0x36, 0xC7, 0};
    BYTE* site = reinterpret_cast<BYTE*>(g_base + 0x4120F);
    memcpy(site, dispatch, 7); site[7] = 0xC3;
    // A failed signature must leave every other site untouched.
    site[6] = 1; InstallCameraHooks(); CHECK(*reinterpret_cast<BYTE*>(g_base + 0x40D6C) == 0xE8);
    LONG relative; memcpy(&relative, reinterpret_cast<void*>(g_base + 0x40D6D), 4);
    CHECK(g_base + 0x40D71 + relative == g_base + 0x22BC0);
    site[6] = 0; InstallCameraHooks(); CHECK(site[0] == 0xE8 && site[5] == 0x90 && site[6] == 0x90);
    // Validate the real system mouse format/cooperative-level combination on
    // an offscreen test window; no input injection or cursor capture is used.
    HWND window = CreateWindowExW(0, L"STATIC", L"Camera input test", WS_POPUP, -10000, -10000, 16, 16, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr); CameraField<HWND>(0x1EC4E4) = window;
    CHECK(PrepareCameraMouse()); ReleaseCameraMouse(); DestroyWindow(window); CameraField<HWND>(0x1EC4E4) = nullptr;
    CameraField<void(__cdecl*)()>(0x8736E0) = NativeStage;
    CameraField<int>(0x8745A0) = 1; CameraField<int16_t>(0x86A542) = 100;
    CameraField<int32_t>(0x86A528) = ff_camera::Fixed(100);
    CameraField<int32_t>(0x86A530) = ff_camera::Fixed(200);
    GameCameraBegin(17); CHECK(begin_value == 17 && !g_camera_active);
    GameCameraInput(); CHECK(input_calls == 1 && g_camera_active);
    const int16_t yaw = CameraField<int16_t>(0x87888C);
    const int32_t x = CameraField<int32_t>(0x866220);
    // Execute the actual patched seven-byte dispatch boundary. Native stage
    // side effects must run, while the orbit survives its anchored pose writes.
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 1 && CameraField<int16_t>(0x87888C) == yaw);
    CHECK(CameraField<int32_t>(0x866220) == x && CameraField<int32_t>(0x8744A8) == 0);
    CHECK(CameraField<int16_t>(0x86D318) == yaw);
    CHECK(CameraField<int32_t>(0x8677B0) == 0); // native render publication owns this copy
    CameraField<int>(0x85A668) = 1; GameCameraInput();
    CHECK(!g_camera_active && input_calls == 2);
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 2 && CameraField<int32_t>(0x866220) == 123);
    CameraField<int>(0x85A668) = 0; CameraField<int>(0x8745A0) = 2; GameCameraInput(); CHECK(!g_camera_active);
    CameraField<int>(0x8745A0) = 1; g_free_orbit = false;
    GameCameraInput(); CHECK(!g_camera_active && g_mouse_combat);
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 3 && CameraField<int32_t>(0x866220) == 123);
    VirtualFree(reinterpret_cast<void*>(g_base), 0, MEM_RELEASE);
    printf("Camera runtime ownership and dispatch: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
