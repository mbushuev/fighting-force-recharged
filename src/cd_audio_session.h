#pragma once
#include <windows.h>
#include <string.h>
#include <stdio.h>

class CdAudioSession {
    bool opened_ = false;
    char root_[4] = {};
public:
    // Only the retail's exact synchronous open/close strings are intercepted.
    // Seek/play/status/time-format commands retain their original semantics.
    template <typename Send>
    DWORD SendCommand(LPCSTR command, LPSTR output, UINT length, HWND callback,
        const char* media_root, Send send, bool* reused) {
        *reused = false;
        if (!command) return send(command, output, length, callback);
        if (!_stricmp(command, "open cdaudio")) {
            if (opened_ && (!media_root || strcmp(media_root, root_))) {
                send("close cdaudio", nullptr, 0, nullptr);
                opened_ = false;
            }
            if (opened_) {
                if (output && length) output[0] = 0;
                *reused = true;
                return 0;
            }
            char explicit_drive[96];
            if (media_root) sprintf_s(explicit_drive, "open %.2s type cdaudio alias cdaudio shareable", media_root);
            DWORD result = send(media_root ? explicit_drive : command, output, length, callback);
            if (!result && media_root) { opened_ = true; strcpy_s(root_, media_root); }
            else if (result && media_root) result = send(command, output, length, callback);
            return result;
        }
        if (!_stricmp(command, "close cdaudio") && opened_) {
            // Retail closes the control session immediately after successful
            // play/status commands. Keep playback alive with the reused alias;
            // native transitions and pause send their own explicit stop.
            if (output && length) output[0] = 0;
            *reused = true;
            return 0;
        }
        return send(command, output, length, callback);
    }
};
