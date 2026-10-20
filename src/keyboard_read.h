#pragma once
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0500
#endif
#include <windows.h>
#include <dinput.h>

// The retail keyboard reader ignores HRESULT and therefore reuses stale bytes
// after focus/device loss. Recover and read in the same poll; never synthesize
// key presses or consume keyboard input while another process has focus.
template <typename Read, typename Acquire>
HRESULT ReadKeyboardState(DWORD size, void* data, bool foreground,
    Read read, Acquire acquire, bool* recovered) {
    *recovered = false;
    if (!data) return E_POINTER;
    ZeroMemory(data, size);
    if (!foreground) return DI_OK;
    HRESULT result = read(size, data);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        HRESULT acquired = acquire();
        if (SUCCEEDED(acquired)) {
            result = read(size, data);
            *recovered = SUCCEEDED(result);
        }
    }
    if (FAILED(result)) ZeroMemory(data, size);
    return result;
}
