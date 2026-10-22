#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mmsystem.h>
#include "game_image.h"

// This compatibility DLL never writes to retail files on disk.
// The retail PCI BIOS probes cannot run in an NT user process. An exception
// handler reports "device absent" using the original carry-flag error path.
static uintptr_t g_base;
static PVOID g_handler;
static volatile LONG g_probe_count;
static INIT_ONCE g_input_once = INIT_ONCE_STATIC_INIT;
static HMODULE g_input;
static wchar_t g_log_path[MAX_PATH];
static bool g_rip_source;
static wchar_t g_folder_root[MAX_PATH];
static decltype(&SetCurrentDirectoryW) g_set_directory_w=SetCurrentDirectoryW;
static decltype(&GetFileAttributesExW) g_source_file_attributes=GetFileAttributesExW;
using RegOpenFunction = LSTATUS(WINAPI*)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
using SetDirectoryFunction = BOOL(WINAPI*)(LPCSTR);
static RegOpenFunction g_reg_open = RegOpenKeyExA;
static SetDirectoryFunction g_set_directory = SetCurrentDirectoryA;
static char g_last_media_root[4];
static wchar_t g_config_path[MAX_PATH];
static bool g_recover_keyboard = true;
static FARPROC SystemFunction(const char* name);

static void Log(const char* message) {
    HANDLE file = CreateFileW(g_log_path, FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    char prefix[64];
    sprintf_s(prefix, "[%llu ms] ", GetTickCount64());
    WriteFile(file, prefix, static_cast<DWORD>(lstrlenA(prefix)), &written, nullptr);
    WriteFile(file, message, static_cast<DWORD>(lstrlenA(message)), &written, nullptr);
    CloseHandle(file);
}

#include "input_proxy.h"
#include "cd_audio_session.h"
#include "render_observer.h"
#include "transition_guard.h"
#include "widescreen.h"
#include "camera_runtime.h"
#include "coop_runtime.h"
#include "frame_runtime.h"
#include "combat_feedback_runtime.h"
#include "player_guard_runtime.h"
#include "control_menu_runtime.h"
static bool FindMediaRoot(char (&root)[4]);
static void WINAPI FeedbackExitProcess(UINT code) {ShutdownControllerFeedback();ExitProcess(code);}
static bool (*g_audio_media_root)(char (&)[4])=FindMediaRoot;
static ULONGLONG g_audio_scan_at;
static char g_audio_scan_root[4];
static bool g_audio_scan_found,g_audio_missing_logged;
static bool NamesCdAudio(const char* command) {
    if(!command)return false;const char* device=strchr(command,' ');if(!device)return false;
    while(*device==' ')++device;
    return !_strnicmp(device,"cdaudio",7) && (!device[7] || device[7]==' ');
}
static bool RipAudioMedia(char (&root)[4]) {
    const ULONGLONG now=GetTickCount64();
    if(!g_audio_scan_at || now-g_audio_scan_at>=2000) {
        g_audio_scan_found=g_audio_media_root(g_audio_scan_root);g_audio_scan_at=now;
    }
    if(g_audio_scan_found)strcpy_s(root,g_audio_scan_root);return g_audio_scan_found;
}
static void PlaySourceCdTrack(DWORD native_rva,int track) {
    // The native track-start routines retry an unavailable CD twelve times,
    // with a half-second busy wait each time. Use their own music-disabled
    // path for this call only, preserving track bookkeeping and preferences.
    auto enabled=reinterpret_cast<DWORD*>(g_base+0x6F050);
    const DWORD saved=*enabled;char root[4];
    const bool silent=g_rip_source && !RipAudioMedia(root);
    if(silent)*enabled=0;
    reinterpret_cast<void(__cdecl*)(int)>(g_base+native_rva)(track);
    if(silent)*enabled=saved;
}
static void __cdecl GameCdLoopTrack(int track){PlaySourceCdTrack(0x429E0,track);}
static void __cdecl GameCdOneShotTrack(int track){PlaySourceCdTrack(0x42BB0,track);}
static bool InstallSourceCdTracks() {
    if(!g_rip_source)return true;
    struct Call {DWORD rva,target;void* hook;};
    const Call calls[]={
        {0x2EE71,0x429E0,reinterpret_cast<void*>(GameCdLoopTrack)},
        {0x40FDD,0x429E0,reinterpret_cast<void*>(GameCdLoopTrack)},
        {0x4D275,0x429E0,reinterpret_cast<void*>(GameCdLoopTrack)},
        {0x4D5A6,0x429E0,reinterpret_cast<void*>(GameCdLoopTrack)},
        {0x4DB61,0x429E0,reinterpret_cast<void*>(GameCdLoopTrack)},
        {0x41940,0x42BB0,reinterpret_cast<void*>(GameCdOneShotTrack)}};
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG relative;memcpy(&relative,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+relative!=g_base+call.target)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+calls[0].rva);const SIZE_T length=0x4DB66-calls[0].rva;DWORD previous;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(call.hook)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,4);
    }
    DWORD ignored;VirtualProtect(start,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    Log("RIP CD track starts use native silent mode when audio media is absent; retry waits avoided.\r\n");return true;
}

static void ShortenStartupScreen() {
    if (!GetPrivateProfileIntW(L"Compatibility", L"ShortStartupScreen", 1, g_config_path)) return;
    // Retail 0x422570 displays english.tga, then busy-waits for 10 seconds
    // without pumping window messages. Change only the two comparison operands
    // in process memory; keep all original draw/initialization/cleanup paths.
    const DWORD rvas[] = {0x2282B, 0x2283F};
    const BYTE expected[] = {0x3D, 0x10, 0x27, 0x00, 0x00};
    for (DWORD rva : rvas)
        if (memcmp(reinterpret_cast<void*>(g_base + rva), expected, sizeof(expected))) {
            Log("Startup wait signature differs; keeping native timing.\r\n"); return;
        }
    DWORD previous;
    BYTE* first = reinterpret_cast<BYTE*>(g_base + rvas[0]);
    if (!VirtualProtect(first, rvas[1] - rvas[0] + 5, PAGE_EXECUTE_READWRITE, &previous)) return;
    DWORD duration=GetPrivateProfileIntW(L"Compatibility",L"StartupScreenMs",2500,g_config_path);
    if(duration<500)duration=500;if(duration>10000)duration=10000;
    for (DWORD rva : rvas) memcpy(reinterpret_cast<void*>(g_base + rva + 1), &duration, sizeof(duration));
    DWORD ignored;
    VirtualProtect(first, rvas[1] - rvas[0] + 5, previous, &ignored);
    FlushInstructionCache(GetCurrentProcess(), first, rvas[1] - rvas[0] + 5);
    char message[128];sprintf_s(message,"Startup screen wait: 10000 -> %lu ms (process memory only).\r\n",duration);Log(message);
}

static DWORD WINAPI GameMciSendString(LPCSTR command, LPSTR output, UINT length, HWND callback) {
    if(g_rip_source && NamesCdAudio(command)) {
        char root[4];
        if(!RipAudioMedia(root)) {
            if(output && length)output[0]=0;
            if(!g_audio_missing_logged) {Log("RIP has no mounted CD audio; silent playback retained without optical-device waits.\r\n");g_audio_missing_logged=true;}
            return MCIERR_DEVICE_NOT_READY;
        }
        g_audio_missing_logged=false;
    }
    using Function = DWORD(WINAPI*)(LPCSTR, LPSTR, UINT, HWND);
    static auto real = reinterpret_cast<Function>(GetProcAddress(LoadLibraryW(L"winmm.dll"), "mciSendStringA"));
    static SRWLOCK lock = SRWLOCK_INIT;
    static CdAudioSession session;
    if (!real) return 1;
    ULONGLONG start = GetTickCount64();
    bool reused = false;
    DWORD result;
    if (GetPrivateProfileIntW(L"Compatibility", L"ReuseCDAudio", 1, g_config_path)) {
        AcquireSRWLockExclusive(&lock);
        char root[4];
        const char* media = command && !_stricmp(command, "open cdaudio") && g_audio_media_root(root) ? root : nullptr;
        result = session.SendCommand(command, output, length, callback, media, real, &reused);
        ReleaseSRWLockExclusive(&lock);
    } else result = real(command, output, length, callback);
    ULONGLONG duration = GetTickCount64() - start;
    const bool playback_command=command && (!_strnicmp(command,"play cdaudio",12) || !_strnicmp(command,"stop cdaudio",12));
    if (result || duration >= 100 || reused || playback_command) {
        char message[320];
        sprintf_s(message, "CD audio: %.180s -> %lu (%llu ms)%s\r\n", command ? command : "<null>", result, duration,
            reused ? "; reused mounted-disc session" : "");
        Log(message);
    }
    return result;
}

static void ConfigureDpiAwareness() {
    // Called during this early-loaded proxy's initialization, before the game
    // creates either its movie window or its DirectDraw window. Keep legacy
    // render sizes in physical pixels instead of Windows scaling them again.
    using SetContext = BOOL(WINAPI*)(HANDLE);
    using GetContext = HANDLE(WINAPI*)();
    using GetAwareness = int(WINAPI*)(HANDLE);
    HMODULE user = GetModuleHandleW(L"user32.dll");
    auto set_context = reinterpret_cast<SetContext>(
        GetProcAddress(user, "SetProcessDpiAwarenessContext"));
    auto get_context = reinterpret_cast<GetContext>(
        GetProcAddress(user, "GetThreadDpiAwarenessContext"));
    auto get_awareness = reinterpret_cast<GetAwareness>(
        GetProcAddress(user, "GetAwarenessFromDpiAwarenessContext"));
    SetLastError(ERROR_SUCCESS);
    BOOL configured = set_context
        ? set_context(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))
        : SetProcessDPIAware();
    DWORD error = configured ? ERROR_SUCCESS : GetLastError();
    int awareness = get_context && get_awareness ? get_awareness(get_context()) : -1;
    char message[192];
    sprintf_s(message, "DPI initialization: configured=%d, awareness=%d, error=%lu, desktop=%dx%d physical pixels\r\n",
        configured, awareness, error, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    Log(message);
}

static void LogActiveGameWindow() {
    HWND window = GetActiveWindow();
    if (!window) return;
    RECT client = {}, frame = {};
    GetClientRect(window, &client);
    GetWindowRect(window, &frame);
    using GetDpi = UINT(WINAPI*)(HWND);
    auto get_dpi = reinterpret_cast<GetDpi>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    char message[192];
    sprintf_s(message, "Game window: client=%ldx%ld, frame=%ldx%ld, DPI=%u\r\n",
        client.right - client.left, client.bottom - client.top,
        frame.right - frame.left, frame.bottom - frame.top,
        get_dpi ? get_dpi(window) : 0);
    Log(message);
}

static bool RecognizeRetailImage() {
    auto base = reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.TimeDateStamp != 0x34ED7B34 ||
        nt->OptionalHeader.ImageBase != 0x400000) return false;
    const auto kind=ff_image::Classify(base,nt->OptionalHeader.SizeOfImage);
    if(kind==ff_image::Kind::Unsupported)return false;
    if(kind==ff_image::Kind::Rip) {
        wchar_t path[MAX_PATH];DWORD count=GetModuleFileNameW(nullptr,path,MAX_PATH);
        if(!count || count>=MAX_PATH || !ff_image::IsReviewedRip(path))return false;
        wchar_t* slash=wcsrchr(path,L'\\');if(!slash)return false;slash[1]=0;
        wcscpy_s(g_folder_root,path);g_rip_source=true;
        Log("Reviewed English RIP recognized; local folder source enabled.\r\n");
    }
    g_base = reinterpret_cast<uintptr_t>(base);
    return true;
}

static LONG CALLBACK PciBiosException(PEXCEPTION_POINTERS event) {
    if (!g_base || !event || !event->ExceptionRecord || !event->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;
    const DWORD code = event->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    auto context = event->ContextRecord;
    for (DWORD rva : ff_image::ProbeRvas) {
        if (context->Eip != g_base + rva ||
            event->ExceptionRecord->ExceptionAddress != reinterpret_cast<void*>(g_base + rva) ||
            context->Eax != 0xB102) continue;
        const BYTE* instruction = reinterpret_cast<const BYTE*>(context->Eip);
        if (instruction[0] != 0xCD || instruction[1] != 0x1A)
            return EXCEPTION_CONTINUE_SEARCH;
        context->Eip += 2;
        context->EFlags |= 1; // PCI BIOS error: retain the game's original fallback.
        InterlockedIncrement(&g_probe_count);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static bool MediaFileMatches(const char* root, const char* relative, DWORD size) {
    char path[MAX_PATH];
    strcpy_s(path, root);
    strcat_s(path, relative);
    WIN32_FILE_ATTRIBUTE_DATA file;
    return GetFileAttributesExA(path, GetFileExInfoStandard, &file) &&
        !(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        file.nFileSizeHigh == 0 && file.nFileSizeLow == size;
}

static bool FindMediaRoot(char (&root)[4]) {
    char drives[512];
    DWORD count = GetLogicalDriveStringsA(sizeof(drives), drives);
    if (!count || count >= sizeof(drives)) return false;
    for (const char* drive = drives; *drive; drive += lstrlenA(drive) + 1) {
        if (GetDriveTypeA(drive) != DRIVE_CDROM) continue;
        if (!MediaFileMatches(drive, "Wads\\ENGLISH.WAD", 3040) ||
            !MediaFileMatches(drive, "Wads\\STAGE11.WAD", 1707820) ||
            !MediaFileMatches(drive, "Intro\\logo.rpl", 15996067)) continue;
        strcpy_s(root, drive);
        if (strcmp(root, g_last_media_root)) {
            strcpy_s(g_last_media_root, root);
            char message[160];
            sprintf_s(message, "Original game CD detected at %s; PCI BIOS probes handled: %ld\r\n",
                root, g_probe_count);
            Log(message);
        }
        return true;
    }
    return false;
}

static LSTATUS WINAPI GameRegOpen(HKEY key, LPCSTR name, DWORD options,
    REGSAM access, PHKEY output) {
    if (g_base && key == HKEY_CLASSES_ROOT && name &&
        !_stricmp(name, "Software\\Core Design\\Fighting Force")) {
        if(g_rip_source) {
            // A relative source token fits the native 128-byte buffer. Resolve
            // it against the EXE folder at the directory boundary, never the CD.
            memcpy(reinterpret_cast<void*>(g_base+0x6C0D0),".\\",3);
            if(output)*output=nullptr;return ERROR_FILE_NOT_FOUND;
        }
        char root[4];
        if (FindMediaRoot(root)) {
            // The original installer stored the source drive in HKCR. Supply
            // the real mounted CD in the existing runtime buffer, then use
            // the game's normal "no installation registry entry" fallback.
            // No registry data or original files are changed.
            memcpy(reinterpret_cast<void*>(g_base + 0x6C0D0), root, sizeof(root));
            if (output) *output = nullptr;
            return ERROR_FILE_NOT_FOUND;
        }
    }
    return g_reg_open(key, name, options, access, output);
}

static BOOL WINAPI GameSetDirectory(LPCSTR path) {
    if (g_base && path == reinterpret_cast<LPCSTR>(g_base + 0x6C0D0)) {
        if(g_rip_source && path[0]=='.' && path[1]=='\\') {
            wchar_t suffix[MAX_PATH],destination[MAX_PATH];
            if(!MultiByteToWideChar(CP_ACP,0,path+2,-1,suffix,MAX_PATH))return FALSE;
            if(swprintf_s(destination,L"%s%s",g_folder_root,suffix)<0)return FALSE;
            return g_set_directory_w(destination);
        }
        char root[4];
        if (FindMediaRoot(root)) {
            // Preserve any relative suffix appended by the game. Also rescan
            // on its Retry path, so a remounted disc can have another letter.
            memcpy(reinterpret_cast<void*>(g_base + 0x6C0D0), root, 3);
        }
    }
    return g_set_directory(path);
}

static int __cdecl GameOptionalIntro(const char* path) {
    if(g_rip_source && path && !_stricmp(path,"intro\\logo.rpl")) {
        wchar_t filename[MAX_PATH];WIN32_FILE_ATTRIBUTE_DATA file;
        if(swprintf_s(filename,L"%sintro\\logo.rpl",g_folder_root)<0 ||
            !g_source_file_attributes(filename,GetFileExInfoStandard,&file) ||
            (!file.nFileSizeHigh && !file.nFileSizeLow)) {
            Log("Empty or absent RIP intro skipped; source files unchanged.\r\n");return 1;
        }
    }
    return reinterpret_cast<int(__cdecl*)(const char*)>(g_base+0x2E650)(path);
}
static bool InstallOptionalIntro() {
    if(!g_rip_source)return true;
    BYTE* code=reinterpret_cast<BYTE*>(g_base+0x2E0C1);
    const BYTE expected[]={0xE8,0x8A,0x05,0,0};
    if(memcmp(code,expected,sizeof(expected)))return false;
    DWORD previous;if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&previous))return false;
    LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(GameOptionalIntro)-reinterpret_cast<uintptr_t>(code+5));
    memcpy(code+1,&relative,4);DWORD ignored;VirtualProtect(code,5,previous,&ignored);
    FlushInstructionCache(GetCurrentProcess(),code,5);return true;
}
static int __cdecl GameVideoSettings(void* output) {
    using Read=int(__cdecl*)(void*);
    const bool test_defaults=GetEnvironmentVariableW(L"FIGHTINGFORCE_TEST_VIDEO_DEFAULTS",nullptr,0)!=0;
    if(!test_defaults && reinterpret_cast<Read>(g_base+0x221B0)(output))return 1;
    // Use the primary DirectDraw adapter and the standard Direct3D HAL device
    // when legacy setup has never saved a device selection. No registry writes.
    auto settings=static_cast<DWORD*>(output);memset(settings,0,0x74);
    settings[0]=settings[1]=settings[2]=settings[6]=settings[8]=1;
    settings[9]=settings[13]=100;settings[10]=640;settings[11]=480;settings[28]=0x10;
    memcpy(settings+18,&IID_IDirect3DHALDevice,sizeof(GUID));
    Log("No stored video settings; primary Direct3D HAL defaults supplied in memory.\r\n");return 1;
}
static bool InstallVideoDefaults() {
    BYTE* code=reinterpret_cast<BYTE*>(g_base+0x2DEA2);LONG original;
    memcpy(&original,code+1,4);
    if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+original!=g_base+0x221B0)return false;
    DWORD previous;if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&previous))return false;
    LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(GameVideoSettings)-reinterpret_cast<uintptr_t>(code+5));
    memcpy(code+1,&relative,4);DWORD ignored;VirtualProtect(code,5,previous,&ignored);
    FlushInstructionCache(GetCurrentProcess(),code,5);return true;
}

static bool ReplaceImport(const char* dll, const char* name, void* replacement,void** original=nullptr) {
    auto base = reinterpret_cast<BYTE*>(g_base);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base +
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (; descriptor->Name; ++descriptor) {
        if (_stricmp(reinterpret_cast<char*>(base + descriptor->Name), dll)) continue;
        if (!descriptor->OriginalFirstThunk) return false;
        auto names = reinterpret_cast<IMAGE_THUNK_DATA32*>(base + descriptor->OriginalFirstThunk);
        auto addresses = reinterpret_cast<IMAGE_THUNK_DATA32*>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses) {
            if (IMAGE_SNAP_BY_ORDINAL32(names->u1.Ordinal)) continue;
            auto entry = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
            if (strcmp(reinterpret_cast<char*>(entry->Name), name)) continue;
            DWORD previous;
            if (!VirtualProtect(&addresses->u1.Function, sizeof(DWORD), PAGE_READWRITE, &previous))
                return false;
            if(original)*original=reinterpret_cast<void*>(addresses->u1.Function);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&addresses->u1.Function),
                static_cast<LONG>(reinterpret_cast<uintptr_t>(replacement)));
            DWORD ignored;
            VirtualProtect(&addresses->u1.Function, sizeof(DWORD), previous, &ignored);
            return true;
        }
    }
    return false;
}

static BOOL CALLBACK LoadSystemInput(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[MAX_PATH];
    UINT length = GetSystemDirectoryW(path, MAX_PATH);
    if (!length || length + 12 >= MAX_PATH) return FALSE;
    lstrcatW(path, L"\\dinput.dll");
    g_input = LoadLibraryW(path);
    char message[192];
    sprintf_s(message, "System DirectInput loaded: %s; intercepted PCI BIOS probes: %ld\r\n",
        g_input ? "yes" : "no", g_probe_count);
    Log(message);
    return g_input != nullptr;
}

static FARPROC SystemFunction(const char* name) {
    if (!InitOnceExecuteOnce(&g_input_once, LoadSystemInput, nullptr, nullptr)) return nullptr;
    return GetProcAddress(g_input, name);
}

extern "C" HRESULT WINAPI Proxy_DirectInputCreateA(HINSTANCE instance, DWORD version,
    void** output, void* outer) {
    using Function = HRESULT(WINAPI*)(HINSTANCE, DWORD, void**, void*);
    auto function = reinterpret_cast<Function>(SystemFunction("DirectInputCreateA"));
    HRESULT result = function ? function(instance, version, output, outer) : E_FAIL;
    if (g_base && g_recover_keyboard && SUCCEEDED(result) && output && *output && !outer) {
        auto proxy = new(std::nothrow) InputProxy(static_cast<IDirectInputA*>(*output));
        if (proxy) *output = proxy;
    }
    if (g_base) LogActiveGameWindow();
    return result;
}
extern "C" HRESULT WINAPI Proxy_DirectInputCreateW(HINSTANCE instance, DWORD version,
    void** output, void* outer) {
    using Function = HRESULT(WINAPI*)(HINSTANCE, DWORD, void**, void*);
    auto function = reinterpret_cast<Function>(SystemFunction("DirectInputCreateW"));
    return function ? function(instance, version, output, outer) : E_FAIL;
}
extern "C" HRESULT WINAPI Proxy_DirectInputCreateEx(HINSTANCE instance, DWORD version,
    const GUID& iid, void** output, void* outer) {
    using Function = HRESULT(WINAPI*)(HINSTANCE, DWORD, const GUID&, void**, void*);
    auto function = reinterpret_cast<Function>(SystemFunction("DirectInputCreateEx"));
    return function ? function(instance, version, iid, output, outer) : E_FAIL;
}
extern "C" HRESULT WINAPI Proxy_DllCanUnloadNow() {
    using Function = HRESULT(WINAPI*)();
    auto function = reinterpret_cast<Function>(SystemFunction("DllCanUnloadNow"));
    return function ? function() : S_FALSE;
}
extern "C" HRESULT WINAPI Proxy_DllGetClassObject(const GUID& clsid, const GUID& iid,
    void** output) {
    using Function = HRESULT(WINAPI*)(const GUID&, const GUID&, void**);
    auto function = reinterpret_cast<Function>(SystemFunction("DllGetClassObject"));
    return function ? function(clsid, iid, output) : E_FAIL;
}
extern "C" HRESULT WINAPI Proxy_DllRegisterServer() {
    using Function = HRESULT(WINAPI*)();
    auto function = reinterpret_cast<Function>(SystemFunction("DllRegisterServer"));
    return function ? function() : E_FAIL;
}
extern "C" HRESULT WINAPI Proxy_DllUnregisterServer() {
    using Function = HRESULT(WINAPI*)();
    auto function = reinterpret_cast<Function>(SystemFunction("DllUnregisterServer"));
    return function ? function() : E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        GetModuleFileNameW(instance, g_log_path, MAX_PATH);
        wchar_t* slash = wcsrchr(g_log_path, L'\\');
        if (slash) lstrcpyW(slash + 1, L"fightingforce-compat.log");
        lstrcpyW(g_config_path, g_log_path);
        wchar_t* config_slash = wcsrchr(g_config_path, L'\\');
        if (config_slash) lstrcpyW(config_slash + 1, L"fightingforce-compat.ini");
        if (RecognizeRetailImage()) {
            if(!InstallVideoDefaults())Log("Video-settings signature differs; native settings retained.\r\n");
            g_recover_keyboard = GetPrivateProfileIntW(L"Compatibility", L"RecoverKeyboard", 1, g_config_path) != 0;
            g_smooth_transitions=GetPrivateProfileIntW(L"Compatibility",L"SmoothTransitions",1,g_config_path)!=0;
            if(GetEnvironmentVariableW(L"FIGHTINGFORCE_TEST_TRANSITIONS",nullptr,0))g_smooth_transitions=true;
            g_transition_instance=instance;
            if(g_smooth_transitions) {
                g_render_scene_complete=TransitionSceneComplete;
                g_render_before_release=TransitionRendererRelease;
                g_render_poll=PollGameTransition;
            }
            ConfigureDpiAwareness();
            g_loading_renderer_observe=PrepareLoadingRenderer;
            ShortenStartupScreen();
            InstallCopyrightTransitions();
            InstallLoadingStartTransitions();
            InstallWidescreenHooks();
            InstallModernInputHooks();
            g_transition_feedback_reset=StopControllerFeedback;
            const BYTE movie_abort_read[]={0xA1,0x44,0xD5,0x60,0x00,0x83,0xC4,0x20,0x85,0xC0};
            if(!memcmp(reinterpret_cast<void*>(g_base+0x2E91A),movie_abort_read,sizeof(movie_abort_read)))
                g_movie_input_poll=ModernMovieInput;
            InstallCameraHooks();
            g_presentation_focus_lost=GameFocusLost;
            InstallPresentationGuard();
            InstallCooperativeHooks();
            InstallSmoothFrameHooks();
            InstallControlMenu();
            InstallPlayerGuard();
            InstallCombatFeedback();
            g_handler = AddVectoredExceptionHandler(1, PciBiosException);
            if(g_rip_source && (!g_handler || !ff_image::RestoreRipProbes(reinterpret_cast<BYTE*>(g_base),
                reinterpret_cast<IMAGE_NT_HEADERS32*>(g_base+reinterpret_cast<IMAGE_DOS_HEADER*>(g_base)->e_lfanew)->OptionalHeader.SizeOfImage)))
                Log("RIP BIOS fallback could not be enabled.\r\n");
            if(!InstallOptionalIntro())Log("Optional intro signature differs; native intro retained.\r\n");
            if(!InstallSourceCdTracks())Log("CD track-start signatures differ; native retry behavior retained.\r\n");
            Log(g_handler ? "Retail image recognized; PCI BIOS compatibility handler enabled.\r\n"
                          : "Failed to register PCI BIOS compatibility handler.\r\n");
            const bool registry = ReplaceImport("ADVAPI32.dll", "RegOpenKeyExA",
                reinterpret_cast<void*>(GameRegOpen));
            const bool directory = ReplaceImport("KERNEL32.dll", "SetCurrentDirectoryA",
                reinterpret_cast<void*>(GameSetDirectory));
            ReplaceImport("WINMM.dll", "mciSendStringA", reinterpret_cast<void*>(GameMciSendString));
            ReplaceImport("USER32.dll", "PeekMessageA", reinterpret_cast<void*>(GamePeekMessage));
            ReplaceImport("KERNEL32.dll", "ExitProcess", reinterpret_cast<void*>(FeedbackExitProcess));
            ReplaceImport("USER32.dll", "RegisterClassExA", reinterpret_cast<void*>(GameRegisterClassExA),reinterpret_cast<void**>(&g_native_register_class));
            ReplaceImport("USER32.dll", "CreateWindowExA", reinterpret_cast<void*>(GameCreateWindowExA),reinterpret_cast<void**>(&g_native_create_window));
            ReplaceImport("USER32.dll", "DestroyWindow", reinterpret_cast<void*>(GameDestroyWindow),reinterpret_cast<void**>(&g_native_destroy_window));
            ReplaceImport("USER32.dll", "GetClientRect", reinterpret_cast<void*>(GameGetClientRect),reinterpret_cast<void**>(&g_native_get_client_rect));
            ReplaceImport("winplay.dll", "Player_InitVideoSystem", reinterpret_cast<void*>(GameMovieInitVideoSystem));
            ReplaceImport("winplay.dll", "Player_ReturnPlaybackMode", reinterpret_cast<void*>(GameMovieReturnPlaybackMode));
            ReplaceImport("winplay.dll", "Player_PlayFrame", reinterpret_cast<void*>(GameMoviePlayFrame));
            Log(registry && directory ? "Mounted-CD discovery hooks enabled.\r\n"
                                      : "Failed to install mounted-CD discovery hooks.\r\n");
        } else {
            Log("Image not recognized; forwarding DirectInput without game fixes.\r\n");
        }
    } else if (reason == DLL_PROCESS_DETACH && !reserved && g_handler) {
        RemoveVectoredExceptionHandler(g_handler);
    }
    return TRUE;
}
