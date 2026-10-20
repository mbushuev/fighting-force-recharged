#include "../src/cd_audio_session.h"
#include <string>
#include <vector>
static int failures;
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
int main() {
    CdAudioSession session;
    std::vector<std::string> commands;
    bool reused;
    const char first_disc[]={'G',':','\\',0},second_disc[]={'J',':','\\',0};
    char output[32] = "stale";
    auto send = [&](LPCSTR command, LPSTR, UINT, HWND) -> DWORD { commands.emplace_back(command); return 0; };
    CHECK(session.SendCommand("open cdaudio", output, sizeof(output), nullptr, first_disc, send, &reused) == 0 && !reused);
    CHECK(commands.back() == "open G: type cdaudio alias cdaudio shareable");
    CHECK(session.SendCommand("close cdaudio", output, sizeof(output), nullptr, nullptr, send, &reused) == 0 && reused);
    CHECK(commands.back() == "stop cdaudio wait");
    const size_t count = commands.size();
    CHECK(session.SendCommand("open cdaudio", output, sizeof(output), nullptr, first_disc, send, &reused) == 0 && reused);
    CHECK(commands.size() == count && output[0] == 0);
    session.SendCommand("play cdaudio from 2", nullptr, 0, nullptr, nullptr, send, &reused);
    CHECK(!reused && commands.back() == "play cdaudio from 2");
    session.SendCommand("status cdaudio number of tracks", output, sizeof(output), nullptr, nullptr, send, &reused);
    CHECK(!reused && commands.back() == "status cdaudio number of tracks");
    session.SendCommand("open cdaudio", nullptr, 0, nullptr, second_disc, send, &reused);
    CHECK(!reused && commands[commands.size()-2] == "close cdaudio");
    CHECK(commands.back() == "open J: type cdaudio alias cdaudio shareable");
    CdAudioSession failed;
    auto fail = [&](LPCSTR command, LPSTR, UINT, HWND) -> DWORD { commands.emplace_back(command); return 123; };
    CHECK(failed.SendCommand("open cdaudio", output, sizeof(output), nullptr, first_disc, fail, &reused) == 123 && !reused);
    CHECK(failed.SendCommand("close cdaudio", output, sizeof(output), nullptr, nullptr, fail, &reused) == 123 && !reused);
    CHECK(commands.back() == "close cdaudio");
    printf("CD audio session checks: %s\n", failures ? "FAILED" : "PASS");
    return failures ? 1 : 0;
}
