#pragma once
#include <windows.h>
#include <stdio.h>
#include "campaign_save.h"

namespace ff_campaign {
enum class ReadResult { Missing,Ready,Invalid,Unavailable };
inline ReadResult Read(const wchar_t* path,Checkpoint& output) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND?ReadResult::Missing:ReadResult::Unavailable;
    uint8_t data[Bytes];DWORD read=0;LARGE_INTEGER size;
    const bool ok=GetFileSizeEx(file,&size) && size.QuadPart==Bytes &&
        ReadFile(file,data,Bytes,&read,nullptr) && read==Bytes;
    CloseHandle(file);
    return ok && Decode(data,Bytes,output)?ReadResult::Ready:ReadResult::Invalid;
}
inline bool Write(const wchar_t* path,const Checkpoint& checkpoint) {
    uint8_t data[Bytes];if(!Encode(checkpoint,data))return false;
    wchar_t temporary[MAX_PATH];
    if(swprintf_s(temporary,L"%s.%lu.tmp",path,GetCurrentProcessId())<0)return false;
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    bool ok=WriteFile(file,data,Bytes,&written,nullptr) && written==Bytes && FlushFileBuffers(file);
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);return ok;
}
}
