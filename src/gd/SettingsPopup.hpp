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
    int m_page = 0;  // 0 Look, 1 Colors, 2 More
    std::vector<ButtonSprite*> m_styleSprites, m_variantSprites, m_patternSprites, m_layerSprites, m_backgroundSprites;
    std::vector<cocos2d::CCSprite*> m_primaryBorders, m_secondaryBorders, m_bgBorders;
    std::vector<cocos2d::CCNode*> m_pages[3];
    std::vector<cocos2d::CCNode*> m_variantNodes;
    std::vector<std::pair<Slider*, int>> m_sliders;  // slider and the page it lives on
    ButtonSprite* m_tabs[3] = {nullptr, nullptr, nullptr};
    cocos2d::CCLabelBMFont* m_densityLabel = nullptr;
    cocos2d::CCLabelBMFont* m_lightingLabel = nullptr;
    cocos2d::CCLabelBMFont* m_detailLabel = nullptr;
    cocos2d::CCLabelBMFont* m_variationLabel = nullptr;
    cocos2d::CCLabelBMFont* m_seedLabel = nullptr;
    cocos2d::CCLabelBMFont* m_blurb = nullptr;

    void showPage(int page);
    void refresh();
    void onTab(cocos2d::CCObject* sender);
    void onStyle(cocos2d::CCObject* sender);
    void onVariant(cocos2d::CCObject* sender);
    void onPattern(cocos2d::CCObject* sender);
    void onDensity(cocos2d::CCObject* sender);
    void onLighting(cocos2d::CCObject* sender);
    void onDetail(cocos2d::CCObject* sender);
    void onVariation(cocos2d::CCObject* sender);
    void onLayers(cocos2d::CCObject* sender);
    void onBackground(cocos2d::CCObject* sender);
    void onNewSeed(cocos2d::CCObject*);
    void onPrimary(cocos2d::CCObject* sender);
    void onSecondary(cocos2d::CCObject* sender);
    void onBackgroundColor(cocos2d::CCObject* sender);
    void onSelectedOnly(cocos2d::CCObject* sender);
    void onTriggers(cocos2d::CCObject* sender);
    void onKeepClear(cocos2d::CCObject* sender);
    void onPreview(cocos2d::CCObject*);
    void onGenerate(cocos2d::CCObject*);
    void onRegenerate(cocos2d::CCObject*);
    void onUndo(cocos2d::CCObject*);
};

}  // namespace autodeco
