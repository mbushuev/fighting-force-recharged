#include "../src/campaign_slots_file.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static ff_campaign::Checkpoint Point(int stage=0) {
    ff_campaign::Checkpoint c;c.stage=stage;c.player[0]={3,300,384,2,1000,100000};return c;
}
int main() {
    using namespace ff_campaign;
    Library library;CHECK(library.Latest()==-1 && library.Empty()==0);
    Slot s=library.Updated(0,Point());CHECK(Valid(s) && !s.has_previous && s.order==1);
    library.slots[0]=s;library.status[0]=SlotStatus::Ready;
    s=library.Updated(0,Point(1));CHECK(s.has_previous && s.previous.stage==0 && s.current.stage==1);
    library.slots[0]=s;s=library.Updated(0,Point(1));CHECK(s.previous.stage==0); // Repeated load keeps history.
    library.slots[1]=library.Updated(1,Point(3),true);library.status[1]=SlotStatus::Ready;
    CHECK(library.Latest()==1 && library.Empty()==2);
    library.slots[0]=library.Updated(0,Point(1));CHECK(library.Latest()==0);
    s=library.Updated(0,library.slots[0].previous,false,true);CHECK(!s.has_previous && s.current.stage==0);
    s=library.Updated(0,Point(),true);CHECK(!s.has_previous && s.current.stage==0);
    uint8_t bytes[SlotBytes];Slot result;CHECK(Encode(s,bytes) && Decode(bytes,SlotBytes,result));
    for(unsigned i=0;i<SlotBytes;++i){bytes[i]^=1;CHECK(!Decode(bytes,SlotBytes,result));bytes[i]^=1;}
    for(unsigned n=0;n<SlotBytes;++n)CHECK(!Decode(bytes,n,result));
    s.has_previous=true;s.previous=s.current;CHECK(!Encode(s,bytes));
    s=library.slots[0];s.order=0;CHECK(!Encode(s,bytes));s.order=UINT64_MAX;CHECK(!Encode(s,bytes));
    library.status[0]=SlotStatus::Unavailable;CHECK(library.Latest()==1 && library.Empty()==2);
    wchar_t root[MAX_PATH],folder[MAX_PATH],path[MAX_PATH],backup[MAX_PATH];GetTempPathW(MAX_PATH,root);
    swprintf_s(folder,L"%sff-slots-%lu",root,GetCurrentProcessId());CHECK(CreateDirectoryW(folder,nullptr));
    swprintf_s(path,L"%s\\slot.sav",folder);swprintf_s(backup,L"%s.bak",path);
    CHECK(ReadSlot(path,result)==SlotStatus::Missing);s=library.slots[1];CHECK(WriteSlot(path,s));
    CHECK(ReadSlot(path,result)==SlotStatus::Ready && result.current.stage==3);
    s.current.stage=4;s.order++;CHECK(WriteSlot(path,s));CHECK(ReadSlotRecord(backup,result)==SlotStatus::Ready && result.current.stage==3);
    HANDLE lock=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    s.current.stage=5;CHECK(!WriteSlot(path,s));CloseHandle(lock);
    CHECK(ReadSlot(path,result)==SlotStatus::Ready && result.current.stage==4);
    // Damaged primary recovers its own slot without changing unrelated saves.
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    DWORD count;CHECK(WriteFile(file,"BAD",3,&count,nullptr));CloseHandle(file);
    CHECK(ReadSlot(path,result)==SlotStatus::Recovered && result.current.stage==4);
    CHECK(WriteSlot(path,result));CHECK(ReadSlot(path,result)==SlotStatus::Ready && result.current.stage==4);
    CHECK(ReadSlotRecord(backup,result)==SlotStatus::Ready && result.current.stage==4);
    CHECK(DeleteFileW(path));CHECK(ReadSlot(path,result)==SlotStatus::Recovered);
    CHECK(DeleteFileW(backup));CHECK(RemoveDirectoryW(folder));
    puts(failures?"Campaign slots: FAIL":"Campaign slots: PASS");return failures?1:0;
}
