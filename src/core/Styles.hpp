#pragma once
#include <string>
#include <vector>
#include "Settings.hpp"

namespace autodeco {

// Which kind of surface a decoration may sit on.
enum FaceKind : unsigned {
    FaceTop = 1,
    FaceBottom = 2,
    FaceSide = 4,
    FaceSlope = 8,
    FaceCorner = 16,
    FaceAll = 31,
};

// One family of decoration inside a style. Decorations are chosen by what their
// texture is called and how big they are, never by hardcoded object IDs.
struct DecoRule {
    std::vector<std::string> keywords;  // lowercase substrings of the object's texture name
    unsigned faces = FaceAll;           // surfaces it may sit on
    float weight = 1.f;                 // relative pick chance
    float minSize = 6.f, maxSize = 150.f;  // allowed object size window (level units)
    float scaleMin = 0.6f, scaleMax = 1.f;
    bool alignToNormal = true;          // grow away from the surface (hang / rise)
    bool useSecondary = false;          // use the secondary colour
    float reach = 0.f;                  // gap kept between surface and decoration
    int layer = 1;                      // 0 = back, 1 = middle, 2 = front (all stay behind the layout)
};

struct StyleDef {
    const char* name;
    const char* blurb;
    RGB primary, secondary;
    RGB background;                     // suggested background colour
    std::vector<DecoRule> rules;
    std::vector<DecoRule> backgroundRules;  // large, faint shapes placed away from the layout
    float scatterBand;  // how far from a surface Scatter/Cluster may drift
    float densityMul;   // style-level density multiplier
};

StyleDef const& styleDef(Style s, int variant = 0);

}  // namespace autodeco
