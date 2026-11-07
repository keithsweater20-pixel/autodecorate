#include "Layout.hpp"
#include <algorithm>
#include <cmath>

namespace autodeco {

static constexpr float kPi = 3.14159265358979f;
static constexpr float kUnit = 30.f;

V2 rotateCW(V2 v, float deg) {
    float a = -deg * kPi / 180.f;
    float c = std::cos(a), s = std::sin(a);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

static V2 dirToWorld(ObjDesc const& d, V2 v) {
    if (d.flipX) v.x = -v.x;
    if (d.flipY) v.y = -v.y;
    return rotateCW(v, d.rotation);
}
static V2 toWorld(ObjDesc const& d, V2 local) { return d.pos + dirToWorld(d, local); }

static std::vector<V2> localShape(ObjDesc const& d) {
    float hw = d.w * .5f, hh = d.h * .5f;
    switch (d.kind) {
        case LayoutKind::Block:
            return {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
        case LayoutKind::Slope:
        case LayoutKind::LongSlope:
            // right angle at bottom-right, hypotenuse rising to the right
            return {{-hw, -hh}, {hw, -hh}, {hw, hh}};
        case LayoutKind::Spike:
            return {{-hw, -hh}, {hw, -hh}, {0.f, hh}};
    }
    return {};
}

static float distSeg(V2 p, V2 a, V2 b) {
    V2 ab = b - a;
    float l2 = ab.dot(ab);
    float t = l2 > 1e-6f ? std::clamp((p - a).dot(ab) / l2, 0.f, 1.f) : 0.f;
    return (p - (a + ab * t)).len();
}

static bool insidePoly(Poly const& poly, V2 p) {
    bool pos = false, neg = false;
    size_t n = poly.pts.size();
    for (size_t i = 0; i < n; ++i) {
        V2 a = poly.pts[i], b = poly.pts[(i + 1) % n];
        float c = (b - a).cross(p - a);
        if (c > 0) pos = true;
        if (c < 0) neg = true;
        if (pos && neg) return false;
    }
    return true;
}

static float polyDistance(Poly const& poly, V2 p) {
    if (insidePoly(poly, p)) return 0.f;
    float best = 1e9f;
    size_t n = poly.pts.size();
    for (size_t i = 0; i < n; ++i) best = std::min(best, distSeg(p, poly.pts[i], poly.pts[(i + 1) % n]));
    return best;
}

void LayoutIndex::build(std::vector<Poly> const& polys) {
    m_polys = &polys;
    m_cells.clear();
    for (size_t i = 0; i < polys.size(); ++i) {
        auto const& p = polys[i];
        int x0 = static_cast<int>(std::floor(p.mn.x / kCell)), x1 = static_cast<int>(std::floor(p.mx.x / kCell));
        int y0 = static_cast<int>(std::floor(p.mn.y / kCell)), y1 = static_cast<int>(std::floor(p.mx.y / kCell));
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y) m_cells[key(x, y)].push_back(static_cast<int>(i));
    }
}

float LayoutIndex::distance(V2 p, float maxDist, unsigned kindMask) const {
    if (!m_polys) return maxDist;
    int x0 = static_cast<int>(std::floor((p.x - maxDist) / kCell)), x1 = static_cast<int>(std::floor((p.x + maxDist) / kCell));
    int y0 = static_cast<int>(std::floor((p.y - maxDist) / kCell)), y1 = static_cast<int>(std::floor((p.y + maxDist) / kCell));
    float best = maxDist;
    for (int x = x0; x <= x1; ++x) {
        for (int y = y0; y <= y1; ++y) {
            auto it = m_cells.find(key(x, y));
            if (it == m_cells.end()) continue;
            for (int idx : it->second) {
                auto const& poly = (*m_polys)[idx];
                if (!(maskOf(poly.desc.kind) & kindMask)) continue;
                best = std::min(best, polyDistance(poly, p));
                if (best <= 0.f) return 0.f;
            }
        }
    }
    return best;
}

static bool covered(ScanResult const& r, V2 probe) { return r.index.distance(probe, 0.5f, kMaskAll) <= 0.001f; }

static unsigned kindForNormal(V2 n) {
    if (n.y > 0.7f) return FaceTop;
    if (n.y < -0.7f) return FaceBottom;
    return FaceSide;
}

static void addEdge(ScanResult& r, LayoutKind host, V2 a, V2 b, V2 n, bool slopeFace) {
    V2 d = b - a;
    float L = d.len();
    if (L < 1.f) return;
    int segs = std::max(1, static_cast<int>(std::lround(L / kUnit)));
    V2 dir = d / L;
    float segLen = L / segs;
    for (int k = 0; k < segs; ++k) {
        V2 pos = a + dir * ((k + .5f) * segLen);
        if (covered(r, pos + n * 6.f)) continue;  // another layout object sits against this face
        Face f;
        f.pos = pos;
        f.normal = n;
        f.tangent = dir;
        f.length = segLen;
        f.kind = slopeFace ? FaceSlope : kindForNormal(n);
        f.host = host;
        r.faces.push_back(f);
    }
}

static void addCorner(ScanResult& r, LayoutKind host, V2 c, V2 na, V2 nb) {
    V2 diag = (na + nb).norm();
    if (covered(r, c + diag * 6.f) || covered(r, c + na * 6.f) || covered(r, c + nb * 6.f)) return;
    Face f;
    f.pos = c;
    f.normal = diag;
    f.tangent = V2(-diag.y, diag.x);
    f.length = 10.f;
    f.kind = FaceCorner;
    f.host = host;
    r.faces.push_back(f);
}

void buildScan(ScanResult& r, std::vector<ObjDesc> const& objs) {
    r.polys.clear();
    r.faces.clear();
    for (int& c : r.counts) c = 0;
    r.polys.reserve(objs.size());
    bool first = true;
    for (auto const& d : objs) {
        Poly p;
        p.desc = d;
        for (V2 lp : localShape(d)) p.pts.push_back(toWorld(d, lp));
        p.mn = p.mx = p.pts[0];
        for (V2 q : p.pts) {
            p.mn = V2(std::min(p.mn.x, q.x), std::min(p.mn.y, q.y));
            p.mx = V2(std::max(p.mx.x, q.x), std::max(p.mx.y, q.y));
        }
        if (first) {
            r.mn = p.mn;
            r.mx = p.mx;
            first = false;
        } else {
            r.mn = V2(std::min(r.mn.x, p.mn.x), std::min(r.mn.y, p.mn.y));
            r.mx = V2(std::max(r.mx.x, p.mx.x), std::max(r.mx.y, p.mx.y));
        }
        r.counts[static_cast<int>(d.kind)]++;
        r.polys.push_back(std::move(p));
    }
    r.index.build(r.polys);

    for (auto const& poly : r.polys) {
        ObjDesc const& d = poly.desc;
        if (!d.selected) continue;
        float hw = d.w * .5f, hh = d.h * .5f;
        switch (d.kind) {
            case LayoutKind::Block: {
                V2 lc[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
                V2 ln[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};  // outward normal of edge i (lc[i] -> lc[i+1])
                V2 wc[4], wn[4];
                for (int i = 0; i < 4; ++i) {
                    wc[i] = toWorld(d, lc[i]);
                    wn[i] = dirToWorld(d, ln[i]);
                }
                for (int i = 0; i < 4; ++i) addEdge(r, d.kind, wc[i], wc[(i + 1) % 4], wn[i], false);
                for (int i = 0; i < 4; ++i) addCorner(r, d.kind, wc[i], wn[(i + 3) % 4], wn[i]);
                break;
            }
            case LayoutKind::Slope:
            case LayoutKind::LongSlope: {
                V2 A = toWorld(d, {-hw, -hh}), C = toWorld(d, {hw, -hh}), B = toWorld(d, {hw, hh});
                V2 nBottom = dirToWorld(d, {0, -1});
                V2 nWall = dirToWorld(d, {1, 0});
                V2 nHyp = dirToWorld(d, V2(-d.h, d.w).norm());
                addEdge(r, d.kind, A, C, nBottom, false);
                addEdge(r, d.kind, C, B, nWall, false);
                addEdge(r, d.kind, B, A, nHyp, true);
                break;
            }
            case LayoutKind::Spike: {
                // Spikes are keep-clear zones. Only their flanks get small decoration.
                for (int side = -1; side <= 1; side += 2) {
                    V2 c = toWorld(d, {side * hw, -hh});
                    V2 n = dirToWorld(d, {static_cast<float>(side), 0.f});
                    if (covered(r, c + n * 8.f)) continue;
                    Face f;
                    f.pos = c;
                    f.normal = n;
                    f.tangent = V2(-n.y, n.x);
                    f.length = 10.f;
                    f.kind = FaceCorner;
                    f.host = LayoutKind::Spike;
                    r.faces.push_back(f);
                }
                break;
            }
        }
    }
}

}  // namespace autodeco
