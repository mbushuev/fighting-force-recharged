#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,writes,ready_calls,stage_result=2,frontend_result;
static bool write_ok=true,focus=true,choose_resume;
static ff_campaign::ReadResult read_result=ff_campaign::ReadResult::Missing;
static ff_campaign::Checkpoint disk;
static unsigned menu_input[2];static int draws;
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
static bool Write(const wchar_t*,const ff_campaign::Checkpoint& c){++writes;if(write_ok)disk=c;return write_ok;}
static void __cdecl NativeReady(){++ready_calls;InputField<int>(0x86A528)=123;InputField<int16_t>(0x86A544)=7;}
static void __cdecl ResetInput(){InputField<int>(0x85A668)=0;}
static int __cdecl NativeStage(){CampaignStageReady();return stage_result;}
static int __cdecl NativeFrontend() {
    if(choose_resume){g_campaign_resume=true;g_campaign_pending=g_campaign_checkpoint;}
    InputField<int>(0x85A6B8)=0;InputField<int>(0x8745A0)=1;
    InputField<int>(0x85A668)=1;return frontend_result;
}
static void Relative(BYTE* code,BYTE opcode,void* target) {
    code[0]=opcode;LONG relative=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-code-5);memcpy(code+1,&relative,4);
}
static int __cdecl Width(const char* text){return int(strlen(text)*9);}
static void __cdecl Text(const char*,int x,int y,const BYTE*){CHECK(x>=0 && y>=0 && y<240);++draws;}
static void __cdecl Noop(){}
static unsigned __cdecl Buttons(int device){return menu_input[device&1];}
static void Signatures(BYTE* base,bool end=true) {
    Relative(base+0x41CBC,0xE8,base+0x2EC10);Relative(base+0x41D5F,0xE8,base+0x40D40);
    Relative(base+0x40E88,0xE8,base+0x40A40);
    const BYTE selection[]={0xA1,0xB8,0x22,0x95,0x00};memcpy(base+0x2F623,selection,5);
    if(end)base[0x2F628]=0xC3;
}
static void NativeMenuFixture(BYTE* base,const char* executable) {
    std::ifstream file(executable,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
    CHECK(bytes.size()>0x1000);if(bytes.size()<0x1000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);bool found=false;
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
    g_campaign_available=true;g_campaign_checkpoint=Example();
    for(int row=0;row<4;++row) {
        InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;InputField<int>(0x85A6B8)=26;
        InputField<int>(0x5522B8)=row;InputField<int>(0x5522E0)=InputField<int>(0x5522E4)=0;
        InputField<int>(0x5522D8)=0;g_campaign_resume=false;menu_input[0]=ff_input::Confirm;menu_input[1]=0;
        CHECK(CampaignMainMenu());
        CHECK(!memcmp(base+0x87ACE0,labels,sizeof(labels)) && InputField<int>(0x6C498)==2 && InputField<int>(0x6C49C)==3);
        if(row==0)CHECK(g_campaign_resume && InputField<int>(0x5522D8)==1 && InputField<int>(0x86E9C8)==2);
        else if(row<3)CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2F730 && InputField<int>(0x86E9C8)==row);
        else CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
    }
    // Either controller can confirm Continue, while focus loss prevents resume.
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;InputField<int>(0x5522B8)=0;
    InputField<int>(0x5522E0)=InputField<int>(0x5522E4)=0;g_campaign_resume=false;
    menu_input[0]=0;menu_input[1]=ff_input::Confirm;CampaignMainMenu();CHECK(g_campaign_resume);
    InputField<int>(0x5522E4)=0;g_campaign_resume=false;focus=false;
    CampaignMainMenu();CHECK(!g_campaign_resume);focus=true;menu_input[1]=0;
    // Native navigation wraps through all rows; held confirmation cannot fire twice.
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;InputField<int>(0x5522B8)=0;
    InputField<int>(0x5522E0)=0;menu_input[0]=ff_input::Up;CampaignMainMenu();CHECK(InputField<int>(0x5522B8)==3);
    InputField<int>(0x5522E0)=0;menu_input[0]=ff_input::Down;CampaignMainMenu();CHECK(InputField<int>(0x5522B8)==0);
    InputField<int>(0x5522E0)=ff_input::Confirm;menu_input[0]=ff_input::Confirm;g_campaign_resume=false;
    CampaignMainMenu();CHECK(!g_campaign_resume);
    // The two-row native table also retains its Options action after insertion.
    InputField<int>(0x86D2A8)=1;InputField<int>(0x5522B8)=2;InputField<int>(0x5522E0)=0;
    CampaignMainMenu();CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;
    wcscpy_s(g_config_path,L"fixture\\compat.ini");Signatures(base);
    base[0x41D5F]=0x90;BYTE before[5];memcpy(before,base+0x41CBC,5);
    CHECK(!InstallCampaignSaves() && !memcmp(before,base+0x41CBC,5));Signatures(base);CHECK(InstallCampaignSaves());
    CHECK(wcsstr(g_campaign_path,L"fightingforce-progress.sav")!=nullptr);
    for(bool added:{false,true})for(int row=0;row<4;++row) {
        g_campaign_menu_added=added;InputField<int>(0x5522B8)=row;
        CHECK(reinterpret_cast<int(__cdecl*)()>(base+0x2F623)()==row-int(added));
    }
    g_campaign_menu_added=false;g_campaign_read=Read;g_campaign_write=Write;
    Relative(base+0x2EC10,0xE9,reinterpret_cast<void*>(NativeFrontend));
    Relative(base+0x484D0,0xE9,reinterpret_cast<void*>(ResetInput));
    Relative(base+0x40D40,0xE9,reinterpret_cast<void*>(NativeStage));
    Relative(base+0x40A40,0xE9,reinterpret_cast<void*>(NativeReady));
    Relative(base+0x4AEB0,0xE9,reinterpret_cast<void*>(Width));Relative(base+0x44A20,0xE9,reinterpret_cast<void*>(Text));
    disk=Example();read_result=ff_campaign::ReadResult::Ready;choose_resume=true;
    CHECK(CampaignFrontend()==0 && g_campaign_available && g_campaign_resume);
    auto restored=CampaignCapture();CHECK(!memcmp(&restored,&disk,sizeof(disk)) && InputField<int>(0x85A668)==0);
    CHECK(CampaignStage()==2 && ready_calls==1 && writes==1 && !g_campaign_resume);
    CHECK(InputField<int>(0x86A528)==123 && InputField<int16_t>(0x86A544)==300); // Keep native spawn, synchronize health bars.
    CHECK(CampaignStage()==2 && writes==1); // A retry at the same stage cannot move its checkpoint.
    InputField<int>(0x85A6B8)=2;CHECK(CampaignStage()==2 && writes==2 && disk.stage==2);
    for(int blocked=0;blocked<3;++blocked) {
        CampaignRestore(Example());g_campaign_last_stage=-1;InputField<int>(0x85A668)=0;
        if(blocked==0)InputField<int>(0x6F000)=0;
        if(blocked==1)InputField<int>(0x85A668)=1;
        if(blocked==2)InputField<int>(0x85A6B8)=26;
        CampaignStage();CHECK(writes==2);
    }
    CampaignRestore(Example(1));InputField<int>(0x85A668)=0;g_campaign_last_stage=-1;write_ok=false;CampaignStage();
    CHECK(writes==3 && disk.stage==2 && !strcmp(g_campaign_message,"PROGRESS SAVE FAILED"));write_ok=true;
    CampaignRestore(Example());InputField<int16_t>(0x86A534)=-1;InputField<int16_t>(0x86A542)=0;InputField<int>(0x878890)=0;
    restored=CampaignCapture();CHECK(ff_campaign::Valid(restored) && restored.player[0].character==-1 && restored.player[1].character==0);
    for(auto outcome:{ff_campaign::ReadResult::Missing,ff_campaign::ReadResult::Invalid,ff_campaign::ReadResult::Unavailable}) {
        read_result=outcome;choose_resume=false;CampaignFrontend();CHECK(!g_campaign_available && !g_campaign_resume);
    }
    read_result=ff_campaign::ReadResult::Ready;CampaignFrontend();
    CHECK(!strcmp(g_campaign_message,"PROGRESS SAVE FAILED"));
    if(argc>1){AddVectoredExceptionHandler(1,FixtureException);NativeMenuFixture(base,argv[1]);}
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Campaign checkpoint runtime: FAIL":"Campaign checkpoint runtime: PASS");return failures?1:0;
}
