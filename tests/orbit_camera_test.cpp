#include "../src/orbit_camera.h"
#include <stdio.h>
using namespace ff_camera;
static int failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
static bool Near(double a, double b, double epsilon = 0.0001) { return fabs(a - b) < epsilon; }
int main() {
    Settings settings; settings.initial_pitch = 0; settings.height = 0;
    Orbit orbit; orbit.Begin(0, settings);
    Point focus = {100, -20, 200};
    auto pose = orbit.Follow(focus, settings);
    CHECK(pose.x == Fixed(100) && pose.y == Fixed(-20) && pose.z == Fixed(-520));
    orbit.yaw = kPi / 2; pose = orbit.Follow(focus, settings);
    CHECK(pose.x == Fixed(820) && pose.z == Fixed(200));
    // A camera on the sphere always looks at the same focus, including turns
    // across signed angle wrap. Native projection uses 2048 angular samples.
    for (int i = -180; i <= 180; i += 5) {
        orbit.yaw = i * kPi / 180; orbit.pitch = 30 * kPi / 180;
        pose = orbit.Follow(focus, settings);
        const double dx = pose.x / 65536.0 - focus.x, dy = pose.y / 65536.0 - focus.y, dz = pose.z / 65536.0 - focus.z;
        CHECK(Near(sqrt(dx * dx + dy * dy + dz * dz), 720, 0.0001));
        CHECK(Near(Wrap(Radians(pose.yaw) - orbit.yaw), 0, 0.0001));
    }
    Orbit a, b; a.Begin(32760, settings); b = a;
    a.Look(120, 0, 0, 0, 0.05, 0, settings);
    for (int i = 0; i < 4; ++i) b.Look(30, 0, 0, 0, 0.0125, 0, settings);
    CHECK(Near(a.yaw, b.yaw));
    a.Begin(-32760, settings); b = a;
    a.Look(0, 0, 1, 0, 0.1, 0, settings);
    for (int i = 0; i < 4; ++i) b.Look(0, 0, 1, 0, 0.025, 0, settings);
    CHECK(Near(a.yaw, b.yaw));
    a.Look(0, 100000, 0, 0, 0, 10000, settings);
    CHECK(a.pitch == settings.max_pitch && a.distance == 240);
    a.Look(0, -100000, 0, 0, 0, -10000, settings);
    CHECK(a.pitch == settings.min_pitch && a.distance == 1200);
    CHECK(StickAxis(1000, 8000) == 0 && StickAxis(-32768, 8000) == -1);
    CHECK(StickAxis(32767, 8000) == 1);
    CHECK(Fixed(1e20) == INT32_MAX && Fixed(-1e20) == INT32_MIN);
    CHECK(CanOrbit(0, 1, 0, 100, 1));
    CHECK(!CanOrbit(0, 1, 1, 100, 1) && !CanOrbit(0, 2, 0, 100, 1));
    CHECK(!CanOrbit(26, 1, 0, 100, 1) && !CanOrbit(0, 1, 0, 0, 1));
    CHECK(!CanOrbit(4, 1, 0, 100, 659) && CanOrbit(4, 1, 0, 100, 660));
    printf("Orbit camera: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
