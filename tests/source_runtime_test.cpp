#include "../src/dinput_proxy.cpp"
#include <string>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,reg_calls,narrow_calls,wide_calls;
static std::wstring wide_path;
static bool movie_exists=true;static DWORD movie_size;
static int disc_scans;
static bool disc_available;
static bool NoDisc(char (&root)[4]) {
    ++disc_scans;const char fixture_root[]={'Z',':','\\',0};
    if(disc_available)strcpy_s(root,fixture_root);return disc_available;
}
static HANDLE cd_opened,cd_release_open,cd_playing,cd_release_play,cd_resumed,cd_stopped,cd_release_stop,cd_restarted;
static unsigned cd_stop_count;
static DWORD WINAPI SlowCd(LPCSTR command,LPSTR output,UINT length,HWND) {
    if(!strcmp(command,"open Z: type cdaudio alias cdaudio shareable")) {
        SetEvent(cd_opened);WaitForSingleObject(cd_release_open,3000);
    } else if(!strcmp(command,"status cdaudio length track 22"))strcpy_s(output,length,"148800");
    else if(!strcmp(command,"status cdaudio position track 22"))strcpy_s(output,length,"2785947");
    else if(!strcmp(command,"play cdaudio from 22")) {
        SetEvent(cd_playing);WaitForSingleObject(cd_release_play,3000);
    } else if(!strcmp(command,"play cdaudio from 2786597"))SetEvent(cd_resumed);
    else if(!strcmp(command,"stop cdaudio wait")) {
        ++cd_stop_count;SetEvent(cd_stopped);WaitForSingleObject(cd_release_stop,3000);
    } else if(!strcmp(command,"play cdaudio from 24"))SetEvent(cd_restarted);
    return 0;
}
static CdSend TestCdSend() {return SlowCd;}
static int track_calls,last_track;static DWORD observed_music;
static void __cdecl TrackSpy(int track) {
    ++track_calls;last_track=track;observed_music=*reinterpret_cast<DWORD*>(g_base+0x6F050);
}
static void Branch(BYTE* code,uintptr_t target) {
    code[0]=0xE8;LONG relative=static_cast<LONG>(target-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&relative,4);
}
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static LSTATUS WINAPI RegSpy(HKEY,LPCSTR,DWORD,REGSAM,PHKEY) {++reg_calls;return ERROR_ACCESS_DENIED;}
static BOOL WINAPI NarrowSpy(LPCSTR) {++narrow_calls;return TRUE;}
static BOOL WINAPI WideSpy(LPCWSTR path) {++wide_calls;wide_path=path;return TRUE;}
static BOOL WINAPI FileSpy(LPCWSTR,GET_FILEEX_INFO_LEVELS,LPVOID output) {
    auto value=static_cast<WIN32_FILE_ATTRIBUTE_DATA*>(output);*value={};value->nFileSizeLow=movie_size;return movie_exists;
}
static void SetProbes(BYTE* base,bool rip) {
    const BYTE code[]={0xB8,0x02,0xB1,0,0,0xCD,0x1A,0x72,0x07};
    for(DWORD rva:ff_image::ProbeRvas) {memcpy(base+rva-5,code,sizeof(code));if(rip)base[rva]=base[rva+1]=0x90;}
}
__declspec(naked) static int InvokeIntro(void*,const char*) {
    __asm {
        mov eax,[esp+4]
        mov ecx,[esp+8]
        push ecx
        jmp eax
    }
}
int main() {
    constexpr size_t length=0x880000;
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,length,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(base+0x80);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;nt->FileHeader.TimeDateStamp=0x34ED7B34;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;nt->OptionalHeader.ImageBase=0x400000;
    SetProbes(base,false);CHECK(ff_image::Classify(base,length)==ff_image::Kind::Retail);
    CHECK(ff_image::Classify(base,sizeof(IMAGE_DOS_HEADER))==ff_image::Kind::Unsupported);
    CHECK(ff_image::Classify(base,ff_image::ProbeRvas[3]+1)==ff_image::Kind::Unsupported);
    base[ff_image::ProbeRvas[0]]=base[ff_image::ProbeRvas[0]+1]=0x90;
    CHECK(ff_image::Classify(base,length)==ff_image::Kind::Unsupported);
    BYTE before[9];memcpy(before,base+ff_image::ProbeRvas[0]-5,9);
    CHECK(!ff_image::RestoreRipProbes(base,length));CHECK(!memcmp(before,base+ff_image::ProbeRvas[0]-5,9));
    SetProbes(base,true);CHECK(ff_image::Classify(base,length)==ff_image::Kind::Rip);
    CHECK(ff_image::RestoreRipProbes(base,length));CHECK(ff_image::Classify(base,length)==ff_image::Kind::Retail);
    EXCEPTION_RECORD record={};CONTEXT context={};EXCEPTION_POINTERS event={&record,&context};
    record.ExceptionCode=EXCEPTION_PRIV_INSTRUCTION;record.ExceptionAddress=base+ff_image::ProbeRvas[0];
    context.Eip=static_cast<DWORD>(g_base+ff_image::ProbeRvas[0]);context.Eax=0xB102;
    CHECK(PciBiosException(&event)==EXCEPTION_CONTINUE_EXECUTION);CHECK((context.EFlags&1) && context.Eip==g_base+ff_image::ProbeRvas[0]+2);
    context.Eax=0;CHECK(PciBiosException(&event)==EXCEPTION_CONTINUE_SEARCH);
    g_reg_open=RegSpy;g_set_directory=NarrowSpy;g_set_directory_w=WideSpy;
    g_rip_source=true;wcscpy_s(g_folder_root,L"fixture\\");HKEY key=reinterpret_cast<HKEY>(1);
    CHECK(GameRegOpen(HKEY_CLASSES_ROOT,"Software\\Core Design\\Fighting Force",0,0,&key)==ERROR_FILE_NOT_FOUND);
    CHECK(!key && !reg_calls);auto source=reinterpret_cast<char*>(base+0x6C0D0);CHECK(!strcmp(source,".\\"));
    strcat_s(source,128,"\\");CHECK(GameSetDirectory(source));CHECK(wide_calls==1 && !narrow_calls && wide_path==L"fixture\\\\");
    CHECK(GameSetDirectory("unrelated"));CHECK(narrow_calls==1);
    CHECK(GameRegOpen(HKEY_CURRENT_USER,"unrelated",0,0,&key)==ERROR_ACCESS_DENIED && reg_calls==1);
    // A native stub retains cdecl argument cleanup when the optional intro is present.
    const BYTE native[]={0xB8,123,0,0,0,0xC3};memcpy(base+0x2E650,native,sizeof(native));
    g_source_file_attributes=FileSpy;movie_size=0;
    CHECK(GameOptionalIntro("intro\\logo.rpl")==1);movie_exists=false;CHECK(GameOptionalIntro("intro\\logo.rpl")==1);
    movie_exists=true;movie_size=1;CHECK(GameOptionalIntro("intro\\logo.rpl")==123);
    movie_size=0;CHECK(GameOptionalIntro("another.rpl")==123);
    CHECK(!InstallOptionalIntro());CHECK(base[0x2E0C1]==0);
    const BYTE call[]={0xE8,0x8A,0x05,0,0,0x83,0xC4,4,0xC3};memcpy(base+0x2E0C1,call,sizeof(call));
    CHECK(InstallOptionalIntro());CHECK(InvokeIntro(base+0x2E0C1,"intro\\logo.rpl")==1);
    movie_size=1;CHECK(InvokeIntro(base+0x2E0C1,"intro\\logo.rpl")==123);
    g_rip_source=false;movie_size=0;CHECK(GameOptionalIntro("intro\\logo.rpl")==123);
    g_rip_source=true;g_audio_media_root=NoDisc;g_audio_scan_at=0;char output[16]="stale";
    CHECK(GameMciSendString("open cdaudio",output,sizeof(output),nullptr)==MCIERR_DEVICE_NOT_READY);
    CHECK(!output[0]);CHECK(GameMciSendString("play cdaudio from 24",output,sizeof(output),nullptr)==MCIERR_DEVICE_NOT_READY);
    CHECK(disc_scans==1 && g_audio_missing_logged);
    CHECK(!NamesCdAudio(nullptr) && !NamesCdAudio("open waveaudio") && !NamesCdAudio("open cdaudio2"));
    CHECK(NamesCdAudio("STATUS CDAUDIO number of tracks"));
    auto music=reinterpret_cast<DWORD*>(base+0x6F050);*music=7;
    for(DWORD rva:{0x429E0u,0x42BB0u}) {
        Branch(base+rva,reinterpret_cast<uintptr_t>(TrackSpy));base[rva]=0xE9;
    }
    // Native track bookkeeping still executes, with music disabled only inside
    // this synchronous call. A mounted disc and the retail path retain settings.
    GameCdLoopTrack(24);CHECK(track_calls==1 && last_track==24 && observed_music==0 && *music==7);
    GameCdOneShotTrack(6);CHECK(track_calls==2 && last_track==6 && observed_music==0 && *music==7);
    disc_available=true;g_audio_scan_at=0;GameCdLoopTrack(5);
    CHECK(observed_music==7 && *music==7);*music=0;GameCdLoopTrack(4);CHECK(!observed_music && !*music);
    *music=7;g_rip_source=false;disc_available=false;g_audio_scan_at=0;GameCdLoopTrack(3);
    CHECK(observed_music==7 && *music==7);g_rip_source=true;
    const DWORD track_entries[]={0x2EE71,0x40FDD,0x4D275,0x4D5A6,0x4DB61,0x41940};
    for(DWORD rva:track_entries)Branch(base+rva,g_base+(rva==0x41940?0x42BB0:0x429E0));
    base[0x41940]=0x90;CHECK(!InstallSourceCdTracks());
    LONG unchanged;memcpy(&unchanged,base+0x2EE72,4);CHECK(g_base+0x2EE76+unchanged==g_base+0x429E0);
    Branch(base+0x41940,g_base+0x42BB0);CHECK(InstallSourceCdTracks());
    for(DWORD rva:track_entries) {
        const BYTE cleanup[]={0x83,0xC4,4,0xC3};memcpy(base+rva+5,cleanup,sizeof(cleanup));
        InvokeIntro(base+rva,reinterpret_cast<const char*>(17));
        CHECK(last_track==17 && observed_music==0 && *music==7);
    }
    DWORD settings[29];memset(settings,0xAA,sizeof(settings));
    const BYTE present[]={0xB8,1,0,0,0,0xC3};memcpy(base+0x221B0,present,sizeof(present));
    CHECK(GameVideoSettings(settings)==1 && settings[10]==0xAAAAAAAA);
    base[0x221B1]=0;CHECK(GameVideoSettings(settings)==1);
    CHECK(settings[0]==1 && settings[10]==640 && settings[11]==480 && settings[28]==0x10);
    CHECK(!memcmp(settings+18,&IID_IDirect3DHALDevice,sizeof(GUID)));
    CHECK(!InstallVideoDefaults());CHECK(base[0x2DEA2]==0);
    BYTE* settings_call=base+0x2DEA2;settings_call[0]=0xE8;
    LONG relative=static_cast<LONG>(0x221B0-0x2DEA2-5);memcpy(settings_call+1,&relative,4);
    settings_call[5]=0x83;settings_call[6]=0xC4;settings_call[7]=4;settings_call[8]=0xC3;
    CHECK(InstallVideoDefaults());memset(settings,0,sizeof(settings));
    CHECK(InvokeIntro(settings_call,reinterpret_cast<const char*>(settings))==1 && settings[10]==640);
    cd_opened=CreateEventW(nullptr,TRUE,FALSE,nullptr);cd_release_open=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    cd_playing=CreateEventW(nullptr,TRUE,FALSE,nullptr);cd_release_play=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    cd_resumed=CreateEventW(nullptr,TRUE,FALSE,nullptr);cd_stopped=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    cd_release_stop=CreateEventW(nullptr,TRUE,FALSE,nullptr);cd_restarted=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    g_cd_send=TestCdSend;g_background_cd=true;disc_available=true;
    const auto cd=[](const char* command){return GameMciSendString(command,nullptr,0,nullptr);};
    const auto start=GetTickCount64();
    CHECK(!cd("open cdaudio") && !cd("set cdaudio time format tmsf") &&
        !cd("play cdaudio from 22") && !cd("close cdaudio"));
    CHECK(WaitForSingleObject(cd_opened,1000)==WAIT_OBJECT_0 && GetTickCount64()-start<1000);
    CHECK(!GameMciSendString("status cdaudio length track 22",output,sizeof(output),nullptr));
    CHECK(atoi(output)==kCdLengthPending);
    InputField<unsigned>(0x552398)=22;InputField<int>(0x8523F0)=kCdLengthPending;
    SetEvent(cd_release_open);
    CHECK(WaitForSingleObject(cd_playing,1000)==WAIT_OBJECT_0);
    RefreshBackgroundCdLength();CHECK(InputField<int>(0x8523F0)==148800);
    CHECK(!GameMciSendString("status cdaudio position track 22",output,sizeof(output),nullptr));
    CHECK(!strcmp(output,"2785947"));
    // A blocked driver must not stall a rapid pause/resume or play an obsolete request.
    InputField<unsigned>(0x5523A0)=650;
    const auto before_resume=GetTickCount64();
    CHECK(!cd("open cdaudio") && !cd("stop cdaudio wait") && !cd("close cdaudio"));
    CHECK(!cd("open cdaudio") && !cd("set cdaudio time format milliseconds") &&
        !cd("play cdaudio from 2786597") && !cd("close cdaudio"));
    CHECK(GetTickCount64()-before_resume<1000);
    SetEvent(cd_release_play);
    CHECK(WaitForSingleObject(cd_resumed,1000)==WAIT_OBJECT_0);
    CHECK(!cd("open cdaudio") && !cd("stop cdaudio wait") && !cd("close cdaudio"));
    CHECK(WaitForSingleObject(cd_stopped,1000)==WAIT_OBJECT_0);
    const auto before_restart=GetTickCount64();
    CHECK(!cd("open cdaudio") && !cd("set cdaudio time format tmsf") &&
        !cd("play cdaudio from 24") && !cd("close cdaudio"));
    CHECK(GetTickCount64()-before_restart<1000);
    SetEvent(cd_release_stop);
    CHECK(WaitForSingleObject(cd_restarted,1000)==WAIT_OBJECT_0);
    ShutdownBackgroundCd();
    CHECK(!g_cd_thread && cd_stop_count==1);
    for(auto cd_event:{cd_opened,cd_release_open,cd_playing,cd_release_play,cd_resumed,cd_stopped,cd_release_stop,cd_restarted})
        CloseHandle(cd_event);
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Source runtime: FAIL":"Source runtime: PASS; image variants, BIOS fallback, local paths and optional intro.");return failures?1:0;
}
