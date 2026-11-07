#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "../core/Settings.hpp"

namespace autodeco {

class SettingsPopup : public geode::Popup {
public:
    static SettingsPopup* create();

protected:
    bool init();

    Settings m_st;
    std::vector<ButtonSprite*> m_styleSprites, m_patternSprites;
    std::vector<cocos2d::CCSprite*> m_primaryBorders, m_secondaryBorders;
    std::vector<cocos2d::CCNode*> m_pageLook, m_pageColors;
    ButtonSprite* m_tabLook = nullptr;
    ButtonSprite* m_tabColors = nullptr;
    cocos2d::CCLabelBMFont* m_densityLabel = nullptr;
    cocos2d::CCLabelBMFont* m_blurb = nullptr;

    void showPage(bool colors);
    void refresh();
    void onTabLook(cocos2d::CCObject*);
    void onTabColors(cocos2d::CCObject*);
    void onStyle(cocos2d::CCObject* sender);
    void onPattern(cocos2d::CCObject* sender);
    void onDensity(cocos2d::CCObject* sender);
    void onPrimary(cocos2d::CCObject* sender);
    void onSecondary(cocos2d::CCObject* sender);
    void onSelectedOnly(cocos2d::CCObject* sender);
    void onTriggers(cocos2d::CCObject* sender);
    void onKeepClear(cocos2d::CCObject* sender);
    void onApply(cocos2d::CCObject*);
};

}  // namespace autodeco
