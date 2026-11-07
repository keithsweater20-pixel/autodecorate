#pragma once
#include "Vec.hpp"

namespace autodeco {

enum class Style : int { Grief = 0, TidalWave = 1, LastHope = 2, Limbo = 3 };
enum class Pattern : int { Edge = 0, Scatter = 1, Rhythm = 2, Cluster = 3 };

constexpr int kStyleCount = 4;
constexpr int kStyleDefCount = 5;  // Tidal Wave has two looks
constexpr int kPatternCount = 4;
constexpr int kSwatchCount = 9;

// Swatch colours offered in the menu. Index -1 means "use the style's own colour".
RGB swatchColor(int index);

struct Settings {
    Style style = Style::Grief;
    Pattern pattern = Pattern::Edge;
    int density = 50;              // 0..100
    int primaryColor = -1;         // -1 = style default, otherwise swatch index
    int secondaryColor = -1;
    bool selectedOnly = false;     // decorate only the selected layout objects
    bool makeColorTriggers = true; // add two colour triggers so the chosen colours show up
    bool keepPlayAreaClear = true; // cap deco height above floors so the path stays readable
    int primaryChannel = 997;      // colour channels the decoration uses
    int secondaryChannel = 998;

    // --- generation controls ---
    int variant = 0;               // Tidal Wave only: 0 = Underwater, 1 = Beach
    int layers = 3;                // 1..3 depth layers (1 = everything on one layer)
    int background = 1;            // 0 off, 1 light, 2 full background decoration
    int blockColor = -1;           // colour of the layout blocks: -2 = leave alone, -1 = style colour, else swatch
    int backgroundColor = -2;      // -2 = leave the level's background alone, -1 = style colour, else swatch
    int lighting = 70;             // 0..100, brightness of the detail colour
    int detail = 50;               // 0..100, how many small accent objects
    int variation = 50;            // 0..100, size / rotation randomness
    unsigned seed = 1;             // same seed + same settings + same layout = same result
};

}  // namespace autodeco
