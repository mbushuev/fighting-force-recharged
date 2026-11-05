#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#include <utility>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,writes,ready_calls,stage_result=2,frontend_result;
static bool write_ok=true,focus=true,choose_resume,restart_once;
static ff_campaign::ReadResult read_result=ff_campaign::ReadResult::Missing;
static ff_campaign::Checkpoint disk;
static ff_campaign::Slot slot_disk[3];
static ff_campaign::SlotStatus slot_status[3]={};
static unsigned menu_input[2];static int draws;
static std::vector<std::string> drawn_text;
static LONG CALLBACK FixtureException(EXCEPTION_POINTERS* error) {
    printf("Native menu fixture exception %08lx at relative address %08lx\n",error->ExceptionRecord->ExceptionCode,
        error->ContextRecord->Eip-DWORD(g_base));fflush(stdout);ExitProcess(1);
}
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static ff_campaign::Checkpoint Example(int players=2) {
    ff_campaign::Checkpoint c;c.stage=1;c.players=players;c.mode=players-1;
    c.player[0]={3,300,384,2,14500,100000};
    if(players==2)c.player[1]={0,200,384,3,25500,125500};return c;
}
static ff_campaign::ReadResult Read(const wchar_t*,ff_campaign::Checkpoint& c){c=disk;return read_result;}
static int SlotIndex(const wchar_t* path){return wcsrchr(path,L'-')[1]-L'1';}
static ff_campaign::SlotStatus ReadSlot(const wchar_t* path,ff_campaign::Slot& c) {
    int i=SlotIndex(path);if(ff_campaign::Usable(slot_status[i]))c=slot_disk[i];return slot_status[i];
}
static bool WriteSlot(const wchar_t* path,const ff_campaign::Slot& c) {
    ++writes;if(write_ok){int i=SlotIndex(path);slot_disk[i]=c;slot_status[i]=ff_campaign::SlotStatus::Ready;}return write_ok;
}
static void __cdecl NativeReady(){++ready_calls;InputField<int>(0x86A528)=123;InputField<int16_t>(0x86A544)=7;}
static void __cdecl ResetInput(){InputField<int>(0x85A668)=0;}
static int __cdecl NativeStage(){CampaignStageReady();if(restart_once){restart_once=false;g_campaign_restart=true;return 1;}return stage_result;}
static int __cdecl NativeFrontend() {
    if(choose_resume){g_campaign_resume=true;g_campaign_pending_slot=g_campaign_library.Latest();g_campaign_pending=g_campaign_library.slots[g_campaign_pending_slot].current;}
    InputField<int>(0x85A6B8)=0;InputField<int>(0x8745A0)=1;
    InputField<int>(0x85A668)=1;return frontend_result;
}
static void Relative(BYTE* code,BYTE opcode,void* target) {
    code[0]=opcode;LONG relative=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-code-5);memcpy(code+1,&relative,4);
}
static int __cdecl Width(const char* text){int n=0;for(;*text;++text)n+=*text==' '?6:11;return n;}
static void __cdecl Text(const char* text,int x,int y,const BYTE*){
    CHECK(x>=0 && x+Width(text)<=320 && y>=0 && y<240);drawn_text.emplace_back(text);++draws;
}
static bool Drawn(const std::string& text){return std::find(drawn_text.begin(),drawn_text.end(),text)!=drawn_text.end();}
static void __cdecl Noop(){}
static unsigned __cdecl Buttons(int device){return menu_input[device&1];}
static void Signatures(BYTE* base,bool end=true) {
    Relative(base+0x41CBC,0xE8,base+0x2EC10);Relative(base+0x41D5F,0xE8,base+0x40D40);
    Relative(base+0x40E88,0xE8,base+0x40A40);
    Relative(base+0x415EA,0xE8,base+0x4AA50);
    Relative(base+0x4AC0C,0xE8,base+0x44A20);Relative(base+0x4AC7D,0xE8,base+0x44A20);
    const BYTE table[]={0xC1,0xE6,4,0x81,0xC6,0xE0,0xAC,0xC7,0};memcpy(base+0x2F4D8,table,sizeof(table));
    const BYTE selection[]={0xA1,0xB8,0x22,0x95,0x00};memcpy(base+0x2F623,selection,5);
    if(end)base[0x2F628]=0xC3;
}
static void NativeMenuFixture(BYTE* base,const char* executable) {
    std::ifstream file(executable,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
    CHECK(bytes.size()>0x1000);if(bytes.size()<0x1000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);bool found=false;
    auto native_bytes=[&](DWORD rva,unsigned count)->const BYTE* {
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
            const auto& s=sections[i];
            if(rva>=s.VirtualAddress && rva+count<=s.VirtualAddress+s.SizeOfRawData &&
                s.PointerToRawData+rva-s.VirtualAddress+count<=bytes.size())
                return bytes.data()+s.PointerToRawData+rva-s.VirtualAddress;
        }
        CHECK(false);return nullptr;
    };
    // Compare every displayed name with the original English level-select table.
    const auto table=native_bytes(0x6C3B8,25*4);
    if(table)for(int stage=0;stage<25;++stage) {
        DWORD address;memcpy(&address,table+stage*4,4);
        const auto label=native_bytes(address-nt->OptionalHeader.ImageBase,32);
        if(label)CHECK(std::string(reinterpret_cast<const char*>(label))==std::string(ff_campaign::StageName(stage))+" LEVEL");
    }
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(0x2F3B0>=sections[i].VirtualAddress &&
        0x2F730<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
        memcpy(base+0x2F3B0,bytes.data()+sections[i].PointerToRawData+0x2F3B0-sections[i].VirtualAddress,0x380);found=true;
    }
    CHECK(found);if(!found)return;
    // Reviewed absolute operand locations; never relocate arithmetic constants
    // or accidental byte patterns spanning adjacent instructions.
    const DWORD addresses[]={0x2f3b1,0x2f3bc,0x2f3cc,0x2f3d2,0x2f3d8,0x2f3f0,0x2f3f8,0x2f402,0x2f408,0x2f40e,
        0x2f416,0x2f420,0x2f426,0x2f42c,0x2f448,0x2f44d,0x2f457,0x2f45d,0x2f463,0x2f46d,0x2f474,0x2f47c,
        0x2f49a,0x2f4a0,0x2f4ad,0x2f4b5,0x2f4bb,0x2f4c2,0x2f4dd,0x2f4eb,0x2f4f7,0x2f4fe,0x2f518,
        0x2f52c,0x2f534,0x2f550,0x2f565,0x2f571,0x2f57a,0x2f582,0x2f588,0x2f58e,0x2f5ab,0x2f5b4,
        0x2f5bd,0x2f5c4,0x2f5ca,0x2f5e3,0x2f5ea,0x2f5f4,0x2f600,0x2f60a,0x2f624,0x2f631,0x2f63b,
        0x2f64c,0x2f656,0x2f65c,0x2f660,0x2f666,0x2f691,0x2f698,0x2f69e,0x2f6a4,0x2f6a9,0x2f6b0,
        0x2f6b6,0x2f6bc,0x2f6c1,0x2f6c7,0x2f6d0,0x2f6dc,0x2f6e7,0x2f6f4,0x2f6f8,0x2f701,0x2f707,
        0x2f70e,0x2f713,0x2f719};
    for(DWORD rva:addresses) {
        DWORD value;memcpy(&value,base+rva,4);CHECK(value>=0x400000 && value<0xC80000);
        value=DWORD(g_base)+value-0x400000;memcpy(base+rva,&value,4);
    }
    Signatures(base,false);CHECK(InstallCampaignSaves());
    Relative(base+0x49570,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x4B1C0,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x42600,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x49A70,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x2E9F0,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x48860,0xE9,reinterpret_cast<void*>(Buttons));
    const char* labels[]={"1 PLAYER GAME","OPTIONS","UNUSED","UNUSED","1 PLAYER GAME","2 PLAYER GAME","OPTIONS","UNUSED"};
    memcpy(base+0x87ACE0,labels,sizeof(labels));InputField<int>(0x6C498)=2;InputField<int>(0x6C49C)=3;
    InputField<int>(0x6F040)=0;InputField<int>(0x6F044)=1;InputField<int>(0x86D2A8)=2;
    g_campaign_library={};g_campaign_library.status[0]=ff_campaign::SlotStatus::Ready;
    g_campaign_library.slots[0]=g_campaign_library.Updated(0,Example(),true);
    for(int row=0;row<5;++row) {
        InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;InputField<int>(0x85A6B8)=26;
        InputField<int>(0x5522B8)=row;InputField<int>(0x5522E0)=InputField<int>(0x5522E4)=0;
        InputField<int>(0x5522D8)=0;g_campaign_resume=g_campaign_menu_active=false;menu_input[0]=ff_input::Confirm;menu_input[1]=0;
        CHECK(CampaignMainMenu());
        CHECK(!memcmp(base+0x87ACE0,labels,sizeof(labels)) && InputField<int>(0x6C498)==2 && InputField<int>(0x6C49C)==3);
        if(row==0)CHECK(g_campaign_resume && InputField<int>(0x5522D8)==1 && InputField<int>(0x86E9C8)==2);
        else if(row==1)CHECK(g_campaign_menu_active && g_campaign_menu.page==ff_campaign::SavePage::Load);
        else if(row<4)CHECK(g_campaign_menu_active && g_campaign_menu.page==ff_campaign::SavePage::NewGame && g_campaign_menu.players==row-1);
        else CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
    }
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;InputField<int>(0x5522B8)=0;
    InputField<int>(0x5522E0)=InputField<int>(0x5522E4)=0;g_campaign_resume=g_campaign_menu_active=false;
    menu_input[0]=0;menu_input[1]=ff_input::Confirm;CampaignMainMenu();CHECK(g_campaign_resume);
    InputField<int>(0x5522E4)=0;g_campaign_resume=false;focus=false;
    CampaignMainMenu();CHECK(!g_campaign_resume);focus=true;menu_input[1]=0;
    InputField<int>(0x5522E0)=0;menu_input[0]=ff_input::Up;CampaignMainMenu();CHECK(InputField<int>(0x5522B8)==4);
    InputField<int>(0x5522E0)=0;menu_input[0]=ff_input::Down;CampaignMainMenu();CHECK(InputField<int>(0x5522B8)==0);
    InputField<int>(0x5522E0)=ff_input::Confirm;menu_input[0]=ff_input::Confirm;g_campaign_resume=false;
    CampaignMainMenu();CHECK(!g_campaign_resume);
    InputField<int>(0x86D2A8)=1;InputField<int>(0x5522B8)=3;InputField<int>(0x5522E0)=0;
    CampaignMainMenu();CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
}

static XINPUT_STATE selection_pad;
static ff_input::Keyboard selection_keys;
static DWORD WINAPI SelectionPad(DWORD index,XINPUT_STATE* output) {
    if(index)return ERROR_DEVICE_NOT_CONNECTED;*output=selection_pad;return ERROR_SUCCESS;
}
static ff_input::Keyboard SelectionKeys(){return selection_keys;}
static unsigned __cdecl ZeroInput(int){return 0;}
static int __cdecl AvailableInput(){return 1;}
static int __cdecl NoScan(){return -1;}
static void NativeSelectionFixture(BYTE* base,const char* executable) {
    std::ifstream file(executable,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
    CHECK(bytes.size()>0x1000);if(bytes.size()<0x1000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    for(const auto range:{std::pair<DWORD,DWORD>{0x2F730,0x4F1},{0x48860,0x4B}}) {
        bool found=false;
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(range.first>=sections[i].VirtualAddress &&
            range.first+range.second<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
            memcpy(base+range.first,bytes.data()+sections[i].PointerToRawData+range.first-sections[i].VirtualAddress,range.second);found=true;
        }
        CHECK(found);if(!found)return;
    }
    // Reviewed absolute operands only; the 0x600000 camera offsets are scalar.
    const DWORD addresses[]={0x2f731,0x2f737,0x2f74b,0x2f756,0x2f75b,0x2f763,0x2f76f,0x2f776,0x2f77c,0x2f782,
        0x2f787,0x2f78e,0x2f794,0x2f79a,0x2f7a3,0x2f7b0,0x2f7b7,0x2f7bd,0x2f7c2,0x2f7c9,0x2f7cf,0x2f7d7,
        0x2f7ed,0x2f7f1,0x2f7f8,0x2f800,0x2f806,0x2f80b,0x2f811,0x2f82e,0x2f839,0x2f84f,0x2f856,0x2f861,
        0x2f86c,0x2f879,0x2f88a,0x2f891,0x2f896,0x2f8a8,0x2f8af,0x2f8b5,0x2f8bf,0x2f8d0,0x2f8d7,0x2f8dc,
        0x2f8ee,0x2f8f5,0x2f8fd,0x2f904,0x2f923,0x2f932,0x2f93c,0x2f945,0x2f95b,0x2f963,0x2f979,0x2f987,
        0x2f98d,0x2f992,0x2f99b,0x2f9a8,0x2f9b6,0x2f9bd,0x2f9c4,0x2f9cb,0x2f9d9,0x2f9df,0x2f9e6,0x2f9ec,
        0x2f9f4,0x2f9fa,0x2f9ff,0x2fa06,0x2fa0c,0x2fa19,0x2fa20,0x2fa26,0x2fa2b,0x2fa32,0x2fa38,0x2fa40,
        0x2fa55,0x2fa59,0x2fa5f,0x2fa6a,0x2fa72,0x2fa78,0x2fa7d,0x2fa83,0x2fa90,0x2faa8,0x2fab1,0x2fabb,
        0x2facd,0x2fae6,0x2fb02,0x2fb24,0x2fb2b,0x2fb35,0x2fb4c,0x2fb67,0x2fb83,0x2fba1,0x2fbab,0x2fbc0,
        0x2fbc6,0x2fbd1,0x2fbd6,0x2fbde,0x2fbed,0x2fc05,0x2fc15,0x4887a,0x48882};
    for(DWORD rva:addresses){DWORD value;memcpy(&value,base+rva,4);value=DWORD(g_base)+value-0x400000;memcpy(base+rva,&value,4);}
    Relative(base+0x48895,0xE8,reinterpret_cast<void*>(ModernAssignedJoystickThunk));
    Relative(base+0x488A0,0xE8,reinterpret_cast<void*>(ModernAssignedKeyboardThunk));
    Relative(base+0x22BE0,0xE9,reinterpret_cast<void*>(ZeroInput));Relative(base+0x22D70,0xE9,reinterpret_cast<void*>(ZeroInput));
    Relative(base+0x22BD0,0xE9,reinterpret_cast<void*>(AvailableInput));Relative(base+0x22F70,0xE9,reinterpret_cast<void*>(NoScan));
    g_modern_keyboard=g_modern_gamepad=g_modern_coop_input=true;
    g_modern_keyboard_reader=SelectionKeys;g_modern_xinput=SelectionPad;
    g_modern_controller=g_modern_second_controller=-1;
    const char* names[]={"HAWK","MACE","SMASHER","ALANA"};memcpy(base+0x8777C0,names,sizeof(names));
    auto frame=[&](){g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        reinterpret_cast<void(__cdecl*)()>(base+0x2F730)();ModernQuitKey();};
    for(int scheme:{1,2,3})for(int available:{1,2})for(bool pad:{false,true})for(int repeat=0;repeat<4;++repeat) {
        g_modern_coop_scheme=scheme;g_modern_pad_session={};g_modern_second_session={};
        InputField<int>(0x85A6B8)=26;InputField<int>(0x86D2A8)=available;InputField<int>(0x93D20)=0;
        InputField<int>(0x6F040)=0;InputField<int>(0x6F044)=1;
        InputField<int>(0x6C460)=1;InputField<int>(0x6C464)=0;InputField<int>(0x5522D8)=0;
        selection_pad={};selection_keys={};CampaignNewGame(1,1);frame();frame();
        CHECK(!g_controls_wait_release);
        if(pad)selection_pad.Gamepad.wButtons=XINPUT_GAMEPAD_B;else selection_keys.cancel=true;
        frame();CHECK(ModernMainMenu());
        selection_pad={};selection_keys={};CampaignNewGame(1,1);frame();frame();
        if(pad)selection_pad.Gamepad.wButtons=XINPUT_GAMEPAD_A;else selection_keys.confirm=true;
        frame();
        if(InputField<int>(0x5522D8)!=1)printf("Selection stalled: scheme=%d available=%d pad=%d flags=%d\n",scheme,available,int(pad),InputField<int>(0x5522B4));
        CHECK(InputField<int>(0x5522D8)==1 && InputField<int>(0x5522B4)==1);
    }
}

int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;
    wcscpy_s(g_config_path,L"fixture\\compat.ini");Signatures(base);
    base[0x41D5F]=0x90;BYTE before[5];memcpy(before,base+0x41CBC,5);
    CHECK(!InstallCampaignSaves() && !memcmp(before,base+0x41CBC,5));Signatures(base);CHECK(InstallCampaignSaves());
    CHECK(wcsstr(g_campaign_path,L"fightingforce-progress.sav")!=nullptr);
    for(bool added:{false,true})for(int row=0;row<5;++row) {
        g_campaign_menu_added=added;g_campaign_prefix=2;InputField<int>(0x86D2A8)=2;InputField<int>(0x5522B8)=row;
        CHECK(reinterpret_cast<int(__cdecl*)()>(base+0x2F623)()==CampaignNativeRow(row));
    }
    g_campaign_menu_added=false;g_campaign_read=Read;g_campaign_read_slot=ReadSlot;g_campaign_write_slot=WriteSlot;
    Relative(base+0x2EC10,0xE9,reinterpret_cast<void*>(NativeFrontend));
    Relative(base+0x484D0,0xE9,reinterpret_cast<void*>(ResetInput));
    Relative(base+0x40D40,0xE9,reinterpret_cast<void*>(NativeStage));
    Relative(base+0x40A40,0xE9,reinterpret_cast<void*>(NativeReady));
    Relative(base+0x2E9F0,0xE9,reinterpret_cast<void*>(Noop));
    Relative(base+0x4AEB0,0xE9,reinterpret_cast<void*>(Width));Relative(base+0x44A20,0xE9,reinterpret_cast<void*>(Text));
    disk=Example();read_result=ff_campaign::ReadResult::Ready;choose_resume=true;
    CHECK(CampaignFrontend()==0 && g_campaign_library.Latest()==0 && g_campaign_resume && writes==1);
    auto restored=CampaignCapture();CHECK(!memcmp(&restored,&disk,sizeof(disk)) && InputField<int>(0x85A668)==0);
    CHECK(CampaignStage()==2 && ready_calls==1 && writes==2 && !g_campaign_resume);
    CHECK(InputField<int>(0x86A528)==123 && InputField<int16_t>(0x86A544)==300);
    CHECK(CampaignStage()==2 && writes==2);
    InputField<int>(0x85A6B8)=2;CHECK(CampaignStage()==2 && writes==3 && slot_disk[0].current.stage==2 && slot_disk[0].previous.stage==1);
    CampaignNewGame(1,1);CHECK(writes==3); // Selection alone never overwrites data.
    CampaignRestore(Example(1));g_campaign_session_slot=1;g_campaign_last_stage=-1;
    CHECK(CampaignStage()==2 && writes==4 && slot_disk[0].current.stage==2 && !slot_disk[1].has_previous);
    write_ok=false;InputField<int>(0x85A6B8)=3;CampaignStage();
    CHECK(writes==5 && slot_disk[1].current.stage==1 && !strcmp(g_campaign_message,"PROGRESS SAVE FAILED"));write_ok=true;
    restart_once=true;CHECK(CampaignStage()==2 && !g_campaign_restart && InputField<int>(0x85A6B8)==3);
    CHECK(slot_disk[1].current.stage==3 && slot_disk[0].current.stage==2);
    for(int blocked=0;blocked<3;++blocked) {
        CampaignRestore(Example());g_campaign_last_stage=-1;InputField<int>(0x85A668)=0;const int writes_before=writes;
        if(blocked==0)InputField<int>(0x6F000)=0;
        if(blocked==1)InputField<int>(0x85A668)=1;
        if(blocked==2)InputField<int>(0x85A6B8)=26;
        CampaignStage();CHECK(writes==writes_before);
    }
    choose_resume=false;CampaignLoadSlots();CHECK(g_campaign_library.Latest()==1 && slot_disk[0].current.stage==2);
    CampaignResumeSlot(0,true);CHECK(g_campaign_pending.stage==1 && g_campaign_rollback);
    g_campaign_session_slot=0;CampaignRestore(g_campaign_pending);InputField<int>(0x85A668)=0;g_campaign_last_stage=-1;CampaignStage();
    CHECK(slot_disk[0].current.stage==1 && !slot_disk[0].has_previous && g_campaign_library.Latest()==0);
    CampaignRestore(Example());InputField<int16_t>(0x86A534)=-1;InputField<int16_t>(0x86A542)=0;InputField<int>(0x878890)=0;
    restored=CampaignCapture();CHECK(ff_campaign::Valid(restored) && restored.player[0].character==-1);
    // Every screen is bounded using the same native text entry points.
    for(int slot=0;slot<3;++slot)for(int row=0;row<4;++row) {
        g_campaign_menu.page=ff_campaign::SavePage::Load;g_campaign_menu.row=row;DrawCampaignMenu();
        g_campaign_menu.page=ff_campaign::SavePage::NewGame;DrawCampaignMenu();
        g_campaign_menu.page=ff_campaign::SavePage::Replace;g_campaign_menu.slot=slot;g_campaign_menu.row=row%2;DrawCampaignMenu();
        g_campaign_menu.page=ff_campaign::SavePage::Details;DrawCampaignMenu();
    }
    // Names must fit in slot rows, rewind choices, Continue previews and pause,
    // including the longest two-player summary and eliminated player slots.
    CHECK(!strcmp(ff_campaign::StageName(-1),"UNKNOWN LEVEL") && !strcmp(ff_campaign::StageName(25),"UNKNOWN LEVEL"));
    for(int stage=0;stage<25;++stage) {
        auto& slot=g_campaign_library.slots[0];slot.current=Example();slot.current.stage=stage;
        slot.current.player[0].character=2;slot.current.player[1].character=3;
        slot.previous=slot.current;slot.has_previous=true;g_campaign_library.status[0]=ff_campaign::SlotStatus::Ready;
        g_campaign_menu.slot=g_campaign_menu.row=0;
        drawn_text.clear();g_campaign_menu.page=ff_campaign::SavePage::Load;DrawCampaignMenu();
        CHECK(Drawn(std::string("SLOT 1  ")+ff_campaign::StageName(stage)));
        CHECK(Drawn(ff_campaign::StageName(stage)) && Drawn("SMASHER AND ALANA  NORMAL"));
        drawn_text.clear();g_campaign_menu.page=ff_campaign::SavePage::Details;DrawCampaignMenu();
        CHECK(Drawn(std::string("PREVIOUS  ")+ff_campaign::StageName(stage)));
        drawn_text.clear();g_campaign_menu.page=ff_campaign::SavePage::Rewind;DrawCampaignMenu();
        CHECK(Drawn(ff_campaign::StageName(stage)));
        CampaignSummary(slot.current,182);slot.current.player[0].character=-1;CampaignSummary(slot.current,182);
        g_campaign_entry=slot.current;g_campaign_session_slot=2;InputField<int>(0x8745A0)=1;
        g_move_list.page=ff_moves::Page::Closed;g_campaign_pause={};
        for(bool failed:{false,true}){g_campaign_write_failed=failed;DrawCampaignPause();}
    }
    g_campaign_menu_active=true;CHECK(ControlInputBlocked() && ModernQuitKey()==-1);CampaignCloseMenu();
    focus=false;InputField<unsigned>(0x85B560)=1234;CampaignMenuFrame();CHECK(InputField<unsigned>(0x5522C8)==1234);focus=true;
    if(argc>1){AddVectoredExceptionHandler(1,FixtureException);NativeMenuFixture(base,argv[1]);NativeSelectionFixture(base,argv[1]);}
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Campaign checkpoint runtime: FAIL":"Campaign checkpoint runtime: PASS");return failures?1:0;
}
