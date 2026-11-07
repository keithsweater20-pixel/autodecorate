#pragma once
#include <functional>
#include <string>
#include <vector>
#include "Layout.hpp"
#include "Settings.hpp"
#include "Styles.hpp"

namespace autodeco {

// A decoration object the pool says is usable for a rule.
struct Candidate {
    int id = 0;
    float size = 30.f;  // largest dimension of the object at scale 1
};

using PoolProvider = std::function<std::vector<Candidate>(DecoRule const&)>;

struct Placement {
    int id = 0;
    V2 pos;
    float rotation = 0.f;  // degrees clockwise
    float scale = 1.f;
    float size = 30.f;     // approximate on-screen size, for the preview
    bool flipX = false;
    bool secondary = false;
    int layer = 1;         // 0 back, 1 middle, 2 front (all behind the layout)
    bool background = false;
};

struct Plan {
    std::vector<Placement> items;
    std::string note;  // set when nothing could be planned
    size_t backgroundCount = 0;
};

constexpr size_t kMaxDecorations = 8000;
constexpr size_t kMaxBackground = 2500;

Plan makePlan(ScanResult const& scan, Settings const& st, unsigned seed, PoolProvider const& pool);

}  // namespace autodeco
