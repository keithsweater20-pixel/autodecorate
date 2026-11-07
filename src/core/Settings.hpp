#pragma once
#include "Vec.hpp"

namespace autodeco {

enum class Style : int { Grief = 0, TidalWave = 1, LastHope = 2, Limbo = 3 };
enum class Pattern : int { Edge = 0, Scatter = 1, Rhythm = 2, Cluster = 3 };

constexpr int kStyleCount = 4;
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
};

}  // namespace autodeco
