#pragma once
#include "../core/Settings.hpp"

namespace autodeco {
// Scans the open level, plans the decoration and creates it over a few frames.
// Called when the player presses Apply in the Autodecorate menu.
void runAutodecorate(Settings const& settings);
}  // namespace autodeco
