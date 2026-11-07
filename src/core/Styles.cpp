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
                     float scaleMin, float scaleMax, bool align, bool secondary, float reach, int layer = 1) {
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
    r.layer = layer;
    return r;
}

// The looks are interpretations of the style names (palette + the kind of shapes that suit the
// mood). They are data, so tuning one is a table edit. Decorations are found at runtime by
// texture name and size, never by object ID. Layers: 0 back, 1 middle, 2 front (all behind the layout).
static StyleDef makeGrief() {
    return StyleDef{
        "Grief",
        "Hell style. Black and gray, blood-red thorns and chains, white neon glints.",
        {210, 30, 48},
        {235, 235, 248},
        {10, 10, 13},
        {
            rule({"spike", "thorn", "hook", "claw", "tooth", "fang", "pit"}, FaceTop | FaceSide | FaceSlope, 3.f, 12, 70, .6f, 1.1f, true, false, 0, 1),
            rule({"chain", "drip", "tear", "rope", "tentacle", "vine"}, FaceBottom, 2.5f, 20, 120, .6f, 1.2f, true, false, 0, 1),
            rule({"skull", "bone", "cross", "eye", "monster", "demon"}, FaceTop | FaceSide, .8f, 24, 110, .7f, 1.1f, false, false, 4, 1),
            rule({"cloud", "smoke", "mist", "fog"}, FaceTop | FaceSlope, 1.f, 30, 140, .8f, 1.4f, false, false, 6, 0),
            rule({"ring", "circle", "ball", "orb", "dot", "glow", "line"}, FaceAll, 2.f, 6, 45, .3f, .7f, false, true, 10, 2),
        },
        {
            rule({"spike", "thorn", "chain", "skull", "cross", "monster", "pit"}, FaceAll, 2.f, 80, 320, .9f, 1.6f, false, false, 0, 0),
            rule({"cloud", "smoke", "fog", "ring", "circle", "glow"}, FaceAll, 1.f, 80, 320, 1.f, 1.8f, false, true, 0, 0),
        },
        70.f,
        1.f};
}

static StyleDef makeTidalUnderwater() {
    return StyleDef{
        "Tidal Wave",
        "Underwater. Curling waves, rising bubbles and kelp in cyan and deep blue.",
        {0, 176, 230},
        {24, 92, 210},
        {4, 22, 52},
        {
            rule({"wave", "curve", "swirl", "spiral", "arc", "tide", "ripple"}, FaceSide | FaceSlope | FaceTop, 3.f, 20, 150, .6f, 1.2f, true, false, 2, 1),
            rule({"ball", "circle", "ring", "bubble", "dot"}, FaceAll, 2.5f, 6, 50, .3f, .9f, false, true, 12, 2),
            rule({"grass", "bush", "plant", "leaf", "kelp", "vine", "coral", "fan", "thorn"}, FaceTop | FaceBottom, 2.f, 14, 100, .6f, 1.1f, true, false, 0, 1),
            rule({"cloud", "foam", "smoke", "fish", "shell"}, FaceTop | FaceSlope, 1.f, 30, 140, .7f, 1.3f, false, true, 8, 0),
        },
        {
            rule({"wave", "curve", "swirl", "arc", "cloud", "ring", "circle", "ball"}, FaceAll, 2.f, 80, 320, 1.f, 1.8f, false, false, 0, 0),
            rule({"cloud", "foam", "bubble", "glow"}, FaceAll, 1.f, 80, 320, 1.f, 1.6f, false, true, 0, 0),
        },
        90.f,
        .9f};
}

static StyleDef makeTidalBeach() {
    return StyleDef{
        "Tidal Wave (Beach)",
        "Beach. Bright sky blue with orange and purple, wave curls, clouds and sun shapes.",
        {255, 152, 56},
        {146, 92, 226},
        {112, 202, 246},
        {
            rule({"wave", "curve", "swirl", "arc", "tide", "ripple"}, FaceSide | FaceSlope | FaceTop, 3.f, 20, 150, .6f, 1.2f, true, false, 2, 1),
            rule({"ball", "circle", "ring", "sun", "dot"}, FaceAll, 2.f, 6, 50, .3f, .9f, false, true, 12, 2),
            rule({"grass", "leaf", "plant", "palm", "bush", "vine"}, FaceTop | FaceBottom, 1.5f, 14, 100, .6f, 1.1f, true, false, 0, 1),
            rule({"cloud", "foam", "smoke"}, FaceTop | FaceSlope, 1.5f, 30, 140, .7f, 1.3f, false, true, 8, 0),
        },
        {
            rule({"cloud", "sun", "circle", "ring", "ball", "wave"}, FaceAll, 2.f, 80, 320, 1.f, 1.8f, false, false, 0, 0),
            rule({"cloud", "foam", "swirl", "glow"}, FaceAll, 1.f, 80, 320, 1.f, 1.6f, false, true, 0, 0),
        },
        100.f,
        .9f};
}

static StyleDef makeLastHope() {
    return StyleDef{
        "Last Hope",
        "Cinematic. Dark sky with luminous rays, drifting sparks and feathers in gold and pale blue.",
        {255, 208, 96},
        {170, 205, 255},
        {8, 10, 26},
        {
            rule({"ray", "beam", "light", "glow", "shine"}, FaceTop | FaceSide | FaceSlope, 2.5f, 20, 150, .6f, 1.2f, true, false, 0, 1),
            rule({"star", "spark", "diamond", "glint"}, FaceAll, 3.f, 6, 60, .3f, .8f, false, true, 14, 2),
            rule({"ball", "circle", "ring", "orb"}, FaceAll, 1.5f, 6, 40, .3f, .7f, false, false, 12, 2),
            rule({"arrow", "feather", "wing", "leaf"}, FaceTop | FaceSide, 1.5f, 12, 90, .6f, 1.f, true, true, 2, 1),
            rule({"cloud", "mist", "smoke", "fog"}, FaceTop | FaceSlope, .8f, 30, 140, .8f, 1.4f, false, true, 8, 0),
        },
        {
            rule({"ray", "beam", "glow", "star", "ring", "circle"}, FaceAll, 2.f, 80, 320, .9f, 1.8f, false, false, 0, 0),
            rule({"cloud", "mist", "smoke", "fog"}, FaceAll, 1.f, 80, 320, 1.f, 1.8f, false, true, 0, 0),
        },
        110.f,
        .8f};
}

static StyleDef makeLimbo() {
    return StyleDef{
        "Limbo",
        "Surreal. A dark void with glowing teal and magenta shapes, strange roots and symbols.",
        {64, 230, 208},
        {232, 84, 206},
        {8, 6, 18},
        {
            rule({"diamond", "star", "arrow", "triangle", "rhombus", "square", "hex"}, FaceAll, 2.f, 10, 70, .4f, .9f, false, true, 10, 2),
            rule({"ring", "circle", "ball", "orb", "eye"}, FaceAll, 2.f, 8, 60, .4f, .9f, false, false, 12, 2),
            rule({"chain", "vine", "root", "rope", "drip"}, FaceBottom, 2.f, 20, 120, .6f, 1.2f, true, false, 0, 1),
            rule({"grass", "bush", "thorn", "branch", "fern", "plant", "leaf"}, FaceTop | FaceSlope, 2.5f, 14, 110, .6f, 1.2f, true, false, 0, 1),
            rule({"cloud", "mist", "smoke", "fog"}, FaceTop | FaceSlope | FaceSide, 1.f, 30, 150, .8f, 1.5f, false, true, 8, 0),
        },
        {
            rule({"ring", "circle", "eye", "star", "diamond", "ball"}, FaceAll, 2.f, 80, 320, .9f, 1.8f, false, true, 0, 0),
            rule({"cloud", "mist", "smoke", "fog"}, FaceAll, 1.f, 80, 320, 1.f, 1.8f, false, false, 0, 0),
        },
        80.f,
        1.f};
}

StyleDef const& styleDef(Style s, int variant) {
    static const StyleDef defs[kStyleDefCount] = {makeGrief(), makeTidalUnderwater(), makeLastHope(), makeLimbo(), makeTidalBeach()};
    int i = static_cast<int>(s);
    if (i < 0 || i >= kStyleCount) i = 0;
    if (s == Style::TidalWave && variant == 1) i = 4;
    return defs[i];
}

}  // namespace autodeco
