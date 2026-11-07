#include "SettingsStore.hpp"
#include <algorithm>

using namespace geode::prelude;

namespace autodeco {

Settings loadSettings() {
    Settings s;
    auto m = Mod::get();
    s.style = static_cast<Style>(std::clamp(m->getSavedValue<int>("style", 0), 0, kStyleCount - 1));
    s.pattern = static_cast<Pattern>(std::clamp(m->getSavedValue<int>("pattern", 0), 0, kPatternCount - 1));
    s.density = std::clamp(m->getSavedValue<int>("density", 50), 0, 100);
    s.primaryColor = std::clamp(m->getSavedValue<int>("primary-color", -1), -1, kSwatchCount - 1);
    s.secondaryColor = std::clamp(m->getSavedValue<int>("secondary-color", -1), -1, kSwatchCount - 1);
    s.selectedOnly = m->getSavedValue<bool>("selected-only", false);
    s.makeColorTriggers = m->getSavedValue<bool>("color-triggers", true);
    s.keepPlayAreaClear = m->getSavedValue<bool>("keep-clear", true);
    return s;
}

void saveSettings(Settings const& s) {
    auto m = Mod::get();
    m->setSavedValue<int>("style", static_cast<int>(s.style));
    m->setSavedValue<int>("pattern", static_cast<int>(s.pattern));
    m->setSavedValue<int>("density", s.density);
    m->setSavedValue<int>("primary-color", s.primaryColor);
    m->setSavedValue<int>("secondary-color", s.secondaryColor);
    m->setSavedValue<bool>("selected-only", s.selectedOnly);
    m->setSavedValue<bool>("color-triggers", s.makeColorTriggers);
    m->setSavedValue<bool>("keep-clear", s.keepPlayAreaClear);
}

}  // namespace autodeco
