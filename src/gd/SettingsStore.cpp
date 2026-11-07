#include "SettingsStore.hpp"
#include <algorithm>
#include <random>

using namespace geode::prelude;

namespace autodeco {

unsigned newSeed() {
    static std::mt19937 rng{std::random_device{}()};
    return static_cast<unsigned>(rng() % 900000u) + 1u;
}

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
    s.variant = std::clamp(m->getSavedValue<int>("variant", 0), 0, 1);
    s.layers = std::clamp(m->getSavedValue<int>("layers", 3), 1, 3);
    s.background = std::clamp(m->getSavedValue<int>("background", 1), 0, 2);
    s.backgroundColor = std::clamp(m->getSavedValue<int>("background-color", -2), -2, kSwatchCount - 1);
    s.lighting = std::clamp(m->getSavedValue<int>("lighting", 70), 0, 100);
    s.detail = std::clamp(m->getSavedValue<int>("detail", 50), 0, 100);
    s.variation = std::clamp(m->getSavedValue<int>("variation", 50), 0, 100);
    int seed = m->getSavedValue<int>("seed", 0);
    s.seed = seed != 0 ? static_cast<unsigned>(seed) : newSeed();
    return s;
}

void saveSettings(Settings const& s) {
    auto m = Mod::get();
    m->setSavedValue<int>("variant", s.variant);
    m->setSavedValue<int>("layers", s.layers);
    m->setSavedValue<int>("background", s.background);
    m->setSavedValue<int>("background-color", s.backgroundColor);
    m->setSavedValue<int>("lighting", s.lighting);
    m->setSavedValue<int>("detail", s.detail);
    m->setSavedValue<int>("variation", s.variation);
    m->setSavedValue<int>("seed", static_cast<int>(s.seed));
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
