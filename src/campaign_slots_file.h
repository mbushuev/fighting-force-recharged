#pragma once
#include "campaign_slots.h"
#include "campaign_save_file.h"

namespace ff_campaign {
inline SlotStatus ReadSlotRecord(const wchar_t* path,Slot& output) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND?SlotStatus::Missing:SlotStatus::Unavailable;
    uint8_t bytes[SlotBytes];DWORD count=0;LARGE_INTEGER size;
    const bool ok=GetFileSizeEx(file,&size) && size.QuadPart==SlotBytes &&
        ReadFile(file,bytes,SlotBytes,&count,nullptr) && count==SlotBytes;
    CloseHandle(file);return ok && Decode(bytes,SlotBytes,output)?SlotStatus::Ready:SlotStatus::Invalid;
}
inline SlotStatus ReadSlot(const wchar_t* path,Slot& output) {
    const auto primary=ReadSlotRecord(path,output);if(primary==SlotStatus::Ready)return primary;
    wchar_t backup[MAX_PATH];if(swprintf_s(backup,L"%s.bak",path)<0)return SlotStatus::Unavailable;
    const auto recovered=ReadSlotRecord(backup,output);
    if(recovered==SlotStatus::Ready)return SlotStatus::Recovered;
    if(primary==SlotStatus::Missing && recovered!=SlotStatus::Missing)return recovered;
    return primary;
}
inline bool WriteSlot(const wchar_t* path,const Slot& slot) {
    uint8_t data[SlotBytes];if(!Encode(slot,data))return false;
    // Back up only a verified primary. Never replace a good recovery copy
    // with corrupt or inaccessible data when repairing a damaged main file.
    Slot old;const auto existing=ReadSlotRecord(path,old);
    if(existing==SlotStatus::Unavailable)return false;
    if(existing==SlotStatus::Ready) {
        wchar_t backup[MAX_PATH];uint8_t previous[SlotBytes];
        if(swprintf_s(backup,L"%s.bak",path)<0 || !Encode(old,previous) || !WriteRecord(backup,previous,SlotBytes))return false;
    }
    return WriteRecord(path,data,SlotBytes);
}
}
