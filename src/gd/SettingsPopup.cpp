#include "SettingsPopup.hpp"
#include "../core/Styles.hpp"
#include "Apply.hpp"
#include "SettingsStore.hpp"

using namespace geode::prelude;

namespace autodeco {

namespace {
constexpr char const* kOn = "GJ_button_01.png";   // green = selected
constexpr char const* kOff = "GJ_button_04.png";  // grey = not selected

CCLabelBMFont* makeLabel(char const* text, float scale, bool leftAligned) {
    auto l = CCLabelBMFont::create(text, "bigFont.fnt");
    l->setScale(scale);
    if (leftAligned) l->setAnchorPoint({0.f, .5f});
    return l;
}
}  // namespace

SettingsPopup* SettingsPopup::create() {
    auto ret = new SettingsPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SettingsPopup::init() {
    if (!Popup::init(380.f, 280.f)) return false;
    this->setTitle("Autodecorate");
    m_st = loadSettings();

    auto menu = m_buttonMenu;
    auto addItem = [&](CCNode* n, float x, float y, std::vector<CCNode*>* page) {
        menu->addChildAtPosition(n, Anchor::Center, ccp(x, y));
        if (page) page->push_back(n);
    };
    auto addText = [&](CCNode* n, float x, float y, std::vector<CCNode*>* page) {
        m_mainLayer->addChildAtPosition(n, Anchor::Center, ccp(x, y));
        if (page) page->push_back(n);
    };

    // Tabs
    m_tabLook = ButtonSprite::create("Look", 90, true, "bigFont.fnt", kOn, 28.f, .6f);
    m_tabColors = ButtonSprite::create("Colors", 90, true, "bigFont.fnt", kOff, 28.f, .6f);
    addItem(CCMenuItemSpriteExtra::create(m_tabLook, this, menu_selector(SettingsPopup::onTabLook)), -50.f, 92.f, nullptr);
    addItem(CCMenuItemSpriteExtra::create(m_tabColors, this, menu_selector(SettingsPopup::onTabColors)), 50.f, 92.f, nullptr);

    // ---- Look page ----
    addText(makeLabel("Style", .5f, true), -176.f, 66.f, &m_pageLook);
    static char const* styleNames[kStyleCount] = {"Grief", "Tidal Wave", "Last Hope", "Limbo"};
    for (int i = 0; i < kStyleCount; ++i) {
        auto spr = ButtonSprite::create(styleNames[i], 82, true, "bigFont.fnt", kOff, 30.f, .5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onStyle));
        btn->setTag(i);
        m_styleSprites.push_back(spr);
        addItem(btn, -132.f + i * 88.f, 40.f, &m_pageLook);
    }
    m_blurb = makeLabel("", .3f, false);
    addText(m_blurb, 0.f, 12.f, &m_pageLook);

    addText(makeLabel("Pattern", .5f, true), -176.f, -8.f, &m_pageLook);
    static char const* patternNames[kPatternCount] = {"Edge", "Scatter", "Rhythm", "Cluster"};
    for (int i = 0; i < kPatternCount; ++i) {
        auto spr = ButtonSprite::create(patternNames[i], 82, true, "bigFont.fnt", kOff, 30.f, .5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onPattern));
        btn->setTag(i);
        m_patternSprites.push_back(spr);
        addItem(btn, -132.f + i * 88.f, -32.f, &m_pageLook);
    }

    m_densityLabel = makeLabel("Density 50%", .45f, true);
    addText(m_densityLabel, -176.f, -64.f, &m_pageLook);
    auto slider = Slider::create(this, menu_selector(SettingsPopup::onDensity), .8f);
    slider->setValue(m_st.density / 100.f);
    addItem(slider, 50.f, -66.f, &m_pageLook);

    // ---- Colors page ----
    auto buildSwatches = [&](float y, bool primary, std::vector<CCSprite*>& borders, SEL_MenuHandler sel) {
        for (int i = 0; i < kSwatchCount + 1; ++i) {  // index 0 = style default
            auto border = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
            border->setScale(.5f);
            auto fill = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
            fill->setScale(.8f);
            fill->setPosition(border->getContentSize() / 2.f);
            if (i == 0) {
                fill->setColor({90, 90, 90});
                auto a = CCLabelBMFont::create("A", "bigFont.fnt");
                a->setScale(1.5f);
                a->setPosition(fill->getContentSize() / 2.f);
                fill->addChild(a);
            } else {
                RGB c = swatchColor(i - 1);
                fill->setColor({c.r, c.g, c.b});
            }
            border->addChild(fill);
            auto btn = CCMenuItemSpriteExtra::create(border, this, sel);
            btn->setTag(i - 1);
            borders.push_back(border);
            addItem(btn, -153.f + i * 34.f, y, &m_pageColors);
        }
        (void)primary;
    };
    addText(makeLabel("Main color  (A = style default)", .4f, true), -176.f, 66.f, &m_pageColors);
    buildSwatches(40.f, true, m_primaryBorders, menu_selector(SettingsPopup::onPrimary));
    addText(makeLabel("Detail color", .4f, true), -176.f, 10.f, &m_pageColors);
    buildSwatches(-14.f, false, m_secondaryBorders, menu_selector(SettingsPopup::onSecondary));

    auto addToggle = [&](char const* text, bool value, SEL_MenuHandler sel, float x, float y) {
        auto t = CCMenuItemToggler::createWithStandardSprites(this, sel, .7f);
        t->toggle(value);
        addItem(t, x, y, &m_pageColors);
        auto l = makeLabel(text, .4f, true);
        addText(l, x + 18.f, y, &m_pageColors);
    };
    addToggle("Selected objects only", m_st.selectedOnly, menu_selector(SettingsPopup::onSelectedOnly), -165.f, -50.f);
    addToggle("Add color triggers", m_st.makeColorTriggers, menu_selector(SettingsPopup::onTriggers), 20.f, -50.f);
    addToggle("Keep play area clear", m_st.keepPlayAreaClear, menu_selector(SettingsPopup::onKeepClear), -165.f, -76.f);

    // Apply (shown on both pages)
    auto apply = CCMenuItemSpriteExtra::create(ButtonSprite::create("Apply", 150, true, "goldFont.fnt", kOn, 34.f, .8f), this,
                                               menu_selector(SettingsPopup::onApply));
    addItem(apply, 0.f, -112.f, nullptr);

    refresh();
    showPage(false);
    return true;
}

void SettingsPopup::showPage(bool colors) {
    for (auto n : m_pageLook) n->setVisible(!colors);
    for (auto n : m_pageColors) n->setVisible(colors);
    m_tabLook->updateBGImage(colors ? kOff : kOn);
    m_tabColors->updateBGImage(colors ? kOn : kOff);
}

void SettingsPopup::refresh() {
    for (int i = 0; i < static_cast<int>(m_styleSprites.size()); ++i)
        m_styleSprites[i]->updateBGImage(i == static_cast<int>(m_st.style) ? kOn : kOff);
    for (int i = 0; i < static_cast<int>(m_patternSprites.size()); ++i)
        m_patternSprites[i]->updateBGImage(i == static_cast<int>(m_st.pattern) ? kOn : kOff);
    for (int i = 0; i < static_cast<int>(m_primaryBorders.size()); ++i)
        m_primaryBorders[i]->setColor(i - 1 == m_st.primaryColor ? ccColor3B{255, 255, 255} : ccColor3B{30, 30, 30});
    for (int i = 0; i < static_cast<int>(m_secondaryBorders.size()); ++i)
        m_secondaryBorders[i]->setColor(i - 1 == m_st.secondaryColor ? ccColor3B{255, 255, 255} : ccColor3B{30, 30, 30});
    m_densityLabel->setString(fmt::format("Density {}%", m_st.density).c_str());
    m_blurb->setString(styleDef(m_st.style).blurb);
    m_blurb->limitLabelWidth(350.f, .3f, .15f);
}

void SettingsPopup::onTabLook(CCObject*) { showPage(false); }
void SettingsPopup::onTabColors(CCObject*) { showPage(true); }

void SettingsPopup::onStyle(CCObject* sender) {
    m_st.style = static_cast<Style>(static_cast<CCNode*>(sender)->getTag());
    refresh();
}
void SettingsPopup::onPattern(CCObject* sender) {
    m_st.pattern = static_cast<Pattern>(static_cast<CCNode*>(sender)->getTag());
    refresh();
}
void SettingsPopup::onDensity(CCObject* sender) {
    m_st.density = static_cast<int>(std::round(static_cast<SliderThumb*>(sender)->getValue() * 100.f));
    refresh();
}
void SettingsPopup::onPrimary(CCObject* sender) {
    m_st.primaryColor = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
void SettingsPopup::onSecondary(CCObject* sender) {
    m_st.secondaryColor = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
// GD toggles report their old state inside the callback, so the new value is the inverse.
void SettingsPopup::onSelectedOnly(CCObject* sender) { m_st.selectedOnly = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }
void SettingsPopup::onTriggers(CCObject* sender) { m_st.makeColorTriggers = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }
void SettingsPopup::onKeepClear(CCObject* sender) { m_st.keepPlayAreaClear = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }

void SettingsPopup::onApply(CCObject*) {
    saveSettings(m_st);
    Settings chosen = m_st;
    this->onClose(nullptr);
    runAutodecorate(chosen);
}

}  // namespace autodeco
