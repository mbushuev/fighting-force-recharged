#pragma once
#include <math.h>
#include <stdint.h>

namespace ff_camera {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTurn = 2.0 * kPi;
inline double Clamp(double x, double lo, double hi) { return x < lo ? lo : x > hi ? hi : x; }
inline double Wrap(double angle) { return angle - floor((angle + kPi) / kTurn) * kTurn; }
inline double Radians(int16_t angle) { return angle * (kTurn / 65536.0); }
inline int16_t NativeAngle(double angle) {
    const int units = static_cast<int>(round(Wrap(angle) * (65536.0 / kTurn)));
    return static_cast<int16_t>(static_cast<uint16_t>(units));
}
inline int32_t Fixed(double value) {
    return static_cast<int32_t>(round(Clamp(value * 65536.0, -2147483648.0, 2147483647.0)));
}
inline double StickAxis(int value, int deadzone) {
    const double magnitude = fabs(static_cast<double>(value));
    if (magnitude <= deadzone) return 0;
    const double scaled = Clamp((magnitude - deadzone) / (32767.0 - deadzone), 0, 1);
    return value < 0 ? -scaled : scaled;
}
struct Point { double x = 0, y = 0, z = 0; };
struct Pose { int32_t x, y, z; int16_t yaw, pitch; };
struct Settings {
    double distance = 720, height = 96, ground_clearance = 64;
    double initial_pitch = 20 * kPi / 180;
    double min_pitch = -10 * kPi / 180, max_pitch = 65 * kPi / 180;
    double mouse_x = 0.15 * kPi / 180, mouse_y = 0.12 * kPi / 180;
    double stick_speed = 120 * kPi / 180;
    double trigger_zoom_speed = 720;
    bool invert_y = false;
};
inline double MinimumPitch(double distance, const Settings& settings) {
    // This is a local ground guard relative to the followed actor's feet,
    // not a collision query against remote level geometry.
    return fmax(settings.min_pitch, asin(Clamp((settings.ground_clearance - settings.height) / distance, -1, 1)));
}
struct Orbit {
    double yaw = 0, pitch = 0, distance = 720;
    void Begin(int16_t native_yaw, const Settings& settings) {
        yaw = Radians(native_yaw); distance = Clamp(settings.distance, 240, 1200);
        pitch = Clamp(settings.initial_pitch, MinimumPitch(distance, settings), settings.max_pitch);
    }
    void Look(double mouse_x, double mouse_y, double stick_x, double stick_y,
              double seconds, double wheel_notches, const Settings& settings) {
        // Mouse counts are displacement; a held stick is angular velocity.
        // Bound elapsed time so focus loss cannot produce a catch-up rotation.
        const double dt = Clamp(seconds, 0, 0.1);
        yaw = Wrap(yaw - mouse_x * settings.mouse_x - stick_x * settings.stick_speed * dt);
        const double vertical = mouse_y * settings.mouse_y - stick_y * settings.stick_speed * dt;
        distance = Clamp(distance - wheel_notches * 60, 240, 1200);
        pitch = Clamp(pitch + (settings.invert_y ? -vertical : vertical), MinimumPitch(distance, settings), settings.max_pitch);
    }
    Pose Follow(const Point& player, const Settings& settings) const {
        // Retail coordinates use positive Y downward and yaw zero looks +Z.
        // Quantize the orbit and view to the same native trigonometric sample.
        const int16_t native_yaw = NativeAngle(yaw);
        // Round the lower limit upward to a native sample. Truncating a
        // negative angle toward the floor could put the camera below its guard.
        const double sample = kTurn / 2048;
        const double minimum = ceil(MinimumPitch(distance, settings) / sample) * sample;
        const int16_t native_pitch = NativeAngle(fmax(pitch, minimum));
        const double y = Radians(static_cast<int16_t>(native_yaw & ~31));
        const double p = Radians(static_cast<int16_t>(native_pitch & ~31));
        const double radius = distance * cos(p);
        return {Fixed(player.x + sin(y) * radius), Fixed(player.y - settings.height - sin(p) * distance),
                Fixed(player.z - cos(y) * radius), native_yaw, native_pitch};
    }
};
// Recorded input is an attract demo. The stage-4 opening owns its camera for
// the first 660 native updates. Frontend/recorded scene paths never call this.
inline bool CanOrbit(int stage, int players, int input_mode, int health, unsigned tick) {
    return stage >= 0 && stage < 25 && players == 1 && input_mode != 1 && health > 0 &&
           !(stage == 4 && tick < 660);
}
} // namespace ff_camera
