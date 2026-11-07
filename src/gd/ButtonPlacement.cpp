#include "ButtonPlacement.hpp"
#include <algorithm>
#include <cctype>

using namespace geode::prelude;

namespace autodeco {

namespace {

std::string lowered(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void collectLabels(CCNode* n, std::vector<CCLabelBMFont*>& out) {
    if (!n) return;
    if (auto l = typeinfo_cast<CCLabelBMFont*>(n)) out.push_back(l);
    if (auto kids = n->getChildren())
        for (auto* c : CCArrayExt<CCNode*>(kids)) collectLabels(c, out);
}

// The Copy button is found by its node ID or by the text "Copy" on it, so it works whether
// or not another mod has renamed things.
CCMenuItemSpriteExtra* findCopyButton(CCNode* node) {
    if (auto b = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
        std::string id = lowered(b->getID());
        if (id.find("copy") != std::string::npos && id.find("paste") == std::string::npos) return b;
        std::vector<CCLabelBMFont*> labels;
        collectLabels(b->getNormalImage(), labels);
        for (auto* l : labels)
            if (lowered(l->getString()) == "copy") return b;
    }
    if (auto kids = node->getChildren())
        for (auto* c : CCArrayExt<CCNode*>(kids))
            if (auto r = findCopyButton(c)) return r;
    return nullptr;
}

// Renders the Copy button's artwork with its text hidden, so the background, border and
// colours are copied exactly. Returns null if the artwork cannot be captured.
CCSprite* snapshotWithoutText(CCNode* img) {
    auto size = img->getContentSize();
    if (size.width < 4.f || size.height < 4.f) return nullptr;
    std::vector<CCLabelBMFont*> labels;
    collectLabels(img, labels);
    std::vector<bool> wasVisible;
    for (auto* l : labels) {
        wasVisible.push_back(l->isVisible());
        l->setVisible(false);
    }
    auto oldPos = img->getPosition();
    auto oldScale = img->getScale();
    auto oldAnchor = img->getAnchorPoint();
    img->setScale(1.f);
    img->setAnchorPoint({.5f, .5f});
    img->setPosition({size.width / 2.f, size.height / 2.f});

    CCSprite* result = nullptr;
    if (auto rt = CCRenderTexture::create(static_cast<int>(size.width), static_cast<int>(size.height))) {
        rt->begin();
        img->visit();
        rt->end();
        result = CCSprite::createWithTexture(rt->getSprite()->getTexture());
        if (result) result->setFlipY(true);
    }

    img->setScale(oldScale);
    img->setAnchorPoint(oldAnchor);
    img->setPosition(oldPos);
    for (size_t i = 0; i < labels.size(); ++i) labels[i]->setVisible(wasVisible[i]);
    return result;
}

CCMenuItemSpriteExtra* makeButton(CCMenuItemSpriteExtra* copy, CCObject* target, SEL_MenuHandler cb) {
    CCNode* img = copy->getNormalImage();
    std::vector<CCLabelBMFont*> labels;
    collectLabels(img, labels);
    CCLabelBMFont* refLabel = labels.empty() ? nullptr : labels.front();

    CCNode* normal = nullptr;
    if (img && refLabel) {
        if (auto bg = snapshotWithoutText(img)) {
            auto size = bg->getContentSize();
            auto label = CCLabelBMFont::create("Autodecorate", refLabel->getFntFile());
            label->setColor(refLabel->getColor());
            label->limitLabelWidth(size.width * .9f, std::max(refLabel->getScale(), .2f), .1f);
            label->setPosition({size.width / 2.f, size.height / 2.f});
            bg->addChild(label);
            bg->setScale(img->getScale());
            normal = bg;
        }
    }
    if (!normal) {
        // Could not copy the artwork: fall back to a standard GD text button of the same size.
        auto size = img ? img->getContentSize() : CCSize{80.f, 30.f};
        normal = ButtonSprite::create("Autodecorate", static_cast<int>(std::max(size.width, 90.f)), true, "bigFont.fnt",
                                      "GJ_button_04.png", std::max(size.height, 26.f), .5f);
    }
    auto btn = CCMenuItemSpriteExtra::create(normal, nullptr, target, cb);
    btn->setID("autodecorate-button"_spr);
    return btn;
}

}  // namespace

void attachButton(EditorUI* ui, CCObject* target, SEL_MenuHandler callback) {
    auto copy = findCopyButton(ui);
    if (!copy || !copy->getParent()) {
        // Copy button not found (layout changed?): keep the mod usable with a standalone button.
        log::warn("Autodecorate: could not find the Copy button, adding a standalone button instead");
        auto sprite = ButtonSprite::create("Autodecorate", 100, true, "bigFont.fnt", "GJ_button_04.png", 28.f, .5f);
        auto btn = CCMenuItemSpriteExtra::create(sprite, nullptr, target, callback);
        btn->setID("autodecorate-button"_spr);
        auto menu = CCMenu::create();
        menu->setID("autodecorate-menu"_spr);
        auto win = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({70.f, win.height - 70.f});
        menu->addChild(btn);
        ui->addChild(menu, 100);
        return;
    }

    auto btn = makeButton(copy, target, callback);
    auto menu = copy->getParent();

    if (menu->getLayout()) {
        // The menu positions its own children: slot the new button in right after Copy.
        std::vector<CCNode*> order;
        if (auto kids = menu->getChildren())
            for (auto* c : CCArrayExt<CCNode*>(kids)) order.push_back(c);
        int copyIndex = static_cast<int>(std::find(order.begin(), order.end(), copy) - order.begin());
        for (size_t i = 0; i < order.size(); ++i) order[i]->setZOrder(static_cast<int>(i) * 2);
        menu->addChild(btn, copyIndex * 2 + 1);
        menu->sortAllChildren();
        menu->updateLayout();
        return;
    }

    // Free-positioned menu: put the button on whichever side of Copy has room.
    menu->addChild(btn, copy->getZOrder());
    auto cr = copy->boundingBox();
    auto bs = btn->boundingBox().size;
    const float gap = 4.f;
    CCPoint candidates[4] = {
        {cr.getMaxX() + gap + bs.width / 2.f, cr.getMidY()},
        {cr.getMinX() - gap - bs.width / 2.f, cr.getMidY()},
        {cr.getMidX(), cr.getMinY() - gap - bs.height / 2.f},
        {cr.getMidX(), cr.getMaxY() + gap + bs.height / 2.f},
    };
    CCPoint chosen = candidates[0];
    for (auto const& pos : candidates) {
        CCRect r(pos.x - bs.width / 2.f, pos.y - bs.height / 2.f, bs.width, bs.height);
        bool clash = false;
        for (auto* c : CCArrayExt<CCNode*>(menu->getChildren())) {
            if (c == btn || c == copy || !typeinfo_cast<CCMenuItem*>(c)) continue;
            if (r.intersectsRect(c->boundingBox())) { clash = true; break; }
        }
        if (!clash) { chosen = pos; break; }
    }
    btn->setPosition(chosen);
}

}  // namespace autodeco
