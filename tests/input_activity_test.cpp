#include <stdio.h>
#include "../src/input_activity.h"
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_input;Activity a;Pad pad;pad.connected=true;
    a.Controller(2,pad,7849,8689);CHECK(a.device==Device::Keyboard && !a.serial);
    pad.rx=9000;a.Controller(2,pad,7849,8689);CHECK(!a.serial); // Resting stick noise is not activity.
    pad.buttons=A;a.Controller(2,pad,7849,8689);CHECK(a.device==Device::Gamepad);
    Keyboard keys;keys.pause=true;a.Keyboard(0,keys);CHECK(a.device==Device::Keyboard);
    const auto serial=a.serial;a.Controller(2,pad,7849,8689);CHECK(a.serial==serial); // Held button cannot steal hints back.
    keys={};a.Keyboard(0,keys);CHECK(a.serial==serial);
    pad.buttons=0;pad.rx=25000;a.Controller(2,pad,7849,8689);CHECK(a.device==Device::Gamepad);
    uint8_t raw[256]={};raw[0xD0]=128;a.Physical(raw);CHECK(a.device==Device::Keyboard);
    a.Use(Device::Gamepad);a.Physical(raw);CHECK(a.device==Device::Gamepad);
    a.Reset();CHECK(a.device==Device::Gamepad && !a.pads[2].connected);
    a.Controller(9,pad,7849,8689);a.Keyboard(9,keys);CHECK(!a.serial);
    puts(failures?"Input activity: FAIL":"Input activity: PASS");return failures?1:0;
}
