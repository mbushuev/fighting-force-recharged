#include <stdio.h>
#include "../src/controller_feedback.h"
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_feedback;Mixer m;
    for(unsigned s=0;s<4;++s)CHECK(m.Read(s,100)==Motors{});
    m.Add(1,Hit,100);CHECK(m.Read(0,100)==Motors{} && m.Read(1,100).high==18000);
    m.Add(1,Hit,180);CHECK(m.Read(1,181).high<1800 && m.Read(1,185)==Motors{});
    m.Add(1,Hurt,200);m.Add(1,Block,200);
    CHECK(m.Read(1,200).low==26000 && m.Read(1,200).high==22000);
    CHECK(m.Read(1,280).high<11000 && m.Read(1,350)==Motors{});
    m.Add(0,Block,400);m.Add(3,Hurt,400);m.Clear(0);
    CHECK(m.Read(0,400)==Motors{} && m.Read(3,400).low>0);
    CHECK(m.Read(3,399)==Motors{});m.Clear();CHECK(m.Read(3,400)==Motors{});
    m.Add(99,Hit,500);CHECK(m.Read(99,500)==Motors{});
    puts(failures?"Controller feedback: FAIL":"Controller feedback: PASS");return failures?1:0;
}
