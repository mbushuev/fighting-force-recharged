#pragma once
#include "frame_interpolation.h"
#include "render_phase.h"
#include <limits.h>

static bool g_smooth_frames_enabled,g_smooth_frame_active,g_smooth_frame_history,g_smooth_frame_trace;
static ff_frame::Snapshot g_smooth_previous,g_smooth_current;
static ff_frame::Cadence g_smooth_cadence;
static LARGE_INTEGER g_smooth_frequency;
static void(__cdecl* g_smooth_native_background)();
static void(__cdecl* g_smooth_native_clocks[4])();
static int(__cdecl* g_smooth_native_elapsed)();
struct SmoothView {
    int32_t position[3]={};int16_t yaw=0,pitch=0;
    bool valid=false;
};
static SmoothView g_smooth_old_views[3],g_smooth_new_views[3];
static void SmoothCapture(ff_frame::Snapshot& frame) {
    memcpy(frame.actors,reinterpret_cast<void*>(g_base+0x86A500),sizeof(frame.actors));
    memcpy(frame.palettes,reinterpret_cast<void*>(g_base+0x868A40),sizeof(frame.palettes));
    memcpy(frame.objects,reinterpret_cast<void*>(g_base+0x870620),sizeof(frame.objects));
    memcpy(frame.effects,reinterpret_cast<void*>(g_base+0x86ACA0),sizeof(frame.effects));
    frame.stage=CameraField<int>(0x85A6B8);frame.players=CameraField<int>(0x8745A0);
    frame.tick=CameraField<uint32_t>(0x85B560);
    frame.eye[0]=g_camera_cooperative?g_coop_players[0].eye.selected:g_first_person.selected;
    frame.eye[1]=g_camera_cooperative && g_coop_players[1].eye.selected;
}
struct SmoothRenderTransaction {
    BYTE environment[128*36];DWORD fades[3],random_seed;
    static constexpr DWORD camera_rvas[7]={0x866220,0x866224,0x866228,0x87888C,0x866214,0x878708,0x86BF0C};
    DWORD camera[7];
    SmoothRenderTransaction() {
        memcpy(environment,reinterpret_cast<void*>(g_base+0x86BF20),sizeof(environment));
        fades[0]=CameraField<DWORD>(0x20D53C);fades[1]=CameraField<DWORD>(0x20D528);fades[2]=CameraField<DWORD>(0x9D200);
        random_seed=CameraField<DWORD>(0x6F5D8);
        for(unsigned i=0;i<7;++i)camera[i]=CameraField<DWORD>(camera_rvas[i]);
    }
    ~SmoothRenderTransaction() {
        // Retail object/effect renderers also read live records. An extra
        // sample cannot leak temporary spatial values or HUD writes into a tick.
        memcpy(reinterpret_cast<void*>(g_base+0x86A500),g_smooth_current.actors,sizeof(g_smooth_current.actors));
        memcpy(reinterpret_cast<void*>(g_base+0x868A40),g_smooth_current.palettes,sizeof(g_smooth_current.palettes));
        memcpy(reinterpret_cast<void*>(g_base+0x870620),g_smooth_current.objects,sizeof(g_smooth_current.objects));
        memcpy(reinterpret_cast<void*>(g_base+0x86ACA0),g_smooth_current.effects,sizeof(g_smooth_current.effects));
        memcpy(reinterpret_cast<void*>(g_base+0x86BF20),environment,sizeof(environment));
        CameraField<DWORD>(0x20D53C)=fades[0];CameraField<DWORD>(0x20D528)=fades[1];CameraField<DWORD>(0x9D200)=fades[2];
        CameraField<DWORD>(0x6F5D8)=random_seed;
        for(unsigned i=0;i<7;++i)CameraField<DWORD>(camera_rvas[i])=camera[i];
    }
};
static void SmoothInterpolateObjects() {
    if(!g_smooth_frame_history)return;
    for(unsigned i=0;i<64;++i)ff_frame::Object(reinterpret_cast<void*>(g_base+0x870620+i*36),
        g_smooth_previous.objects[i],g_smooth_current.objects[i]);
    for(unsigned i=0;i<128;++i)ff_frame::Object(reinterpret_cast<void*>(g_base+0x86ACA0+i*36),
        g_smooth_previous.effects[i],g_smooth_current.effects[i],false);
}
static void __cdecl SmoothGameplayBackground() {
    if(g_smooth_frame_active) {
        const int owner=g_camera_cooperative && g_camera_owner>=0?g_camera_owner:2;
        SmoothView current;
        memcpy(current.position,reinterpret_cast<void*>(g_base+0x8677B0),sizeof(current.position));
        current.yaw=CameraField<int16_t>(0x873CE0);current.pitch=CameraField<int16_t>(0x87459C);current.valid=true;
        g_smooth_new_views[owner]=current;
        if(g_render_intermediate && g_smooth_frame_history) {
            for(unsigned i=0;i<8;++i)ff_frame::Actor(reinterpret_cast<void*>(g_base+0x86FE80+i*0xF4),
                reinterpret_cast<void*>(g_base+0x8623E0+i*0x238),g_smooth_previous.actors[i],g_smooth_current.actors[i],
                g_smooth_previous.palettes[i],g_smooth_current.palettes[i]);
            const auto& before=g_smooth_old_views[owner];
            if(before.valid && ff_frame::Nearby(before.position,current.position,0)) {
                ff_frame::Position(reinterpret_cast<void*>(g_base+0x8677B0),before.position,current.position,0);
                CameraField<int16_t>(0x873CE0)=int16_t(ff_frame::Angle(before.yaw,current.yaw,65536));
                CameraField<int16_t>(0x87459C)=int16_t(ff_frame::Angle(before.pitch,current.pitch,65536));
                // The non-threaded native matrix initializer still reads the
                // source camera. Keep it coherent with the published render
                // pose during this transaction, then restore it before input.
                const auto position=reinterpret_cast<const int32_t*>(g_base+0x8677B0);
                for(unsigned axis=0;axis<3;++axis)CameraField<int32_t>(0x866220+axis*4)=position[axis];
                CameraField<int16_t>(0x87888C)=CameraField<int16_t>(0x873CE0);
                CameraField<int16_t>(0x87888E)=CameraField<int16_t>(0x87459C);
                CameraField<int32_t>(0x866214)=position[1]>>12;
                CameraField<int32_t>(0x878708)=int32_t(-int64_t(position[0])>>12);
                CameraField<int32_t>(0x86BF0C)=int32_t(-int64_t(position[2])>>12);
            }
        }
    }
    g_smooth_native_background();
}
static int64_t SmoothCounter() {LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
static void SmoothDelay(int64_t remaining) {
    // A high-resolution waitable timer avoids changing the system timer period.
    static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x2,TIMER_ALL_ACCESS);
    if(timer && remaining>g_smooth_frequency.QuadPart/1000) {
        LARGE_INTEGER due;due.QuadPart=-((remaining-g_smooth_frequency.QuadPart/2000)*10000000/g_smooth_frequency.QuadPart);
        if(SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,100);
    } else if(remaining>g_smooth_frequency.QuadPart/500)Sleep(1);
    else if(remaining>g_smooth_frequency.QuadPart/5000)SwitchToThread();
    else YieldProcessor();
}
static int64_t(*g_smooth_counter)()=SmoothCounter;
static void(*g_smooth_delay)(int64_t)=SmoothDelay;
static void SmoothWait() {
    const int64_t deadline=g_smooth_cadence.Next(g_smooth_counter(),g_smooth_frequency.QuadPart);
    for(int64_t now=g_smooth_counter();now<deadline;now=g_smooth_counter())g_smooth_delay(deadline-now);
    if(g_smooth_frame_trace) {
        static unsigned frames=0;static int64_t began=0,last=0;static uint32_t tick=0;
        const int64_t now=g_smooth_counter();
        if(last && now-last>g_smooth_frequency.QuadPart/4)frames=0;
        last=now;
        if(!frames) {began=now;tick=CameraField<uint32_t>(0x85B560);}
        if(++frames==121) {
            const double seconds=double(now-began)/g_smooth_frequency.QuadPart;
            char message[160];sprintf_s(message,"Gameplay cadence: %.2f presentations/s, %.2f native updates/s.\r\n",
                120/seconds,uint32_t(CameraField<uint32_t>(0x85B560)-tick)/seconds);Log(message);frames=0;
        }
    }
}
static void __cdecl SmoothClockStop() {if(!g_smooth_frame_active)g_smooth_native_clocks[0]();}
static int __cdecl SmoothClockElapsed() {
    if(!g_smooth_frame_active)return g_smooth_native_elapsed();
    if(g_coop_render_pass!=0)SmoothWait();
    return INT_MAX;
}
static void __cdecl SmoothClockReset() {if(!g_smooth_frame_active)g_smooth_native_clocks[2]();}
static void __cdecl SmoothClockStart() {if(!g_smooth_frame_active)g_smooth_native_clocks[3]();}
static bool SmoothMayRender() {
    const int stage=CameraField<int>(0x85A6B8),players=CameraField<int>(0x8745A0);
    return g_smooth_frames_enabled && stage>=0 && stage<25 && (players==1 || players==2) &&
        CameraField<int>(0x85A668)!=1 && !CameraField<int>(0x861F68) && ModernHasFocus();
}
static void __cdecl SmoothGameplayFrame() {
    auto native=reinterpret_cast<void(__cdecl*)()>(g_base+0x40BC0);
    static int64_t previous_end=0;
    const int64_t began=g_smooth_counter();
    if(!SmoothMayRender()) {
        g_smooth_frame_history=false;g_smooth_cadence.Reset();previous_end=0;native();return;
    }
    SmoothCapture(g_smooth_current);
    g_smooth_frame_history=g_smooth_frame_history && g_smooth_previous.Consecutive(g_smooth_current) &&
        previous_end && began-previous_end<g_smooth_frequency.QuadPart/4;
    memset(g_smooth_new_views,0,sizeof(g_smooth_new_views));
    g_smooth_frame_active=true;
    {
        SmoothRenderTransaction restore;
        g_render_intermediate=true;SmoothInterpolateObjects();native();g_render_intermediate=false;
    }
    native();g_smooth_frame_active=false;
    // Return to the native clock at the same boundary as one original frame.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x23030)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x23040)();
    g_smooth_previous=g_smooth_current;g_smooth_frame_history=true;
    memcpy(g_smooth_old_views,g_smooth_new_views,sizeof(g_smooth_old_views));
    previous_end=g_smooth_counter();
}
static void InstallSmoothFrameHooks() {
    if(!g_presentation_installed || GetPrivateProfileIntW(L"Rendering",L"GameplayFPS",60,g_config_path)!=60)return;
    const DWORD sites[]={0x41157,0x40C42,0x25978,0x25985,0x259CB,0x259D0};
    const DWORD targets[]={0x40BC0,0x47A10,0x23060,0x23090,0x23030,0x23040};
    void* hooks[]={reinterpret_cast<void*>(SmoothGameplayFrame),reinterpret_cast<void*>(SmoothGameplayBackground),
        reinterpret_cast<void*>(SmoothClockStop),reinterpret_cast<void*>(SmoothClockElapsed),
        reinterpret_cast<void*>(SmoothClockReset),reinterpret_cast<void*>(SmoothClockStart)};
    void* previous[6];
    void* cooperative[]={nullptr,reinterpret_cast<void*>(GameWideBackground),reinterpret_cast<void*>(CooperativeClockStop),
        reinterpret_cast<void*>(CooperativeClockElapsed),reinterpret_cast<void*>(CooperativeClockReset),reinterpret_cast<void*>(CooperativeClockStart)};
    for(unsigned i=0;i<6;++i) {
        auto code=reinterpret_cast<const BYTE*>(g_base+sites[i]);LONG relative;memcpy(&relative,code+1,4);
        previous[i]=reinterpret_cast<void*>(g_base+sites[i]+5+relative);
        if(code[0]!=0xE8 || (previous[i]!=reinterpret_cast<void*>(g_base+targets[i]) && previous[i]!=cooperative[i])) {
            Log("Smooth gameplay signatures differ; original cadence retained.\r\n");return;
        }
    }
    auto begin=reinterpret_cast<BYTE*>(g_base+0x25978);const SIZE_T length=0x4115C-0x25978;DWORD old;
    if(!VirtualProtect(begin,length,PAGE_EXECUTE_READWRITE,&old))return;
    for(unsigned i=0;i<6;++i) {
        const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(hooks[i])-(g_base+sites[i]+5));
        memcpy(reinterpret_cast<void*>(g_base+sites[i]+1),&relative,4);
    }
    DWORD ignored;VirtualProtect(begin,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),begin,length);
    g_smooth_native_background=reinterpret_cast<void(__cdecl*)()>(previous[1]);
    g_smooth_native_clocks[0]=reinterpret_cast<void(__cdecl*)()>(previous[2]);
    g_smooth_native_elapsed=reinterpret_cast<int(__cdecl*)()>(previous[3]);
    g_smooth_native_clocks[2]=reinterpret_cast<void(__cdecl*)()>(previous[4]);
    g_smooth_native_clocks[3]=reinterpret_cast<void(__cdecl*)()>(previous[5]);
    QueryPerformanceFrequency(&g_smooth_frequency);g_smooth_frames_enabled=true;
    g_smooth_frame_trace=GetPrivateProfileIntW(L"Diagnostics",L"TraceFrameRate",0,g_config_path)!=0;
    Log("60 Hz gameplay rendering enabled; native simulation advances once per two displayed samples.\r\n");
}
