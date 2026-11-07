#pragma once
// Tiny 2D vector used by the engine-independent core (no Geode/cocos types here,
// so the core can be unit-tested off-device).
#include <cmath>

namespace autodeco {

struct V2 {
    float x = 0.f, y = 0.f;
    V2() = default;
    V2(float x_, float y_) : x(x_), y(y_) {}
    V2 operator+(V2 o) const { return {x + o.x, y + o.y}; }
    V2 operator-(V2 o) const { return {x - o.x, y - o.y}; }
    V2 operator*(float s) const { return {x * s, y * s}; }
    V2 operator/(float s) const { return {x / s, y / s}; }
    float dot(V2 o) const { return x * o.x + y * o.y; }
    float cross(V2 o) const { return x * o.y - y * o.x; }
    float len() const { return std::sqrt(x * x + y * y); }
    V2 norm() const {
        float l = len();
        return l > 1e-6f ? V2(x / l, y / l) : V2(0.f, 1.f);
    }
};

struct RGB {
    unsigned char r = 255, g = 255, b = 255;
};

}  // namespace autodeco
