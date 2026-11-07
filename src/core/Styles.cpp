#include "Styles.hpp"

namespace autodeco {

RGB swatchColor(int i) {
    static const RGB table[kSwatchCount] = {
        {230, 60, 60},   // red
        {240, 140, 40},  // orange
        {250, 220, 70},  // yellow
        {120, 220, 70},  // lime
        {40, 200, 170},  // teal
        {60, 200, 240},  // cyan
        {60, 110, 240},  // blue
        {150, 80, 230},  // purple
        {240, 100, 190}, // pink
    };
    if (i < 0 || i >= kSwatchCount) return {255, 255, 255};
    return table[i];
}

static DecoRule rule(std::vector<std::string> kw, unsigned faces, float weight, float minSize, float maxSize,
                     float scaleMin, float scaleMax, bool align, bool secondary, float reach) {
    DecoRule r;
    r.keywords = std::move(kw);
    r.faces = faces;
    r.weight = weight;
    r.minSize = minSize;
    r.maxSize = maxSize;
    r.scaleMin = scaleMin;
    r.scaleMax = scaleMax;
    r.alignToNormal = align;
    r.useSecondary = secondary;
    r.reach = reach;
    return r;
}

// The four looks are interpretations of the style names (palette + the kind of
// shapes that suit the mood). They are data, so tuning one is a table edit.
static StyleDef makeGrief() {
    return StyleDef{
        "Grief",
        "Heavy and mournful. Thorns, hanging chains and drifting ash in bruised violet and crimson.",
        {96, 64, 128},
        {200, 48, 84},
        {
            rule({"spike", "thorn", "hook", "claw"}, FaceTop | FaceSide | FaceSlope, 3.f, 12, 70, .6f, 1.1f, true, false, 0),
            rule({"chain", "drip", "tear", "rope"}, FaceBottom, 2.5f, 20, 120, .6f, 1.2f, true, false, 0),
            rule({"cloud", "smoke", "mist", "fog"}, FaceTop | FaceSlope, 1.f, 30, 140, .7f, 1.3f, false, true, 6),
            rule({"ball", "circle", "dot", "orb"}, FaceAll, 1.5f, 6, 40, .3f, .7f, false, true, 10),
        },
        70.f,
        .8f};
}

static StyleDef makeTidalWave() {
    return StyleDef{
        "Tidal Wave",
        "Flowing and bright. Curling waves, rising bubbles and kelp in cyan and deep blue.",
        {0, 176, 230},
        {24, 92, 210},
        {
            rule({"wave", "curve", "swirl", "spiral", "arc", "tide"}, FaceSide | FaceSlope | FaceTop, 3.f, 20, 150, .6f, 1.2f, true, false, 2),
            rule({"ball", "circle", "ring", "bubble", "dot"}, FaceAll, 2.5f, 6, 50, .3f, .9f, false, true, 12),
            rule({"grass", "bush", "plant", "leaf", "kelp", "vine", "thorn"}, FaceTop | FaceBottom, 2.f, 14, 100, .6f, 1.1f, true, false, 0),
            rule({"cloud", "foam", "smoke"}, FaceTop | FaceSlope, 1.f, 30, 140, .7f, 1.3f, false, true, 8),
        },
        90.f,
        1.f};
}

static StyleDef makeLastHope() {
    return StyleDef{
        "Last Hope",
        "Sparse and luminous. Rays of light, drifting sparks and feathers in gold and pale blue.",
        {255, 208, 96},
        {150, 190, 255},
        {
            rule({"ray", "beam", "light", "glow", "shine"}, FaceTop | FaceSide | FaceSlope, 2.5f, 20, 150, .6f, 1.2f, true, false, 0),
            rule({"star", "spark", "diamond", "glint"}, FaceAll, 3.f, 6, 60, .3f, .8f, false, true, 14),
            rule({"ball", "circle", "ring", "orb"}, FaceAll, 1.5f, 6, 40, .3f, .7f, false, false, 12),
            rule({"arrow", "feather", "wing", "leaf"}, FaceTop | FaceSide, 1.5f, 12, 90, .6f, 1.f, true, true, 2),
        },
        110.f,
        .75f};
}

static StyleDef makeLimbo() {
    return StyleDef{
        "Limbo",
        "Dark silhouettes. Grass and branches along the floor, roots hanging from ceilings, thin mist.",
        {36, 38, 46},
        {112, 118, 130},
        {
            rule({"grass", "bush", "tree", "brush", "branch", "fern", "thorn"}, FaceTop | FaceSlope, 3.5f, 14, 110, .6f, 1.2f, true, false, 0),
            rule({"chain", "vine", "root", "rope", "drip"}, FaceBottom, 3.f, 20, 120, .6f, 1.2f, true, false, 0),
            rule({"spike", "thorn", "branch", "claw"}, FaceSide | FaceCorner, 1.5f, 12, 70, .6f, 1.f, true, false, 0),
            rule({"cloud", "mist", "smoke", "fog"}, FaceTop | FaceSlope | FaceSide, 1.f, 30, 150, .8f, 1.5f, false, true, 8),
        },
        60.f,
        .9f};
}

StyleDef const& styleDef(Style s) {
    static const StyleDef defs[kStyleCount] = {makeGrief(), makeTidalWave(), makeLastHope(), makeLimbo()};
    int i = static_cast<int>(s);
    if (i < 0 || i >= kStyleCount) i = 0;
    return defs[i];
}

}  // namespace autodeco
