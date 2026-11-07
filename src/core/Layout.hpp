#pragma once
#include <unordered_map>
#include <vector>
#include "Styles.hpp"
#include "Vec.hpp"

namespace autodeco {

enum class LayoutKind { Block = 0, Spike = 1, Slope = 2, LongSlope = 3 };

constexpr unsigned maskOf(LayoutKind k) { return 1u << static_cast<unsigned>(k); }
constexpr unsigned kMaskAll = 15u;
constexpr unsigned kMaskSpike = 2u;

// What the Geode glue reads off a real GameObject. Everything after this is pure geometry.
struct ObjDesc {
    LayoutKind kind = LayoutKind::Block;
    V2 pos;
    float w = 30.f, h = 30.f;  // unrotated size in level units
    float rotation = 0.f;      // degrees, clockwise (Geometry Dash convention)
    bool flipX = false, flipY = false;
    bool selected = true;      // faces are only generated for selected objects
};

struct Poly {
    ObjDesc desc;
    std::vector<V2> pts;
    V2 mn, mx;
};

// A piece of exposed outline that decoration can attach to.
struct Face {
    V2 pos, normal, tangent;
    float length = 30.f;
    unsigned kind = FaceTop;
    LayoutKind host = LayoutKind::Block;
};

class LayoutIndex {
public:
    void build(std::vector<Poly> const& polys);
    // Smallest distance from p to any polygon whose kind is in kindMask.
    // 0 means p is inside one; maxDist is returned when nothing is that close.
    float distance(V2 p, float maxDist, unsigned kindMask) const;

private:
    static constexpr float kCell = 60.f;
    std::vector<Poly> const* m_polys = nullptr;
    std::unordered_map<long long, std::vector<int>> m_cells;
    static long long key(int ix, int iy) {
        return (static_cast<long long>(ix) << 32) | static_cast<unsigned int>(iy);
    }
};

struct ScanResult {
    std::vector<Poly> polys;
    std::vector<Face> faces;
    LayoutIndex index;
    int counts[4] = {0, 0, 0, 0};
    V2 mn, mx;
};

// Builds polygons, the spatial index and the exposed faces from object descriptions.
// Moving a ScanResult would dangle the index, so it is built in place.
void buildScan(ScanResult& out, std::vector<ObjDesc> const& objs);

V2 rotateCW(V2 v, float degrees);

}  // namespace autodeco
