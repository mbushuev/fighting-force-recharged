#pragma once
#include <process.h>
#include <algorithm>
#include <limits.h>
#include <stdlib.h>

using CdSend = DWORD(WINAPI*)(LPCSTR,LPSTR,UINT,HWND);
static CdSend RealCdSend() {
    static auto send=reinterpret_cast<CdSend>(GetProcAddress(LoadLibraryW(L"winmm.dll"),"mciSendStringA"));
    return send;
}
static CdSend (*g_cd_send)()=RealCdSend;
static bool g_background_cd;
static constexpr int kCdLengthPending=INT_MAX;
enum class CdAction {Stop,Track,Resume};
struct CdRequest {
    CdAction action=CdAction::Stop;
    unsigned track=0,elapsed=0,serial=0;
};
struct CdTrack {
    unsigned number=0;
    int length=-1,position=-1;
};
static SRWLOCK g_cd_lock=SRWLOCK_INIT;
static HANDLE g_cd_wake,g_cd_thread;
static CdRequest g_cd_request;
static CdTrack g_cd_track;
static bool g_cd_pending,g_cd_exiting,g_cd_milliseconds;
static unsigned g_cd_waiting_length;

static DWORD SendCdAudio(CdAudioSession& session,const char* command,char* output,UINT length,HWND callback,
                        const char* root,CdSend send,bool reuse) {
    const ULONGLONG start=GetTickCount64();
    bool reused=false;
    const DWORD result=reuse?session.SendCommand(command,output,length,callback,root,send,&reused):
        send(command,output,length,callback);
    const ULONGLONG duration=GetTickCount64()-start;
    const bool transport=command && (!_strnicmp(command,"play cdaudio",12) || !_strnicmp(command,"stop cdaudio",12));
    if(result || duration>=100 || transport) {
        char message[320];
        sprintf_s(message,"CD audio: %.180s -> %lu (%llu ms)%s\r\n",command?command:"<null>",result,duration,
            reused?"; reused mounted-disc session":"");
        Log(message);
    }
    return result;
}
static int CdTrackValue(CdAudioSession& session,CdSend send,const char* property,unsigned track) {
    char command[64],answer[32]={};
    sprintf_s(command,"status cdaudio %s track %u",property,track);
    if(SendCdAudio(session,command,answer,sizeof(answer),nullptr,nullptr,send,true))return -1;
    char* end;
    const long value=strtol(answer,&end,10);
    return end!=answer && !*end && value>=0?value:-1;
}
static bool LatestCdRequest(const CdRequest& request) {
    AcquireSRWLockShared(&g_cd_lock);
    const bool latest=!g_cd_exiting && request.serial==g_cd_request.serial;
    ReleaseSRWLockShared(&g_cd_lock);
    return latest;
}
static unsigned __stdcall BackgroundCdAudio(void*) {
    const auto send=g_cd_send();
    CdAudioSession session;
    char media[4]={};
    CdTrack track;
    bool playing=false;
    while(WaitForSingleObject(g_cd_wake,INFINITE)==WAIT_OBJECT_0) {
        AcquireSRWLockExclusive(&g_cd_lock);
        const bool exiting=g_cd_exiting,pending=g_cd_pending;
        const auto request=g_cd_request;
        g_cd_pending=false;
        ReleaseSRWLockExclusive(&g_cd_lock);
        if(exiting)break;
        if(!pending || !send)continue;
        const bool stop=request.action==CdAction::Stop;
        if(stop && !playing)continue;
        if(!stop) {
            char root[4]={};
            g_audio_media_root(root);
            if(strcmp(media,root)) {strcpy_s(media,root);track={};}
            if(g_rip_source && !*media) {
                send("close cdaudio",nullptr,0,nullptr);session={};playing=false;
                AcquireSRWLockExclusive(&g_cd_lock);
                g_cd_track={};
                ReleaseSRWLockExclusive(&g_cd_lock);
                continue;
            }
        }
        if(!LatestCdRequest(request))continue;
        const auto command=[&](const char* text) {
            return SendCdAudio(session,text,nullptr,0,nullptr,*media?media:nullptr,send,true);
        };
        if(command("open cdaudio"))continue;
        DWORD playback_error=0;
        if(stop) {
            if(LatestCdRequest(request)) {
                playback_error=command("stop cdaudio wait");
                playing=false;
            }
        } else {
            if(track.number!=request.track || track.length<=0 || track.position<0) {
                track={};track.number=request.track;
                if(!command("set cdaudio time format milliseconds")) {
                    track.length=CdTrackValue(session,send,"length",request.track);
                    track.position=CdTrackValue(session,send,"position",request.track);
                }
            }
            AcquireSRWLockExclusive(&g_cd_lock);
            g_cd_track=track;
            ReleaseSRWLockExclusive(&g_cd_lock);
            if(LatestCdRequest(request)) {
                const bool resume=request.action==CdAction::Resume && track.position>=0;
                playback_error=command(resume?"set cdaudio time format milliseconds":"set cdaudio time format tmsf");
                if(!playback_error) {
                    char play[64];
                    unsigned from=request.track;
                    if(resume) {
                        const unsigned offset=track.length>0?
                            (std::min)(request.elapsed,unsigned(track.length-1)):request.elapsed;
                        from=unsigned(track.position)+offset;
                    }
                    sprintf_s(play,"play cdaudio from %u",from);
                    if(LatestCdRequest(request)) {
                        playback_error=command(play);
                        playing=!playback_error;
                    }
                }
            }
        }
        if(playback_error) {
            send("close cdaudio",nullptr,0,nullptr);session={};track={};playing=false;
        } else command("close cdaudio");
    }
    if(send)send("close cdaudio",nullptr,0,nullptr);
    return 0;
}
static bool StartBackgroundCd() {
    if(g_cd_thread)return true;
    g_cd_wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(g_cd_wake)g_cd_thread=reinterpret_cast<HANDLE>(_beginthreadex(nullptr,0,BackgroundCdAudio,nullptr,0,nullptr));
    if(g_cd_thread)return true;
    if(g_cd_wake)CloseHandle(g_cd_wake);
    g_cd_wake=nullptr;g_background_cd=false;
    return false;
}
static void ShutdownBackgroundCd() {
    if(!g_cd_thread)return;
    AcquireSRWLockExclusive(&g_cd_lock);
    g_cd_exiting=true;
    ReleaseSRWLockExclusive(&g_cd_lock);
    SetEvent(g_cd_wake);
    // The driver can be inside a slow command. Do not wait for it indefinitely.
    if(WaitForSingleObject(g_cd_thread,2000)==WAIT_OBJECT_0) {
        CloseHandle(g_cd_thread);CloseHandle(g_cd_wake);
        g_cd_thread=g_cd_wake=nullptr;
    }
}
static void RequestBackgroundCd(CdAction action,unsigned track=0,unsigned elapsed=0) {
    AcquireSRWLockExclusive(&g_cd_lock);
    // Keep the current intent when a menu, pause or level replaces a request.
    g_cd_request={action,track,elapsed,g_cd_request.serial+1};
    g_cd_pending=true;
    ReleaseSRWLockExclusive(&g_cd_lock);
    SetEvent(g_cd_wake);
}
static CdTrack ReadBackgroundCdTrack(unsigned number) {
    AcquireSRWLockShared(&g_cd_lock);
    const CdTrack track=g_cd_track.number==number?g_cd_track:CdTrack{};
    ReleaseSRWLockShared(&g_cd_lock);
    return track;
}
static bool ParseCdNumber(const char* command,const char* prefix,unsigned& value) {
    const size_t length=strlen(prefix);
    if(_strnicmp(command,prefix,length))return false;
    const char* digits=command+length;
    if(*digits<'0' || *digits>'9')return false;
    char* end;
    value=strtoul(digits,&end,10);
    return !*end;
}
static bool GameBackgroundCdCommand(const char* command,char* output,UINT length,HWND callback) {
    if(!g_background_cd || !command || callback)return false;
    if(!_stricmp(command,"open cdaudio") && !StartBackgroundCd())return false;
    if(!g_cd_thread)return false;
    unsigned value;
    if(!_stricmp(command,"set cdaudio time format tmsf"))g_cd_milliseconds=false;
    else if(!_stricmp(command,"set cdaudio time format milliseconds"))g_cd_milliseconds=true;
    else if(ParseCdNumber(command,"play cdaudio from ",value)) {
        if(g_cd_milliseconds) {
            // Retail resumes from the track's disc offset plus its saved elapsed time.
            RequestBackgroundCd(CdAction::Resume,InputField<unsigned>(0x552398),InputField<unsigned>(0x5523A0));
        } else RequestBackgroundCd(CdAction::Track,value);
    } else if(!_stricmp(command,"stop cdaudio wait"))RequestBackgroundCd(CdAction::Stop);
    else if(ParseCdNumber(command,"status cdaudio length track ",value)) {
        const int length_ms=ReadBackgroundCdTrack(value).length;
        g_cd_waiting_length=length_ms>0?0:value;
        if(output && length)_snprintf_s(output,length,_TRUNCATE,"%d",length_ms>0?length_ms:kCdLengthPending);
        return true;
    } else if(ParseCdNumber(command,"status cdaudio position track ",value)) {
        const int position=ReadBackgroundCdTrack(value).position;
        if(output && length)_snprintf_s(output,length,_TRUNCATE,"%d",position>=0?position:0);
        return true;
    } else if(_stricmp(command,"open cdaudio") && _stricmp(command,"close cdaudio"))return false;
    if(output && length)output[0]=0;
    return true;
}
static void RefreshBackgroundCdLength() {
    if(g_cd_waiting_length && InputField<unsigned>(0x552398)==g_cd_waiting_length &&
       InputField<int>(0x8523F0)==kCdLengthPending) {
        const int length=ReadBackgroundCdTrack(g_cd_waiting_length).length;
        if(length>0) {InputField<int>(0x8523F0)=length;g_cd_waiting_length=0;}
    }
}
static BOOL WINAPI GameCdPeekMessage(LPMSG message,HWND window,UINT first,UINT last,UINT flags) {
    RefreshBackgroundCdLength();
    return GamePeekMessage(message,window,first,last,flags);
}
