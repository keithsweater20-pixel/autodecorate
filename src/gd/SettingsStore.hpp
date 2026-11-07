#pragma once
#include <Geode/Geode.hpp>
#include "../core/Settings.hpp"

namespace autodeco {

// A fresh random seed in 1..900000 (small enough to read and retype).
unsigned newSeed();
Settings loadSettings();
void saveSettings(Settings const& s);
}  // namespace autodeco
