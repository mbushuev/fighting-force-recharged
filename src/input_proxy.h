#pragma once
#include <new>
#include "keyboard_read.h"

class KeyboardProxy final : public IDirectInputDeviceA {
    IDirectInputDeviceA* real_;
    LONG references_ = 1;
    HRESULT last_error_ = DI_OK;
public:
    explicit KeyboardProxy(IDirectInputDeviceA* real) : real_(real) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** output) override {
        if (!output) return E_POINTER;
        if (iid == IID_IUnknown || iid == IID_IDirectInputDeviceA) {
            *output = static_cast<IDirectInputDeviceA*>(this); AddRef(); return S_OK;
        }
        return real_->QueryInterface(iid, output);
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&references_);
        if (!count) { real_->Release(); delete this; }
        return count;
    }
    HRESULT STDMETHODCALLTYPE GetCapabilities(LPDIDEVCAPS caps) override { return real_->GetCapabilities(caps); }
    HRESULT STDMETHODCALLTYPE EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA callback, void* context, DWORD flags) override { return real_->EnumObjects(callback, context, flags); }
    HRESULT STDMETHODCALLTYPE GetProperty(REFGUID property, LPDIPROPHEADER header) override { return real_->GetProperty(property, header); }
    HRESULT STDMETHODCALLTYPE SetProperty(REFGUID property, LPCDIPROPHEADER header) override { return real_->SetProperty(property, header); }
    HRESULT STDMETHODCALLTYPE Acquire() override { return real_->Acquire(); }
    HRESULT STDMETHODCALLTYPE Unacquire() override { return real_->Unacquire(); }
    HRESULT STDMETHODCALLTYPE GetDeviceState(DWORD size, void* data) override {
        DWORD foreground_process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground_process);
        bool recovered;
        HRESULT result = ReadKeyboardState(size, data,
            foreground_process == GetCurrentProcessId(),
            [this](DWORD length, void* output) { return real_->GetDeviceState(length, output); },
            [this]() { return real_->Acquire(); }, &recovered);
        if (recovered) Log("Keyboard reacquired and read in the same poll.\r\n");
        if (FAILED(result) && result != last_error_) {
            char message[96];
            sprintf_s(message, "Keyboard read failed: 0x%08lx; cleared stale state\r\n", result);
            Log(message);
        }
        last_error_ = result;
        return result;
    }
    HRESULT STDMETHODCALLTYPE GetDeviceData(DWORD size, LPDIDEVICEOBJECTDATA data, DWORD* count, DWORD flags) override { return real_->GetDeviceData(size, data, count, flags); }
    HRESULT STDMETHODCALLTYPE SetDataFormat(LPCDIDATAFORMAT format) override { return real_->SetDataFormat(format); }
    HRESULT STDMETHODCALLTYPE SetEventNotification(HANDLE event) override { return real_->SetEventNotification(event); }
    HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND window, DWORD flags) override { return real_->SetCooperativeLevel(window, flags); }
    HRESULT STDMETHODCALLTYPE GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA info, DWORD object, DWORD how) override { return real_->GetObjectInfo(info, object, how); }
    HRESULT STDMETHODCALLTYPE GetDeviceInfo(LPDIDEVICEINSTANCEA info) override { return real_->GetDeviceInfo(info); }
    HRESULT STDMETHODCALLTYPE RunControlPanel(HWND window, DWORD flags) override { return real_->RunControlPanel(window, flags); }
    HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE instance, DWORD version, REFGUID guid) override { return real_->Initialize(instance, version, guid); }
};

class InputProxy final : public IDirectInputA {
    IDirectInputA* real_;
    LONG references_ = 1;
public:
    explicit InputProxy(IDirectInputA* real) : real_(real) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** output) override {
        if (!output) return E_POINTER;
        if (iid == IID_IUnknown || iid == IID_IDirectInputA) {
            *output = static_cast<IDirectInputA*>(this); AddRef(); return S_OK;
        }
        return real_->QueryInterface(iid, output);
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&references_);
        if (!count) { real_->Release(); delete this; }
        return count;
    }
    HRESULT STDMETHODCALLTYPE CreateDevice(REFGUID guid, LPDIRECTINPUTDEVICEA* output, IUnknown* outer) override {
        HRESULT result = real_->CreateDevice(guid, output, outer);
        if (SUCCEEDED(result) && output && *output && !outer && guid == GUID_SysKeyboard) {
            auto proxy = new(std::nothrow) KeyboardProxy(*output);
            if (proxy) { *output = proxy; Log("Keyboard state recovery enabled.\r\n"); }
        }
        return result;
    }
    HRESULT STDMETHODCALLTYPE EnumDevices(DWORD type, LPDIENUMDEVICESCALLBACKA callback, void* context, DWORD flags) override { return real_->EnumDevices(type, callback, context, flags); }
    HRESULT STDMETHODCALLTYPE GetDeviceStatus(REFGUID guid) override { return real_->GetDeviceStatus(guid); }
    HRESULT STDMETHODCALLTYPE RunControlPanel(HWND window, DWORD flags) override { return real_->RunControlPanel(window, flags); }
    HRESULT STDMETHODCALLTYPE Initialize(HINSTANCE instance, DWORD version) override { return real_->Initialize(instance, version); }
};
