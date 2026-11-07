#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "gd/ButtonPlacement.hpp"
#include "gd/SettingsPopup.hpp"

using namespace geode::prelude;

class $modify(AutoDecorateEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;
        autodeco::attachButton(this, this, menu_selector(AutoDecorateEditorUI::onAutodecorate));
        return true;
    }

    void onAutodecorate(CCObject*) {
        if (auto popup = autodeco::SettingsPopup::create()) popup->show();
    }
};
