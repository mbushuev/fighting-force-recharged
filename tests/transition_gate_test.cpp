#include "../src/transition_gate.h"
#include "../src/render_size.h"
#include <stdio.h>
static int failures;
#define CHECK(condition) do {if(!(condition)){printf("FAIL line %d: %s\n",__LINE__,#condition);++failures;}}while(0)
int main() {
    TransitionGate gate;
    CHECK(!gate.PollGame());CHECK(gate.Begin(TransitionSource::Game));
    CHECK(!gate.PollGame());gate.SceneComplete();CHECK(!gate.PollGame());
    CHECK(!gate.Begin(TransitionSource::Game));CHECK(gate.scenes==1);
    gate.SceneComplete();CHECK(gate.PollGame());CHECK(!gate.PollGame());
    CHECK(gate.Begin(TransitionSource::Movie));
    gate.SceneComplete();gate.SceneComplete();CHECK(!gate.PollGame());
    CHECK(!gate.MovieFrame(0,0));CHECK(!gate.MovieFrame(0,1));
    CHECK(!gate.MovieFrame(2,2));CHECK(gate.MovieFrame(1,2));CHECK(!gate.MovieFrame(2,3));
    CHECK(gate.Begin(TransitionSource::Movie));
    CHECK(gate.Begin(TransitionSource::Game));CHECK(!gate.MovieFrame(1,3));
    CHECK(!gate.PollGame());gate.SceneComplete();gate.SceneComplete();CHECK(gate.PollGame());
    CHECK(gate.Begin(TransitionSource::Splash));gate.SceneComplete();gate.SceneComplete();
    CHECK(!gate.PollGame());CHECK(!gate.MovieFrame(1,2));CHECK(gate.SplashFrame());CHECK(!gate.SplashFrame());
    CHECK(gate.Begin(TransitionSource::Game));CHECK(!gate.SplashFrame());
    CHECK(gate.Begin(TransitionSource::Loading));gate.SceneComplete();gate.SceneComplete();
    CHECK(!gate.PollGame() && !gate.SplashFrame() && !gate.MovieFrame(1,2));
    CHECK(gate.LoadingFrame() && !gate.LoadingFrame());
    CHECK(gate.Begin(TransitionSource::Game) && !gate.LoadingFrame());
    auto size=RetailInitialRenderRect(800,600);CHECK(size.right+1==800 && size.bottom+1==600);
    size=RetailInitialRenderRect(2880,1800);CHECK(size.right+1==2400 && size.bottom+1==1800);
    size=RetailInitialRenderRect(0,0);CHECK(size.right+1==800 && size.bottom+1==600);
    printf("Transition source/lifetime checks: %s\n",failures?"FAILED":"PASS");
    return failures?1:0;
}
