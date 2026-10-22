#include <stdio.h>
#include "../src/controller_feedback.h"
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_feedback;Mixer m;
    for(unsigned s=0;s<4;++s)CHECK(m.Read(s,100)==Motors{});
    m.Add(1,Hit,100);CHECK(m.Read(0,100)==Motors{} && m.Read(1,100).high==36000);
    CHECK(m.Read(1,160).low==20000 && m.Read(1,160).high==36000);
    m.Add(1,Hit,180);CHECK(m.Read(1,216).high<3600 && m.Read(1,220)==Motors{});
    m.Add(1,Hurt,200);m.Add(1,Block,200);
    CHECK(m.Read(1,200).low==46000 && m.Read(1,200).high==42000);
    CHECK(m.Read(1,310).high<23000 && m.Read(1,390)==Motors{});
    m.Add(0,Block,400);m.Add(3,Hurt,400);m.Clear(0);
    CHECK(m.Read(0,400)==Motors{} && m.Read(3,400).low>0);
    CHECK(m.Read(3,399)==Motors{});m.Clear();CHECK(m.Read(3,400)==Motors{});
    m.Add(2,Block,400);
    CHECK(m.Read(2,450).low==14000 && m.Read(2,450).high==42000);
    CHECK(m.Read(2,499).high>0 && m.Read(2,500)==Motors{});
    m.Add(99,Hit,500);CHECK(m.Read(99,500)==Motors{});
    puts(failures?"Controller feedback: FAIL":"Controller feedback: PASS");return failures?1:0;
}
