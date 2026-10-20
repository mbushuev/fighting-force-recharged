#pragma once
#include <atomic>

static bool g_camera_trace;
enum CameraTimingKind {CameraEnvironmentTime,CameraObjectsTime,CameraCaptureTime,CameraFacesTime,CameraQueryTime,
    CameraRenderPrepareTime,CameraRenderSubmitTime,CameraRenderEndTime,CameraTimingCount};
struct CameraTimingCounters {
    std::atomic<unsigned long long> total{0},maximum{0},calls{0};
};
static CameraTimingCounters g_camera_timings[CameraTimingCount];
struct CameraTiming {
    CameraTimingKind kind;
    LARGE_INTEGER start={};
    explicit CameraTiming(CameraTimingKind value):kind(value) {if(g_camera_trace)QueryPerformanceCounter(&start);}
    ~CameraTiming() {
        if(!start.QuadPart)return;
        LARGE_INTEGER end;QueryPerformanceCounter(&end);
        static const LONGLONG frequency=[]() {LARGE_INTEGER value;QueryPerformanceFrequency(&value);return value.QuadPart;}();
        const auto micros=static_cast<unsigned long long>((end.QuadPart-start.QuadPart)*1000000/frequency);
        auto& counters=g_camera_timings[kind];counters.total+=micros;++counters.calls;
        auto maximum=counters.maximum.load();while(maximum<micros && !counters.maximum.compare_exchange_weak(maximum,micros)){}
    }
};
static void ReportCameraTimings() {
    const char* names[]={"environment","objects","capture","faces","query","render-prepare","render-submit","render-end"};
    for(int i=0;i<CameraTimingCount;++i) {
        auto& counters=g_camera_timings[i];const auto calls=counters.calls.exchange(0);
        const auto total=counters.total.exchange(0),maximum=counters.maximum.exchange(0);
        if(!calls)continue;
        char message[160];sprintf_s(message,"Camera timing %s: %llu calls, mean %.3f ms, max %.3f ms.\r\n",
            names[i],calls,total/(calls*1000.0),maximum/1000.0);Log(message);
    }
}
