#include "../src/keyboard_read.h"
#include <stdio.h>

static int failures;
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)

int main() {
    BYTE state[256];
    bool recovered;
    int reads = 0, acquisitions = 0;
    auto acquire = [&]() -> HRESULT { ++acquisitions; return DI_OK; };
    const HRESULT recoverable[] = {DIERR_INPUTLOST, DIERR_NOTACQUIRED};
    for (HRESULT lost : recoverable) {
        reads = acquisitions = 0;
        HRESULT result = ReadKeyboardState(sizeof(state), state, true,
            [&](DWORD, void* data) -> HRESULT {
                ++reads;
                if (reads == 1) return lost;
                static_cast<BYTE*>(data)[DIK_RETURN] = 0x80;
                return DI_OK;
            }, acquire, &recovered);
        CHECK(result == DI_OK && recovered);
        CHECK(reads == 2 && acquisitions == 1 && state[DIK_RETURN] == 0x80);
    }
    reads = acquisitions = 0;
    HRESULT result = ReadKeyboardState(sizeof(state), state, false,
        [&](DWORD, void*) -> HRESULT { ++reads; return DI_OK; }, acquire, &recovered);
    CHECK(result == DI_OK && !recovered && reads == 0 && acquisitions == 0);
    CHECK(state[DIK_RETURN] == 0);
    reads = acquisitions = 0;
    result = ReadKeyboardState(sizeof(state), state, true,
        [&](DWORD size, void* data) -> HRESULT { ++reads; FillMemory(data, size, 0x80); return DIERR_INPUTLOST; },
        [&]() -> HRESULT { ++acquisitions; return DIERR_OTHERAPPHASPRIO; }, &recovered);
    CHECK(result == DIERR_INPUTLOST && !recovered && reads == 1 && acquisitions == 1);
    for (BYTE key : state) CHECK(key == 0);
    reads = acquisitions = 0;
    result = ReadKeyboardState(sizeof(state), state, true,
        [&](DWORD size, void* data) -> HRESULT { ++reads; FillMemory(data, size, 0x80); return E_FAIL; }, acquire, &recovered);
    CHECK(result == E_FAIL && !recovered && reads == 1 && acquisitions == 0);
    for (BYTE key : state) CHECK(key == 0);
    reads = acquisitions = 0;
    result = ReadKeyboardState(sizeof(state), state, true,
        [&](DWORD, void* data) -> HRESULT { ++reads; static_cast<BYTE*>(data)[DIK_Q] = 0x80; return DI_OK; }, acquire, &recovered);
    CHECK(result == DI_OK && !recovered && reads == 1 && acquisitions == 0 && state[DIK_Q] == 0x80);
    printf("Keyboard recovery checks: %s\n", failures ? "FAILED" : "PASS");
    return failures ? 1 : 0;
}
