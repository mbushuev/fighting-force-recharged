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
    printf("Modern input mapping: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
