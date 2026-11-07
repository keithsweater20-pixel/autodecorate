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

float sliderValue(CCObject* sender) { return static_cast<SliderThumb*>(sender)->getValue(); }
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
    if (!Popup::init(420.f, 320.f)) return false;
    this->setTitle("Autodecorate");
    m_st = loadSettings();

    auto menu = m_buttonMenu;
    auto addItem = [&](CCNode* n, float x, float y, int page) {
        menu->addChildAtPosition(n, Anchor::Center, ccp(x, y));
        if (page >= 0) m_pages[page].push_back(n);
    };
    auto addText = [&](CCNode* n, float x, float y, int page) {
        m_mainLayer->addChildAtPosition(n, Anchor::Center, ccp(x, y));
        if (page >= 0) m_pages[page].push_back(n);
    };
    auto addButton = [&](char const* text, int width, float scale, SEL_MenuHandler sel, int tag, float x, float y, int page,
                         std::vector<ButtonSprite*>* sprites) {
        auto spr = ButtonSprite::create(text, width, true, "bigFont.fnt", kOff, 28.f, scale);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, sel);
        btn->setTag(tag);
        if (sprites) sprites->push_back(spr);
        addItem(btn, x, y, page);
        return btn;
    };

    // Tabs
    static char const* tabNames[3] = {"Look", "Colors", "More"};
    for (int i = 0; i < 3; ++i) {
        m_tabs[i] = ButtonSprite::create(tabNames[i], 84, true, "bigFont.fnt", kOff, 26.f, .6f);
        auto btn = CCMenuItemSpriteExtra::create(m_tabs[i], this, menu_selector(SettingsPopup::onTab));
        btn->setTag(i);
        addItem(btn, -90.f + i * 90.f, 124.f, -1);
    }

    // ---- Look page ----
    addText(makeLabel("Style", .5f, true), -196.f, 86.f, 0);
    static char const* styleNames[kStyleCount] = {"Grief", "Tidal Wave", "Last Hope", "Limbo"};
    for (int i = 0; i < kStyleCount; ++i)
        addButton(styleNames[i], 92, .5f, menu_selector(SettingsPopup::onStyle), i, -150.f + i * 100.f, 62.f, 0, &m_styleSprites);
    m_blurb = makeLabel("", .3f, false);
    addText(m_blurb, 0.f, 38.f, 0);

    auto variantLabel = makeLabel("Look", .5f, true);
    addText(variantLabel, -196.f, 14.f, 0);
    m_variantNodes.push_back(variantLabel);
    m_variantNodes.push_back(addButton("Underwater", 110, .45f, menu_selector(SettingsPopup::onVariant), 0, -90.f, 14.f, 0, &m_variantSprites));
    m_variantNodes.push_back(addButton("Beach", 110, .45f, menu_selector(SettingsPopup::onVariant), 1, 30.f, 14.f, 0, &m_variantSprites));

    addText(makeLabel("Pattern", .5f, true), -196.f, -12.f, 0);
    static char const* patternNames[kPatternCount] = {"Edge", "Scatter", "Rhythm", "Cluster"};
    for (int i = 0; i < kPatternCount; ++i)
        addButton(patternNames[i], 92, .5f, menu_selector(SettingsPopup::onPattern), i, -150.f + i * 100.f, -34.f, 0, &m_patternSprites);

    auto addSlider = [&](CCLabelBMFont*& label, char const* text, SEL_MenuHandler sel, float value, float y, int page) {
        label = makeLabel(text, .45f, true);
        addText(label, -196.f, y, page);
        auto slider = Slider::create(this, sel, .8f);
        slider->setValue(value);
        addItem(slider, 60.f, y - 2.f, page);
        m_sliders.push_back({slider, page});
    };
    addSlider(m_densityLabel, "Density 50%", menu_selector(SettingsPopup::onDensity), m_st.density / 100.f, -64.f, 0);

    // ---- Colors page ----
    addText(makeLabel("Main color  (A = style default)", .4f, true), -196.f, 98.f, 1);
    // Main/detail: button 0 is "A" (value -1), then 9 swatches (values 0..8). Tags are stored as value+1.
    {
        for (int row = 0; row < 2; ++row) {
            float y = row == 0 ? 78.f : 36.f;
            auto& borders = row == 0 ? m_primaryBorders : m_secondaryBorders;
            auto sel = row == 0 ? menu_selector(SettingsPopup::onPrimary) : menu_selector(SettingsPopup::onSecondary);
            for (int i = 0; i < kSwatchCount + 1; ++i) {
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
                btn->setTag(i - 1);  // value: -1 = style default, 0.. = swatch
                borders.push_back(border);
                addItem(btn, -153.f + i * 34.f, y, 1);
            }
        }
    }
    addText(makeLabel("Detail color", .4f, true), -196.f, 56.f, 1);
    auto specialRow = [&](float y, std::vector<CCSprite*>& borders, SEL_MenuHandler sel) {
        for (int i = 0; i < kSwatchCount + 2; ++i) {  // X, A, then the swatches
            auto border = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
            border->setScale(.5f);
            auto fill = CCSprite::createWithSpriteFrameName("whiteSquare60_001.png");
            fill->setScale(.8f);
            fill->setPosition(border->getContentSize() / 2.f);
            if (i < 2) {
                fill->setColor({90, 90, 90});
                auto a = CCLabelBMFont::create(i == 0 ? "X" : "A", "bigFont.fnt");
                a->setScale(1.5f);
                a->setPosition(fill->getContentSize() / 2.f);
                fill->addChild(a);
            } else {
                RGB c = swatchColor(i - 2);
                fill->setColor({c.r, c.g, c.b});
            }
            border->addChild(fill);
            auto btn = CCMenuItemSpriteExtra::create(border, this, sel);
            btn->setTag(i - 2);  // value: -2 = leave alone, -1 = style colour, 0.. = swatch
            borders.push_back(border);
            addItem(btn, -160.f + i * 32.f, y, 1);
        }
    };
    addText(makeLabel("Background color  (X = leave alone, A = style color)", .4f, true), -196.f, 14.f, 1);
    specialRow(-6.f, m_bgBorders, menu_selector(SettingsPopup::onBackgroundColor));
    addText(makeLabel("Block color  (recolors your layout blocks)", .4f, true), -196.f, -28.f, 1);
    specialRow(-48.f, m_blockBorders, menu_selector(SettingsPopup::onBlockColor));

    auto addToggle = [&](char const* text, bool value, SEL_MenuHandler sel, float x, float y) {
        auto t = CCMenuItemToggler::createWithStandardSprites(this, sel, .7f);
        t->toggle(value);
        addItem(t, x, y, 1);
        addText(makeLabel(text, .4f, true), x + 18.f, y, 1);
    };
    addToggle("Selected objects only", m_st.selectedOnly, menu_selector(SettingsPopup::onSelectedOnly), -190.f, -78.f);
    addToggle("Add color triggers", m_st.makeColorTriggers, menu_selector(SettingsPopup::onTriggers), 20.f, -78.f);
    addToggle("Keep play area clear", m_st.keepPlayAreaClear, menu_selector(SettingsPopup::onKeepClear), -190.f, -98.f);

    // ---- More page ----
    addText(makeLabel("Layers", .5f, true), -196.f, 86.f, 2);
    for (int i = 0; i < 3; ++i)
        addButton(std::to_string(i + 1).c_str(), 50, .5f, menu_selector(SettingsPopup::onLayers), i + 1, -100.f + i * 56.f, 86.f, 2, &m_layerSprites);
    addText(makeLabel("Background", .5f, true), -196.f, 58.f, 2);
    static char const* bgNames[3] = {"Off", "Light", "Full"};
    for (int i = 0; i < 3; ++i)
        addButton(bgNames[i], 50, .5f, menu_selector(SettingsPopup::onBackground), i, -100.f + i * 56.f, 58.f, 2, &m_backgroundSprites);
    addSlider(m_lightingLabel, "Lighting 70%", menu_selector(SettingsPopup::onLighting), m_st.lighting / 100.f, 28.f, 2);
    addSlider(m_detailLabel, "Detail 50%", menu_selector(SettingsPopup::onDetail), m_st.detail / 100.f, 2.f, 2);
    addSlider(m_variationLabel, "Variation 50%", menu_selector(SettingsPopup::onVariation), m_st.variation / 100.f, -24.f, 2);
    m_seedLabel = makeLabel("Seed 1", .45f, true);
    addText(m_seedLabel, -196.f, -52.f, 2);
    addButton("New seed", 100, .45f, menu_selector(SettingsPopup::onNewSeed), 0, 20.f, -52.f, 2, nullptr);
    addText(makeLabel("Same seed + same settings = same result", .3f, true), -196.f, -76.f, 2);

    // ---- Actions (every page) ----
    float ay = -126.f;
    addItem(CCMenuItemSpriteExtra::create(ButtonSprite::create("Preview", 92, true, "bigFont.fnt", kOff, 28.f, .5f), this,
                                          menu_selector(SettingsPopup::onPreview)), -150.f, ay, -1);
    addItem(CCMenuItemSpriteExtra::create(ButtonSprite::create("Generate", 92, true, "goldFont.fnt", kOn, 28.f, .5f), this,
                                          menu_selector(SettingsPopup::onGenerate)), -50.f, ay, -1);
    addItem(CCMenuItemSpriteExtra::create(ButtonSprite::create("Regenerate", 92, true, "bigFont.fnt", kOff, 28.f, .45f), this,
                                          menu_selector(SettingsPopup::onRegenerate)), 50.f, ay, -1);
    addItem(CCMenuItemSpriteExtra::create(ButtonSprite::create("Undo", 92, true, "bigFont.fnt", kOff, 28.f, .5f), this,
                                          menu_selector(SettingsPopup::onUndo)), 150.f, ay, -1);

    refresh();
    showPage(0);
    return true;
}

void SettingsPopup::showPage(int page) {
    m_page = page;
    for (int p = 0; p < 3; ++p)
        for (auto n : m_pages[p]) n->setVisible(p == page);
    // The Underwater/Beach choice only exists for Tidal Wave.
    if (page == 0)
        for (auto n : m_variantNodes) n->setVisible(m_st.style == Style::TidalWave);
    for (auto const& [slider, p] : m_sliders) {
        if (p == page) slider->enableSlider();
        else slider->disableSlider();
    }
    for (int i = 0; i < 3; ++i) m_tabs[i]->updateBGImage(i == page ? kOn : kOff);
}

void SettingsPopup::refresh() {
    auto mark = [](std::vector<ButtonSprite*>& v, int selected) {
        for (int i = 0; i < static_cast<int>(v.size()); ++i) v[i]->updateBGImage(i == selected ? kOn : kOff);
    };
    mark(m_styleSprites, static_cast<int>(m_st.style));
    mark(m_variantSprites, m_st.variant);
    mark(m_patternSprites, static_cast<int>(m_st.pattern));
    mark(m_layerSprites, m_st.layers - 1);
    mark(m_backgroundSprites, m_st.background);
    auto outline = [](std::vector<CCSprite*>& v, int selectedValue, int offset) {
        for (int i = 0; i < static_cast<int>(v.size()); ++i)
            v[i]->setColor(i - offset == selectedValue ? ccColor3B{255, 255, 255} : ccColor3B{30, 30, 30});
    };
    outline(m_primaryBorders, m_st.primaryColor, 1);
    outline(m_secondaryBorders, m_st.secondaryColor, 1);
    outline(m_bgBorders, m_st.backgroundColor, 2);
    outline(m_blockBorders, m_st.blockColor, 2);
    m_densityLabel->setString(fmt::format("Density {}%", m_st.density).c_str());
    m_lightingLabel->setString(fmt::format("Lighting {}%", m_st.lighting).c_str());
    m_detailLabel->setString(fmt::format("Detail {}%", m_st.detail).c_str());
    m_variationLabel->setString(fmt::format("Variation {}%", m_st.variation).c_str());
    m_seedLabel->setString(fmt::format("Seed {}", m_st.seed).c_str());
    m_blurb->setString(styleDef(m_st.style, m_st.variant).blurb);
    m_blurb->limitLabelWidth(390.f, .3f, .15f);
    if (m_page == 0)
        for (auto n : m_variantNodes) n->setVisible(m_st.style == Style::TidalWave);
}

void SettingsPopup::onTab(CCObject* sender) { showPage(static_cast<CCNode*>(sender)->getTag()); }
void SettingsPopup::onStyle(CCObject* sender) {
    m_st.style = static_cast<Style>(static_cast<CCNode*>(sender)->getTag());
    refresh();
}
void SettingsPopup::onVariant(CCObject* sender) {
    m_st.variant = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
void SettingsPopup::onPattern(CCObject* sender) {
    m_st.pattern = static_cast<Pattern>(static_cast<CCNode*>(sender)->getTag());
    refresh();
}
void SettingsPopup::onDensity(CCObject* sender) {
    m_st.density = static_cast<int>(std::round(sliderValue(sender) * 100.f));
    refresh();
}
void SettingsPopup::onLighting(CCObject* sender) {
    m_st.lighting = static_cast<int>(std::round(sliderValue(sender) * 100.f));
    refresh();
}
void SettingsPopup::onDetail(CCObject* sender) {
    m_st.detail = static_cast<int>(std::round(sliderValue(sender) * 100.f));
    refresh();
}
void SettingsPopup::onVariation(CCObject* sender) {
    m_st.variation = static_cast<int>(std::round(sliderValue(sender) * 100.f));
    refresh();
}
void SettingsPopup::onLayers(CCObject* sender) {
    m_st.layers = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
void SettingsPopup::onBackground(CCObject* sender) {
    m_st.background = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
void SettingsPopup::onNewSeed(CCObject*) {
    m_st.seed = newSeed();
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
void SettingsPopup::onBackgroundColor(CCObject* sender) {
    m_st.backgroundColor = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
void SettingsPopup::onBlockColor(CCObject* sender) {
    m_st.blockColor = static_cast<CCNode*>(sender)->getTag();
    refresh();
}
// GD toggles report their old state inside the callback, so the new value is the inverse.
void SettingsPopup::onSelectedOnly(CCObject* sender) { m_st.selectedOnly = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }
void SettingsPopup::onTriggers(CCObject* sender) { m_st.makeColorTriggers = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }
void SettingsPopup::onKeepClear(CCObject* sender) { m_st.keepPlayAreaClear = !static_cast<CCMenuItemToggler*>(sender)->isToggled(); }

void SettingsPopup::onPreview(CCObject*) {
    saveSettings(m_st);
    Settings chosen = m_st;
    this->onClose(nullptr);
    previewAutodecorate(chosen);
}
void SettingsPopup::onGenerate(CCObject*) {
    saveSettings(m_st);
    Settings chosen = m_st;
    this->onClose(nullptr);
    runAutodecorate(chosen);
}
void SettingsPopup::onRegenerate(CCObject*) {
    m_st.seed = newSeed();  // a different result than last time
    saveSettings(m_st);
    Settings chosen = m_st;
    this->onClose(nullptr);
    regenerateAutodecorate(chosen);
}
void SettingsPopup::onUndo(CCObject*) {
    saveSettings(m_st);
    this->onClose(nullptr);
    undoAutodecorate();
}

}  // namespace autodeco
