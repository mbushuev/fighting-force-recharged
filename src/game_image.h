#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <string.h>
#pragma comment(lib,"advapi32.lib")

namespace ff_image {
enum class Kind { Unsupported, Retail, Rip };
static constexpr DWORD ProbeRvas[]={0x366F1,0x3673F,0x3678D,0x367D4};
inline Kind Classify(const BYTE* base,size_t length) {
    if(!base || length<sizeof(IMAGE_NT_HEADERS32))return Kind::Unsupported;
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 ||
        static_cast<size_t>(dos->e_lfanew)>length-sizeof(IMAGE_NT_HEADERS32))return Kind::Unsupported;
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.TimeDateStamp!=0x34ED7B34 || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->OptionalHeader.ImageBase!=0x400000)return Kind::Unsupported;
    const BYTE expected[]={0xB8,0x02,0xB1,0,0,0xCD,0x1A,0x72,0x07};
    Kind kind=Kind::Unsupported;
    for(DWORD rva:ProbeRvas) {
        if(rva+4>length)return Kind::Unsupported;
        const BYTE* code=base+rva-5;
        if(memcmp(code,expected,5) || memcmp(code+7,expected+7,2))return Kind::Unsupported;
        Kind probe=code[5]==0xCD && code[6]==0x1A?Kind::Retail:
            code[5]==0x90 && code[6]==0x90?Kind::Rip:Kind::Unsupported;
        if(probe==Kind::Unsupported || (kind!=Kind::Unsupported && probe!=kind))return Kind::Unsupported;
        kind=probe;
    }
    return kind;
}
inline bool FileHasHash(const wchar_t* path,const BYTE (&expected)[32]) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;bool ok=false;
    if(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) {
        BYTE buffer[16384];DWORD count=0;ok=true;
        for(;;) {
            if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr)) {ok=false;break;}
            if(!count)break;
            if(!CryptHashData(hash,buffer,count,0)) {ok=false;break;}
        }
        BYTE digest[32];DWORD bytes=sizeof(digest);
        ok=ok && CryptGetHashParam(hash,HP_HASHVAL,digest,&bytes,0) && bytes==32 && !memcmp(digest,expected,32);
    }
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(file);return ok;
}
inline bool IsReviewedRip(const wchar_t* path) {
    const BYTE digest[]={0xBF,0x1B,0xD0,0x2D,0xC9,0xB6,0x50,0x8E,0xDD,0x76,0xEF,0xA3,0x6E,0x39,0xA6,0xCC,
        0x1B,0x82,0x05,0x5F,0x7D,0x31,0xCC,0xDA,0x32,0x9C,0x1A,0xE5,0x57,0x96,0xFD,0xA3};
    return FileHasHash(path,digest);
}
inline bool RestoreRipProbes(BYTE* base,size_t length) {
    if(Classify(base,length)!=Kind::Rip)return false;
    // Restore only process memory so the existing exception handler can report
    // the original BIOS error path. The supplied executable is never rewritten.
    DWORD previous;const DWORD start=ProbeRvas[0],end=ProbeRvas[3]+2;
    if(!VirtualProtect(base+start,end-start,PAGE_EXECUTE_READWRITE,&previous))return false;
    for(DWORD rva:ProbeRvas) {base[rva]=0xCD;base[rva+1]=0x1A;}
    DWORD ignored;VirtualProtect(base+start,end-start,previous,&ignored);
    FlushInstructionCache(GetCurrentProcess(),base+start,end-start);return true;
}
}
