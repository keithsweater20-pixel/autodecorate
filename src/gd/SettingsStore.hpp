#pragma once
#include <Geode/Geode.hpp>
#include "../core/Settings.hpp"

namespace autodeco {
Settings loadSettings();
void saveSettings(Settings const& s);
}  // namespace autodeco
