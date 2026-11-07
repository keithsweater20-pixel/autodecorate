#include "Plan.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_map>

namespace autodeco {

static constexpr float kPi = 3.14159265358979f;

namespace {
struct RuleRT {
    DecoRule const* rule;
    std::vector<Candidate> pool;
};

struct Occupancy {
    static constexpr float kCell = 10.f;
    std::unordered_map<long long, int> cells;
    static long long key(int x, int y) { return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(y); }

    template <class F>
    void forCells(V2 c, float r, F&& fn) {
        int x0 = static_cast<int>(std::floor((c.x - r) / kCell)), x1 = static_cast<int>(std::floor((c.x + r) / kCell));
        int y0 = static_cast<int>(std::floor((c.y - r) / kCell)), y1 = static_cast<int>(std::floor((c.y + r) / kCell));
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y) {
                V2 cc((x + .5f) * kCell, (y + .5f) * kCell);
                if ((cc - c).len() <= r + kCell * .5f) fn(key(x, y));
            }
    }
    float fraction(V2 c, float r) {
        int total = 0, taken = 0;
        forCells(c, r, [&](long long k) {
            ++total;
            if (cells.count(k)) ++taken;
        });
        return total ? static_cast<float>(taken) / total : 0.f;
    }
    void mark(V2 c, float r) {
        forCells(c, r, [&](long long k) { cells[k] = 1; });
    }
};
}  // namespace

Plan makePlan(ScanResult const& sc, Settings const& st, unsigned seed, PoolProvider const& poolFn) {
    Plan plan;
    StyleDef const& sd = styleDef(st.style, st.variant);
    std::mt19937 rng(seed);
    auto u = [&](float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); };

    std::vector<RuleRT> rules;
    for (auto const& r : sd.rules) {
        auto pool = poolFn(r);
        if (!pool.empty()) rules.push_back({&r, std::move(pool)});
    }
    if (rules.empty()) {
        plan.note = "No usable decoration objects were found for this style.";
        return plan;
    }
    if (sc.faces.empty()) {
        plan.note = "No exposed layout surfaces to decorate.";
        return plan;
    }

    float d = std::clamp(st.density / 100.f, 0.f, 1.f) * sd.densityMul;
    float detailK = std::clamp(st.detail / 100.f, 0.f, 1.f);
    float varK = 0.4f + 1.2f * std::clamp(st.variation / 100.f, 0.f, 1.f);  // 1.0 at the default 50
    Occupancy occ;
    for (auto const& e : sc.existing) occ.mark(e.first, std::max(4.f, e.second * .35f));

    // Small accent rules scale with the Detail setting (1.0 at the default 50).
    auto effWeight = [&](DecoRule const& r) { return r.maxSize <= 60.f ? r.weight * (0.4f + 1.2f * detailK) : r.weight; };

    auto pickRule = [&](unsigned kind, bool anyFace) -> int {
        float total = 0.f;
        for (auto const& r : rules)
            if (anyFace || (r.rule->faces & kind)) total += effWeight(*r.rule);
        if (total <= 0.f) return -1;
        float roll = u(0.f, total);
        for (size_t i = 0; i < rules.size(); ++i) {
            if (!(anyFace || (rules[i].rule->faces & kind))) continue;
            roll -= effWeight(*rules[i].rule);
            if (roll <= 0.f) return static_cast<int>(i);
        }
        for (size_t i = rules.size(); i-- > 0;)
            if (anyFace || (rules[i].rule->faces & kind)) return static_cast<int>(i);
        return -1;
    };

    auto tryPlace = [&](RuleRT& rr, Face const& f, float extraReach, float along, bool jitterRot) -> bool {
        if (plan.items.size() >= kMaxDecorations) return false;
        DecoRule const& rule = *rr.rule;
        Candidate const& cand = rr.pool[std::uniform_int_distribution<size_t>(0, rr.pool.size() - 1)(rng)];
        float mid = (rule.scaleMin + rule.scaleMax) * .5f, half = (rule.scaleMax - rule.scaleMin) * .5f * varK;
        float scale = std::max(.15f, mid + u(-1.f, 1.f) * half);
        float size = cand.size * scale;
        if (size < 6.f) {
            scale *= 6.f / std::max(size, 0.5f);
            size = 6.f;
        }
        // Keep the play space clear: nothing taller than ~1 block rises off a floor.
        if (st.keepPlayAreaClear && (f.kind & FaceTop) && extraReach < 1.f) {
            const float cap = 36.f;
            if (size > cap) {
                scale *= cap / size;
                size = cap;
            }
        }
        float reach = rule.reach + extraReach;
        V2 centre = f.pos + f.normal * (reach + size * (rule.alignToNormal ? .5f : .6f)) + f.tangent * along;
        V2 tip = f.pos + f.normal * (reach + size) + f.tangent * along;

        // Never inside layout geometry, so nothing can ever hide a hitbox the wrong way round.
        if (sc.index.distance(centre, 1.f, kMaskAll) <= 0.001f) return false;
        if (sc.index.distance(tip, 1.f, kMaskAll) <= 0.001f) return false;
        // Keep spikes readable.
        float clear = std::max(8.f, size * .4f);
        if (sc.index.distance(centre, clear + 1.f, kMaskSpike) < clear) return false;
        // Do not pile decorations on top of each other.
        float r = size * .35f;
        if (occ.fraction(centre, r) > .4f) return false;
        occ.mark(centre, r);

        Placement p;
        p.id = cand.id;
        p.pos = centre;
        p.scale = scale;
        p.size = size;
        p.layer = st.layers <= 1 ? 1 : std::clamp(rule.layer, 0, 2);
        p.flipX = u(0.f, 1.f) < .5f;
        p.secondary = rule.useSecondary != (u(0.f, 1.f) < .15f);
        if (rule.alignToNormal) {
            p.rotation = std::atan2(f.normal.x, f.normal.y) * 180.f / kPi + u(-8.f, 8.f) * varK;
        } else {
            p.rotation = jitterRot ? (varK < .7f ? u(-30.f, 30.f) : u(0.f, 360.f)) : u(-20.f, 20.f) * varK;
        }
        plan.items.push_back(p);
        return true;
    };

    std::vector<Face> faces = sc.faces;
    if (st.pattern == Pattern::Rhythm) {
        std::sort(faces.begin(), faces.end(), [](Face const& a, Face const& b) {
            return a.pos.x != b.pos.x ? a.pos.x < b.pos.x : a.pos.y < b.pos.y;
        });
    } else {
        std::shuffle(faces.begin(), faces.end(), rng);
    }

    switch (st.pattern) {
        case Pattern::Edge: {
            for (auto const& f : faces) {
                if (plan.items.size() >= kMaxDecorations) break;
                if (u(0.f, 1.f) > d * .85f) continue;
                int ri = pickRule(f.kind, false);
                if (ri < 0) continue;
                tryPlace(rules[ri], f, 0.f, u(-f.length * .35f, f.length * .35f), false);
            }
            break;
        }
        case Pattern::Scatter: {
            for (auto const& f : faces) {
                if (plan.items.size() >= kMaxDecorations) break;
                if (u(0.f, 1.f) > d * .55f) continue;
                int ri = pickRule(f.kind, true);
                if (ri < 0) continue;
                tryPlace(rules[ri], f, u(4.f, sd.scatterBand), u(-30.f, 30.f), true);
            }
            break;
        }
        case Pattern::Rhythm: {
            int k = std::max(1, static_cast<int>(std::lround(6.f - 5.f * d)));
            int run = 0;
            for (size_t i = 0; i < faces.size(); ++i) {
                if (plan.items.size() >= kMaxDecorations) break;
                if (static_cast<int>(i) % k) continue;
                std::vector<int> ok;
                for (size_t r = 0; r < rules.size(); ++r)
                    if (rules[r].rule->faces & faces[i].kind) ok.push_back(static_cast<int>(r));
                if (ok.empty()) continue;
                tryPlace(rules[ok[run++ % ok.size()]], faces[i], 0.f, 0.f, false);
            }
            break;
        }
        case Pattern::Cluster: {
            for (auto const& f : faces) {
                if (plan.items.size() >= kMaxDecorations) break;
                if (u(0.f, 1.f) > d * .2f) continue;
                int n = std::clamp(static_cast<int>(u(3.f, 7.f) * (0.5f + detailK)), 2, 12);
                for (int j = 0; j < n; ++j) {
                    int ri = pickRule(f.kind, false);
                    if (ri < 0) break;
                    tryPlace(rules[ri], f, u(0.f, 40.f), u(-45.f, 45.f), true);
                }
            }
            break;
        }
    }


    // ---- Background: large, faint shapes in empty space nearest the layout, on the back layer ----
    if (st.background > 0 && !sd.backgroundRules.empty() && !sc.polys.empty()) {
        std::vector<RuleRT> bgRules;
        for (auto const& r : sd.backgroundRules) {
            auto pool = poolFn(r);
            if (!pool.empty()) bgRules.push_back({&r, std::move(pool)});
        }
        if (!bgRules.empty()) {
            float spacing = st.background >= 2 ? 105.f : 170.f;
            float x0 = sc.mn.x - 60.f, x1 = sc.mx.x + 60.f, y0 = sc.mn.y - 60.f, y1 = sc.mx.y + 240.f;
            float cells = std::max(1.f, (x1 - x0) / spacing) * std::max(1.f, (y1 - y0) / spacing);
            if (cells > static_cast<float>(kMaxBackground)) spacing *= std::sqrt(cells / static_cast<float>(kMaxBackground));
            float wsum = 0.f;
            for (auto const& r : bgRules) wsum += r.rule->weight;
            const float reachMax = 260.f;
            for (float y = y0; y <= y1; y += spacing) {
                for (float x = x0; x <= x1; x += spacing) {
                    if (plan.backgroundCount >= kMaxBackground) break;
                    if (u(0.f, 1.f) > (st.background >= 2 ? .8f : .55f)) continue;
                    V2 c(x + u(-spacing * .4f, spacing * .4f), y + u(-spacing * .4f, spacing * .4f));
                    float roll = u(0.f, wsum);
                    RuleRT* rr = &bgRules.back();
                    for (auto& r : bgRules) {
                        roll -= r.rule->weight;
                        if (roll <= 0.f) { rr = &r; break; }
                    }
                    DecoRule const& rule = *rr->rule;
                    Candidate const& cand = rr->pool[std::uniform_int_distribution<size_t>(0, rr->pool.size() - 1)(rng)];
                    float scale = std::max(.2f, u(rule.scaleMin, rule.scaleMax));
                    float size = cand.size * scale;
                    if (size < 40.f) { scale *= 40.f / std::max(size, 1.f); size = 40.f; }
                    // Stay clear of the layout, but only decorate the area around it, not empty sky.
                    float nearest = sc.index.distance(c, reachMax, kMaskAll);
                    if (nearest <= size * .5f + 12.f || nearest >= reachMax) continue;
                    if (occ.fraction(c, size * .35f) > .25f) continue;
                    occ.mark(c, size * .35f);
                    Placement p;
                    p.id = cand.id;
                    p.pos = c;
                    p.scale = scale;
                    p.size = size;
                    p.layer = 0;
                    p.background = true;
                    p.flipX = u(0.f, 1.f) < .5f;
                    p.secondary = rule.useSecondary != (u(0.f, 1.f) < .3f);
                    p.rotation = u(-25.f, 25.f) * varK;
                    plan.items.push_back(p);
                    ++plan.backgroundCount;
                }
            }
        }
    }

    if (plan.items.empty()) plan.note = "Nothing fit at this density. Try a higher density or another pattern.";
    return plan;
}

}  // namespace autodeco
