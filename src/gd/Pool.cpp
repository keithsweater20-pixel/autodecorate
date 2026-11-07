#include "Pool.hpp"
#include <algorithm>
#include "Compat.hpp"

using namespace geode::prelude;

namespace autodeco {

namespace {
PoolStats g_stats;

struct Info {
    int id;
    std::string lower;
    int state = 0;  // 0 unchecked, 1 usable, 2 rejected
    float size = 0.f;
};

std::vector<Info>& allInfo() {
    static std::vector<Info> all;
    static bool built = false;
    if (built) return all;
    built = true;
    auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    for (int id = 1; id <= 4000; ++id) {
        std::string frame = compat::frameForKey(id);
        if (frame.empty()) continue;
        g_stats.keys++;
        std::string lower = frame;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (lower.find("empty") != std::string::npos) continue;
        if (!cache->spriteFrameByName(frame.c_str())) { g_stats.noFrame++; continue; }
        all.push_back({id, lower, 0, 0.f});
    }
    return all;
}

bool verify(Info& info) {
    if (info.state) return info.state == 1;
    info.state = 2;
    // Create through the editor, exactly like the real placement does, so the object type is
    // fully set up; then remove it again right away.
    auto lel = LevelEditorLayer::get();
    GameObject* obj = lel ? lel->createObject(info.id, ccp(-9000.f, -9000.f), true) : GameObject::createWithKey(info.id);
    if (!obj) { g_stats.noObject++; return false; }
    bool plain = compat::isPlainDecoration(obj);
    auto cs = obj->getContentSize();
    float size = std::max(cs.width, cs.height) * std::max(std::abs(obj->getScaleX()), std::abs(obj->getScaleY()));
    if (lel) lel->removeObject(obj, true);
    if (!plain) { g_stats.notDeco++; return false; }
    if (size < 1.f) { g_stats.tooSmall++; return false; }
    info.size = size;
    info.state = 1;
    g_stats.verified++;
    return true;
}
}  // namespace

PoolStats const& poolStats() { return g_stats; }

std::vector<Candidate> queryDecoPool(DecoRule const& rule) {
    std::vector<Candidate> out;
    auto& all = allInfo();
    for (auto& info : all) {
        bool match = false;
        for (auto const& kw : rule.keywords)
            if (info.lower.find(kw) != std::string::npos) { match = true; break; }
        if (!match || !verify(info)) continue;
        if (info.size < rule.minSize || info.size > rule.maxSize) continue;
        out.push_back({info.id, info.size});
        if (out.size() >= 120) break;
    }
    if (out.size() >= 3) return out;

    // Not enough name matches: fall back to any plain decoration of a fitting size.
    for (auto& info : all) {
        if (out.size() >= 24) break;
        if (!verify(info) || info.size < rule.minSize || info.size > rule.maxSize) continue;
        if (std::any_of(out.begin(), out.end(), [&](Candidate const& c) { return c.id == info.id; })) continue;
        out.push_back({info.id, info.size});
    }
    return out;
}

}  // namespace autodeco
