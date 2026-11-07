#pragma once

static bool g_guard_option_available;
static int g_guard_option_row=-1;
static int g_move_option_row=-1;
static char g_guard_option_text[32];
static const char* g_guard_option_message="";
static uintptr_t g_guard_option_setup_return,g_guard_option_text_return,g_move_option_text_return;
static const char g_move_option_text[]="MOVE LIST";
static const char g_friendly_fire_text[]="FRIENDLY FIRE";
static uintptr_t g_friendly_fire_text_return;
static int& FriendlyFireSetting(){return InputField<int>(0x85A6A4);}

static void __cdecl PreparePlayerGuardOption() {
    // The native Options callback rebuilds its row map each frame. Insert
    // before Exit using an unused row, retaining native layout and selection.
    g_guard_option_row=InputField<int>(0x6C50C);
    InputField<int>(0x6C4F8)=g_guard_option_row;
    g_move_option_row=g_guard_option_row+1;InputField<int>(0x6C4FC)=g_move_option_row;
    InputField<int>(0x6C50C)+=2;InputField<int>(0x6C510)+=2;
    strcpy_s(g_guard_option_text,"GAMEPLAY SETTINGS");
}
static void PollPlayerGuardOption() {
    if(!g_guard_option_available || InputField<uintptr_t>(0x6C47C)!=g_base+0x2FC30 ||
       (InputField<int>(0x552348)!=g_guard_option_row && InputField<int>(0x552348)!=g_move_option_row)) {g_guard_option_message="";return;}
    if(!ModernHasFocus())return;
    // Use native edge events after its navigation pass. Merge the two device
    // readers so one shared keyboard/gamepad press cannot toggle twice.
    const unsigned pressed=InputField<unsigned>(0x5522E8)|InputField<unsigned>(0x5522EC);
    if(pressed&ff_input::Confirm){
        const bool moves=InputField<int>(0x552348)==g_move_option_row;
        OpenControlMenu(g_base+0x2FC30);g_control_menu.page=moves?-3:-2;
        if(moves)OpenMoveList(false);
    }
    if(*g_guard_option_message)ControlText(g_guard_option_message,226,true);
}
static void __declspec(naked) PlayerGuardOptionSetup() {
    __asm {
        pushfd
        pushad
        call PreparePlayerGuardOption
        popad
        popfd
        mov eax,dword ptr [g_base]
        mov eax,[eax+552344h]
        jmp dword ptr [g_guard_option_setup_return]
    }
}
static void __declspec(naked) PlayerGuardOptionText() {
    __asm {
        mov edi,offset g_guard_option_text
        jmp dword ptr [g_guard_option_text_return]
    }
}
static void __declspec(naked) MoveListOptionText() {
    __asm {
        mov edi,offset g_move_option_text
        jmp dword ptr [g_move_option_text_return]
    }
}
static void __declspec(naked) FriendlyFireOptionText() {
    __asm {
        mov edi,offset g_friendly_fire_text
        jmp dword ptr [g_friendly_fire_text_return]
    }
}
static bool InstallPlayerGuardOption() {
    const BYTE setup[]={0xA1,0x44,0x23,0x95,0x00},label[]={0x8B,0x3D,0x90,0x22,0x95,0x00};
    const BYTE unused[]={0x3B,0x05,0xF8,0xC4,0x46,0x00};
    const BYTE moves_label[]={0x8B,0x3D,0x38,0x22,0x95,0x00},moves_unused[]={0x3B,0x05,0xFC,0xC4,0x46,0x00};
    const BYTE friendly_label[]={0x8B,0x3D,0xB0,0x22,0x95,0x00};
    const BYTE title_y[]={0x6A,0x14},list_anchor[]={0xBB,0x80,0,0,0},row_offset[]={0x8D,0x0C,0x40,0xC1,0xE1,0x02};
    if(memcmp(reinterpret_cast<void*>(g_base+0x2FD6A),setup,sizeof(setup)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2FFCE),label,sizeof(label)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x304E1),unused,sizeof(unused)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2FFE1),moves_label,sizeof(moves_label)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x304FB),moves_unused,sizeof(moves_unused)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x30031),friendly_label,sizeof(friendly_label)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2FE34),title_y,sizeof(title_y)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2FE41),list_anchor,sizeof(list_anchor)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2FE4B),row_offset,sizeof(row_offset))) {
        Log("Player Block Options signatures differ; native Options retained.\r\n");return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+0x2FD6A);const SIZE_T size=0x30037-0x2FD6A;DWORD previous;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&previous))return false;
    g_guard_option_setup_return=g_base+0x2FD6F;g_guard_option_text_return=g_base+0x2FFD4;
    g_move_option_text_return=g_base+0x2FFE7;
    g_friendly_fire_text_return=g_base+0x30037;
    auto jump=[](DWORD rva,void* target,unsigned length) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);memset(code,0x90,length);code[0]=0xE9;
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
    };
    jump(0x2FD6A,reinterpret_cast<void*>(PlayerGuardOptionSetup),sizeof(setup));
    jump(0x2FFCE,reinterpret_cast<void*>(PlayerGuardOptionText),sizeof(label));
    jump(0x2FFE1,reinterpret_cast<void*>(MoveListOptionText),sizeof(moves_label));
    jump(0x30031,reinterpret_cast<void*>(FriendlyFireOptionText),sizeof(friendly_label));
    InputField<BYTE>(0x2FE35)=8;
    InputField<DWORD>(0x2FE42)=140;
    // Start the list nine units earlier per row to center its 18-unit spacing.
    const BYTE center_rows[]={0x8D,0x0C,0xC0,0x90,0x90,0x90};
    memcpy(reinterpret_cast<void*>(g_base+0x2FE4B),center_rows,sizeof(center_rows));
    DWORD ignored;VirtualProtect(start,size,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    g_guard_option_available=true;return true;
}
