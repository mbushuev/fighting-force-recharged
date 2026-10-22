#pragma once
#include "controller_feedback.h"
using ModernXInputSet=DWORD(WINAPI*)(DWORD,XINPUT_VIBRATION*);
static ModernXInputSet g_modern_xinput_set;
static SRWLOCK g_feedback_lock=SRWLOCK_INIT;
static ff_feedback::Mixer g_feedback_mixer;
static ff_feedback::Motors g_feedback_sent[4];
static HANDLE g_feedback_timer;
static void WriteControllerFeedback(uint64_t now,bool focused) {
    AcquireSRWLockExclusive(&g_feedback_lock);
    if(!focused)g_feedback_mixer.Clear();
    for(unsigned slot=0;slot<4;++slot) {
        const auto value=g_feedback_mixer.Read(slot,now);
        if(value==g_feedback_sent[slot])continue;
        XINPUT_VIBRATION motors={value.low,value.high};
        if(g_modern_xinput_set && g_modern_xinput_set(slot,&motors)==ERROR_SUCCESS)
            g_feedback_sent[slot]=value;
        else {g_feedback_mixer.Clear(slot);g_feedback_sent[slot]={};}
    }
    ReleaseSRWLockExclusive(&g_feedback_lock);
}
static void CALLBACK ControllerFeedbackTimer(void*,BOOLEAN) {
    // No game memory, input session, renderer or simulation state is read here.
    WriteControllerFeedback(GetTickCount64(),ModernHasFocus());
}
static bool StartControllerFeedbackTimer() {
    return g_feedback_timer || CreateTimerQueueTimer(&g_feedback_timer,nullptr,
        ControllerFeedbackTimer,nullptr,0,10,WT_EXECUTEDEFAULT)!=0;
}
static bool (*g_feedback_start_timer)()=StartControllerFeedbackTimer;
static void StopControllerFeedback() {WriteControllerFeedback(GetTickCount64(),false);}
static void QueueControllerFeedback(unsigned slot,ff_feedback::Event event) {
    if(!g_modern_xinput_set || slot>=4 || !g_feedback_start_timer())return;
    AcquireSRWLockExclusive(&g_feedback_lock);
    g_feedback_mixer.Add(slot,event,GetTickCount64());
    ReleaseSRWLockExclusive(&g_feedback_lock);
}
static void ShutdownControllerFeedback() {
    if(g_feedback_timer) {
        DeleteTimerQueueTimer(nullptr,g_feedback_timer,INVALID_HANDLE_VALUE);
        g_feedback_timer=nullptr;
    }
    StopControllerFeedback();
}
