#include "../src/campaign_save_file.h"
#include <string>
#include <limits.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static ff_campaign::Checkpoint Example() {
    ff_campaign::Checkpoint c;c.stage=7;c.players=2;c.mode=1;
    c.player[0]={3,264,384,2,34500,100000};c.player[1]={0,188,384,1,120000,220000};return c;
}
int main() {
    using namespace ff_campaign;
    auto c=Example();Checkpoint result;uint8_t data[Bytes];
    CHECK(Encode(c,data) && Decode(data,Bytes,result));CHECK(!memcmp(&c,&result,sizeof(c)));
    for(unsigned i=0;i<Bytes;++i) {
        data[i]^=1;result=Example();result.stage=3;
        CHECK(!Decode(data,Bytes,result) && result.stage==3);data[i]^=1;
    }
    for(unsigned n=0;n<Bytes;++n)CHECK(!Decode(data,n,result));
    CHECK(!Decode(data,Bytes+1,result));
    for(int stage=0;stage<25;++stage)for(int hero=0;hero<4;++hero) {
        c=Example();c.stage=stage;c.player[0].character=hero;CHECK(Encode(c,data));
    }
    c=Example();c.player[0]={};CHECK(Encode(c,data)); // Surviving second player keeps its slot.
    c.player[1]={};CHECK(!Valid(c));
    for(int invalid=0;invalid<11;++invalid) {
        c=Example();
        switch(invalid) {
        case 0:c.stage=25;break;case 1:c.stage=-1;break;case 2:c.players=3;break;
        case 3:c.mode=0;break;case 4:c.difficulty=3;break;case 5:c.player[0].character=4;break;
        case 6:c.player[0].health=385;break;case 7:c.player[0].lives=-1;break;
        case 8:c.player[0].next_life=-1;break;case 9:c.player[0].score=-1;break;
        case 10:c.player[0].character=-1;break;
        }
        CHECK(!Encode(c,data));
    }
    wchar_t folder[MAX_PATH],path[MAX_PATH],root[MAX_PATH];GetTempPathW(MAX_PATH,root);
    swprintf_s(folder,L"%sff-campaign-%lu",root,GetCurrentProcessId());CHECK(CreateDirectoryW(folder,nullptr));
    swprintf_s(path,L"%s\\progress.sav",folder);
    result=Example();CHECK(Read(path,result)==ReadResult::Missing);
    c=Example();CHECK(Write(path,c));CHECK(Read(path,result)==ReadResult::Ready && !memcmp(&c,&result,sizeof(c)));
    c.stage=12;CHECK(Write(path,c));CHECK(Read(path,result)==ReadResult::Ready && result.stage==12);
    // A locked destination must retain the previous complete checkpoint.
    HANDLE lock=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    c.stage=18;CHECK(!Write(path,c));CloseHandle(lock);
    CHECK(Read(path,result)==ReadResult::Ready && result.stage==12);
    c.stage=99;CHECK(!Write(path,c));CHECK(Read(path,result)==ReadResult::Ready && result.stage==12);
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    DWORD written;CHECK(WriteFile(file,"bad",3,&written,nullptr));CHECK(SetEndOfFile(file));CloseHandle(file);
    CHECK(Read(path,result)==ReadResult::Invalid);CHECK(DeleteFileW(path));CHECK(RemoveDirectoryW(folder));
    puts(failures?"Campaign checkpoint storage: FAIL":"Campaign checkpoint storage: PASS");return failures?1:0;
}
