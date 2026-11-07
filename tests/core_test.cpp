// Off-device sanity test for the engine-independent core. Not part of the mod build.
#include <cstdio>
#include <cstdlib>
#include "../src/core/Plan.hpp"

using namespace autodeco;

int main(int argc, char** argv) {
    // A small layout: floor, a step, two spikes, a slope up, a long slope down, a ceiling.
    std::vector<ObjDesc> objs;
    auto blk = [&](float x, float y) { ObjDesc d; d.kind = LayoutKind::Block; d.pos = {x, y}; objs.push_back(d); };
    for (int i = 0; i < 40; ++i) blk(15 + i * 30, 15);
    for (int i = 10; i < 14; ++i) blk(15 + i * 30, 45);
    for (int i = 26; i < 32; ++i) blk(15 + i * 30, 165);
    for (int i = 26; i < 32; ++i) blk(15 + i * 30, 195);
    auto spike = [&](float x, float y) { ObjDesc d; d.kind = LayoutKind::Spike; d.pos = {x, y}; objs.push_back(d); };
    spike(15 + 5 * 30, 45);
    spike(15 + 6 * 30, 45);
    spike(15 + 20 * 30, 45);
    ObjDesc s; s.kind = LayoutKind::Slope; s.pos = {15 + 16 * 30, 45}; objs.push_back(s);
    ObjDesc ls; ls.kind = LayoutKind::LongSlope; ls.w = 60; ls.h = 30; ls.pos = {15 + 22 * 30 + 15, 45}; ls.flipX = true; objs.push_back(ls);

    ScanResult scan;
    buildScan(scan, objs);
    std::printf("polys=%zu faces=%zu blocks=%d spikes=%d slopes=%d long=%d\n", scan.polys.size(), scan.faces.size(),
                scan.counts[0], scan.counts[1], scan.counts[2], scan.counts[3]);

    PoolProvider pool = [](DecoRule const& r) {
        std::vector<Candidate> v;
        for (int i = 0; i < 6; ++i) v.push_back({100 + i, std::min(r.maxSize, std::max(r.minSize, 20.f + 12.f * i))});
        return v;
    };

    int fails = 0;
    FILE* csv = std::fopen(argc > 1 ? argv[1] : "/dev/null", "w");
    for (int st = 0; st < kStyleCount; ++st)
      for (int var = 0; var < (st == 1 ? 2 : 1); ++var)
        for (int pt = 0; pt < kPatternCount; ++pt) {
            Settings s2;
            s2.style = static_cast<Style>(st);
            s2.variant = var;
            s2.background = 2;
            s2.seed = 1234;
            s2.pattern = static_cast<Pattern>(pt);
            s2.density = 70;
            Plan p = makePlan(scan, s2, 1234, pool);
            Plan p2 = makePlan(scan, s2, 1234, pool);
            if (p.items.size() != p2.items.size()) { std::printf("NONDETERMINISTIC\n"); ++fails; }
            int inside = 0, nearSpike = 0;
            for (auto const& it : p.items) {
                if (scan.index.distance(it.pos, 1.f, kMaskAll) <= 0.001f) ++inside;
                if (scan.index.distance(it.pos, 9.f, kMaskSpike) < 8.f) ++nearSpike;
                if (csv && st == 0 && pt == 0) std::fprintf(csv, "%f,%f,%f,%f\n", it.pos.x, it.pos.y, it.rotation, it.scale);
            }
            size_t bg = p.backgroundCount; int layerCount[3] = {0, 0, 0};
            for (auto const& it : p.items) if (it.layer >= 0 && it.layer < 3) ++layerCount[it.layer];
            if (var == 0 && pt == 0) {
                // Settings must actually change the result: variation, detail and layers.
                Settings a = s2; a.variation = 0; Settings b = s2; b.variation = 100;
                auto pa = makePlan(scan, a, 1234, pool), pb = makePlan(scan, b, 1234, pool);
                bool differ = false;
                for (size_t i = 0; i < std::min(pa.items.size(), pb.items.size()); ++i)
                    if (std::abs(pa.items[i].scale - pb.items[i].scale) > 1e-3f) { differ = true; break; }
                if (!differ) { std::printf("VARIATION HAS NO EFFECT\n"); ++fails; }
                Settings one = s2; one.layers = 1; auto p1 = makePlan(scan, one, 1234, pool);
                for (auto const& it : p1.items) if (it.layer != 1 && !it.background) { std::printf("LAYERS=1 NOT HONOURED\n"); ++fails; break; }
                Settings nobg = s2; nobg.background = 0; auto pn = makePlan(scan, nobg, 1234, pool);
                if (pn.backgroundCount != 0) { std::printf("BACKGROUND OFF NOT HONOURED\n"); ++fails; }
                Settings ex = s2; for (auto const& it : p.items) ex = s2; ScanResult scan2; buildScan(scan2, objs);
                for (auto const& it : p.items) scan2.existing.push_back({it.pos, 30.f});
                auto pe = makePlan(scan2, s2, 1234, pool);
                int overlap = 0;
                for (auto const& it : pe.items) for (auto const& e : scan2.existing) if ((it.pos - e.first).len() < 2.f) { ++overlap; break; }
                if (overlap) { std::printf("DUPLICATES ON EXISTING: %d\n", overlap); ++fails; }
            }
            std::printf("style=%d variant=%d pattern=%d placed=%zu (bg %zu, layers %d/%d/%d) inside=%d nearSpike=%d %s\n", st, var, pt, p.items.size(), bg,
                        layerCount[0], layerCount[1], layerCount[2], inside, nearSpike, p.note.c_str());
            if (inside || nearSpike) ++fails;
        }
    if (csv) std::fclose(csv);
    std::printf(fails ? "FAILED\n" : "OK\n");
    return fails ? 1 : 0;
}
