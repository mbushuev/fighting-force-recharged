#include "../src/modern_input.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_input;
    Keyboard k;k.up=k.right=k.jump=k.run=true;
    CHECK(Gameplay(k)==(Up|Right|Jump|Run));
    CHECK((Gameplay(k)&Pause)==0); // Jump never becomes the native pause command.
    k.down=true;k.left=true;CHECK((Gameplay(k)&15)==0);
    k={};k.punch=k.kick=k.backfist=true;CHECK(Gameplay(k)==(Punch|Kick|Backfist));
    k={};CHECK(Gameplay(k)==0); // Released keys retain no action latch.
    k.jump=true;CHECK(Menu(k)==Confirm && Menu(k,false)==0);
    k={};k.cancel=true;CHECK(Menu(k)==Cancel);
    ConfirmGate gate;gate.Begin(true,true);
    CHECK(gate.Read(true,true)==-1);CHECK(gate.Read(false,false)==-1);
    CHECK(gate.Read(true,false)==0x15);CHECK(gate.Read(true,false)==-1);
    CHECK(gate.Read(false,true)==0x31);
    PauseGate pause;CHECK(pause.Read(true,false)==Pause);
    CHECK(pause.Read(true,true)==0);CHECK(pause.Read(false,true)==0);
    CHECK(pause.Read(true,true)==(Left|Confirm));
    Pad pad;pad.connected=true;pad.buttons=A|X|RightShoulder;pad.lx=24000;pad.ly=24000;
    CHECK(Gameplay(pad,7849)==(Up|Right|Jump|Punch|Run));
    pad.buttons=B|Y;pad.lx=pad.ly=0;CHECK(Gameplay(pad,7849)==(Backfist|Kick));
    CHECK(Menu(pad,7849)==Cancel);
    pad.buttons=LeftThumb;CHECK(Gameplay(pad,7849)==0);
    k={};k.first_person=true;CHECK(Gameplay(k)==0 && Menu(k)==0);
    pad.buttons=Start;CHECK(Menu(pad,7849)==Confirm && Menu(pad,7849,false)==0);
    pad={};pad.connected=true;pad.lx=5000;pad.ly=5000;CHECK(Gameplay(pad,7849)==0);
    pad.lx=32767;pad.ly=1000;CHECK(Gameplay(pad,7849)==Right);
    pad.lx=-32768;pad.ly=32767;CHECK(Gameplay(pad,7849)==(Up|Left));
    CHECK(fabs(Stick(32767,32767,8689).x-0.70710678118)<0.00001);
    pad.lx=pad.ly=0;pad.buttons=DpadUp|DpadDown|DpadLeft|DpadRight;CHECK(Gameplay(pad,7849)==0);
    PadSession session;int slot=2,reads=0;Pad physical;physical.connected=true;physical.buttons=A;
    auto reader=[&](int i,Pad& out){++reads;if(i!=slot)return false;out=physical;return true;};
    CHECK(session.Read(100,true,-1,0,7849,8689,reader).buttons==0 && session.selected==2);
    CHECK(reads==3);CHECK(session.Read(101,true,-1,0,7849,8689,reader).buttons==0 && reads==3);
    physical.buttons=0;session.Read(104,true,-1,0,7849,8689,reader);CHECK(session.ready);
    physical.buttons=X;CHECK(session.Read(108,true,-1,0,7849,8689,reader).buttons==X);
    CHECK(!session.Read(109,false,-1,0,7849,8689,reader).connected);
    CHECK(session.Read(110,true,-1,0,7849,8689,reader).buttons==0 && !session.ready);
    physical.buttons=0;session.Read(114,true,-1,0,7849,8689,reader);
    physical.buttons=A;CHECK(session.Read(118,true,-1,1,7849,8689,reader).buttons==0);
    slot=-1;CHECK(!session.Read(122,true,-1,1,7849,8689,reader).connected && session.selected==-1);
    const int before=reads;session.Read(400,true,-1,1,7849,8689,reader);CHECK(reads==before);
    slot=1;physical.buttons=0;session.Read(622,true,-1,1,7849,8689,reader);CHECK(session.selected==1);
    PadSession fixed;fixed.Read(700,true,0,0,7849,8689,reader);CHECK(fixed.selected==-1);
    printf("Modern input mapping: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
