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
        for (int pt = 0; pt < kPatternCount; ++pt) {
            Settings s2;
            s2.style = static_cast<Style>(st);
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
            std::printf("style=%d pattern=%d placed=%zu inside=%d nearSpike=%d %s\n", st, pt, p.items.size(), inside, nearSpike,
                        p.note.c_str());
            if (inside || nearSpike) ++fails;
        }
    if (csv) std::fclose(csv);
    std::printf(fails ? "FAILED\n" : "OK\n");
    return fails ? 1 : 0;
}
