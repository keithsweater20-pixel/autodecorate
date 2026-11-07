#include "Apply.hpp"
#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_set>
#include "../core/Plan.hpp"
#include "Compat.hpp"
#include "Pool.hpp"

using namespace geode::prelude;

namespace autodeco {

namespace {

float snapUnits(float v) { return std::max(30.f, std::round(v / 30.f) * 30.f); }

// Reads a GameObject and decides whether it is one of the four layout types, purely from
// its collision type and shape. No object IDs involved.
bool describe(GameObject* o, ObjDesc& d) {
    if (!o || o->m_isTrigger) return false;
    LayoutKind kind;
    switch (o->m_objectType) {
        case GameObjectType::Slope:
            kind = LayoutKind::Slope;
            break;
        case GameObjectType::Solid:
            if (o->m_isPassable || o->m_isNoTouch) return false;
            kind = LayoutKind::Block;
            break;
        case GameObjectType::Hazard:
            if (o->m_objectRadius > 0.f) return false;  // saws and other round hazards are not spikes
            kind = LayoutKind::Spike;
            break;
        default:
            return false;
    }
    auto cs = o->getContentSize();
    float w = snapUnits(cs.width * std::abs(o->getScaleX()));
    float h = snapUnits(cs.height * std::abs(o->getScaleY()));
    if (w > 150.f || h > 150.f) return false;  // too big to be a layout piece
    if (kind == LayoutKind::Slope && std::max(w, h) / std::min(w, h) >= 1.8f) kind = LayoutKind::LongSlope;
    d.kind = kind;
    d.pos = V2(o->getPositionX(), o->getPositionY());
    d.w = w;
    d.h = h;
    d.rotation = o->getRotation();
    d.flipX = o->isFlipX();
    d.flipY = o->isFlipY();
    return true;
}

void toast(char const* text, NotificationIcon icon) { Notification::create(text, icon)->show(); }

Ref<UndoObject> g_lastUndo;      // the editor undo step made by the last generation
Ref<CCDrawNode> g_preview;       // the on-screen preview dots, if shown

void clearPreviewNode() {
    if (g_preview) {
        g_preview->removeFromParent();
        g_preview = nullptr;
    }
}

// True when the editor's newest undo step is the one this mod made.
bool lastUndoIsOurs(LevelEditorLayer* lel) {
    if (!g_lastUndo || !lel || !lel->m_undoObjects || lel->m_undoObjects->count() == 0) return false;
    return lel->m_undoObjects->lastObject() == static_cast<CCObject*>(g_lastUndo.data());
}

struct Prepared {
    ScanResult scan;
    Plan plan;
    std::string error;  // non-empty when nothing can be done
    bool failedPool = false;
};

// Reads the level, plans the decoration. Used by both Generate and Preview.
void prepare(Settings const& st, Prepared& out) {
    auto lel = LevelEditorLayer::get();
    auto ui = EditorUI::get();
    if (!lel || !ui) {
        out.error = "Open the level editor first.";
        return;
    }
    std::unordered_set<GameObject*> selected;
    if (st.selectedOnly) {
        if (auto arr = ui->getSelectedObjects())
            for (auto* o : CCArrayExt<GameObject*>(arr)) selected.insert(o);
        if (selected.empty()) {
            out.error = "Select some layout objects first, or turn off 'Selected objects only'.";
            return;
        }
    }
    std::vector<ObjDesc> descs;
    if (lel->m_objects) {
        for (auto* o : CCArrayExt<GameObject*>(lel->m_objects)) {
            ObjDesc d;
            if (!describe(o, d)) continue;
            d.selected = !st.selectedOnly || selected.count(o) > 0;
            descs.push_back(d);
        }
    }
    if (descs.empty()) {
        out.error = "Autodecorate: no layout blocks, spikes or slopes found.";
        return;
    }
    buildScan(out.scan, descs);

    // Decorations already near the layout count as taken, so nothing is stacked twice.
    if (lel->m_objects) {
        for (auto* o : CCArrayExt<GameObject*>(lel->m_objects)) {
            if (!o || !compat::isPlainDecoration(o)) continue;
            float x = o->getPositionX(), y = o->getPositionY();
            if (x < out.scan.mn.x - 300.f || x > out.scan.mx.x + 300.f || y < out.scan.mn.y - 300.f || y > out.scan.mx.y + 400.f) continue;
            auto cs = o->getContentSize();
            float size = std::max(cs.width * std::abs(o->getScaleX()), cs.height * std::abs(o->getScaleY()));
            out.scan.existing.push_back({V2(x, y), size});
        }
    }

    out.plan = makePlan(out.scan, st, st.seed, queryDecoPool);
    if (out.plan.items.empty()) {
        auto const& ps = poolStats();
        out.error = out.plan.note.empty() ? std::string("Autodecorate: nothing to add.") : out.plan.note;
        out.error += fmt::format(" [keys {} / noframe {} / noobj {} / notdeco {} / small {} / ok {} / size {}]", ps.keys, ps.noFrame,
                                 ps.noObject, ps.notDeco, ps.tooSmall, ps.verified, ps.sizeRej);
    }
}

class DecoJob : public CCNode {
public:
    static DecoJob* create(Plan plan, Settings st, RGB pri, RGB sec, RGB bg, V2 mn, V2 mx) {
        auto ret = new DecoJob();
        ret->m_plan = std::move(plan);
        ret->m_st = st;
        ret->m_pri = pri;
        ret->m_sec = sec;
        ret->m_bg = bg;
        ret->m_mn = mn;
        ret->m_mx = mx;
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCNode::init()) return false;
        m_created = CCArray::create();
        auto win = CCDirector::sharedDirector()->getWinSize();
        m_label = CCLabelBMFont::create("Autodecorating...", "bigFont.fnt");
        m_label->setScale(.5f);
        m_label->setPosition({win.width / 2.f, win.height - 28.f});
        this->addChild(m_label);
        this->scheduleUpdate();
        return true;
    }

    void update(float) override {
        auto lel = LevelEditorLayer::get();
        if (!lel) {
            this->removeFromParent();
            return;
        }
        if (!m_started) {
            m_started = true;
            if (m_st.makeColorTriggers) {
                float x = m_mn.x - 60.f, y = m_mx.y + 150.f;
                // Lighting scales how strongly the detail colour shows (glow strength).
                float glow = 0.2f + 0.8f * std::clamp(m_st.lighting / 100.f, 0.f, 1.f);
                makeTrigger(lel, m_st.primaryChannel, m_pri, x, y, 1.f);
                makeTrigger(lel, m_st.secondaryChannel, m_sec, x, y + 30.f, glow);
                if (m_st.backgroundColor != -2) makeTrigger(lel, compat::kBackgroundChannel, m_bg, x, y + 60.f, 1.f);
            }
        }
        size_t end = std::min(m_next + kBatch, m_plan.items.size());
        for (; m_next < end; ++m_next) {
            auto const& p = m_plan.items[m_next];
            auto obj = lel->createObject(p.id, CCPoint(p.pos.x, p.pos.y), true);
            if (!obj) continue;
            if (!compat::isPlainDecoration(obj)) {
                // Safety net: if anything that is not a plain decoration slipped through, remove it.
                lel->removeObject(obj, true);
                continue;
            }
            obj->setRotation(p.rotation);
            if (std::abs(p.scale - 1.f) > 0.01f) {
                obj->updateCustomScaleX(p.scale);
                obj->updateCustomScaleY(p.scale);
            }
            if (p.flipX) obj->setFlipX(true);
            compat::setChannels(obj, p.secondary ? m_st.secondaryChannel : m_st.primaryChannel,
                                p.secondary ? m_st.primaryChannel : m_st.secondaryChannel);
            compat::sendBehind(obj, p.layer);
            m_created->addObject(obj);
        }
        int pct = m_plan.items.empty() ? 100 : static_cast<int>(100.f * m_next / m_plan.items.size());
        m_label->setString(fmt::format("Autodecorating... {}%", pct).c_str());
        if (m_next >= m_plan.items.size()) finish(lel);
    }

private:
    static constexpr size_t kBatch = 250;
    Plan m_plan;
    Settings m_st;
    RGB m_pri, m_sec, m_bg;
    V2 m_mn, m_mx;
    size_t m_next = 0;
    bool m_started = false;
    Ref<CCArray> m_created;
    CCLabelBMFont* m_label = nullptr;

    void makeTrigger(LevelEditorLayer* lel, int channel, RGB c, float x, float y, float opacity) {
        auto t = lel->createObject(compat::kColorTriggerKey, CCPoint(x, y), true);
        if (!t) return;
        if (!compat::configureColorTrigger(t, channel, ccColor3B{c.r, c.g, c.b}, opacity)) {
            lel->removeObject(t, true);
            return;
        }
        m_created->addObject(t);
    }

    void finish(LevelEditorLayer* lel) {
        this->unscheduleUpdate();
        if (m_created->count() > 0) {
            g_lastUndo = compat::registerUndo(lel, m_created);
            if (auto ui = EditorUI::get()) ui->updateButtons();
        }
        toast(fmt::format("Autodecorate added {} objects ({} background). Undo removes them.", m_plan.items.size(),
                          m_plan.backgroundCount).c_str(),
              NotificationIcon::Success);
        this->removeFromParent();
    }
};

}  // namespace

void runAutodecorate(Settings const& st) {
    clearPreviewNode();
    auto ui = EditorUI::get();
    Prepared prep;
    prepare(st, prep);
    if (!prep.error.empty()) {
        toast(prep.error.c_str(), NotificationIcon::Warning);
        return;
    }
    StyleDef const& sd = styleDef(st.style, st.variant);
    RGB pri = st.primaryColor >= 0 ? swatchColor(st.primaryColor) : sd.primary;
    RGB sec = st.secondaryColor >= 0 ? swatchColor(st.secondaryColor) : sd.secondary;
    RGB bg = st.backgroundColor >= 0 ? swatchColor(st.backgroundColor) : sd.background;
    if (auto job = DecoJob::create(std::move(prep.plan), st, pri, sec, bg, prep.scan.mn, prep.scan.mx)) ui->addChild(job, 1000);
}

void previewAutodecorate(Settings const& st) {
    bool wasShown = g_preview && g_preview->getParent();
    clearPreviewNode();
    if (wasShown) {
        toast("Preview cleared.", NotificationIcon::Info);
        return;
    }
    auto lel = LevelEditorLayer::get();
    Prepared prep;
    prepare(st, prep);
    if (!prep.error.empty()) {
        toast(prep.error.c_str(), NotificationIcon::Warning);
        return;
    }
    StyleDef const& sd = styleDef(st.style, st.variant);
    RGB pri = st.primaryColor >= 0 ? swatchColor(st.primaryColor) : sd.primary;
    RGB sec = st.secondaryColor >= 0 ? swatchColor(st.secondaryColor) : sd.secondary;
    auto node = CCDrawNode::create();
    size_t drawn = 0;
    for (auto const& it : prep.plan.items) {
        if (drawn++ > 6000) break;
        RGB c = it.secondary ? sec : pri;
        float r = std::clamp(it.size * .3f, 3.f, 30.f);
        node->drawDot(CCPoint(it.pos.x, it.pos.y), r,
                      ccColor4F{c.r / 255.f, c.g / 255.f, c.b / 255.f, it.background ? .35f : .7f});
    }
    lel->m_objectLayer->addChild(node, 9999);
    g_preview = node;
    toast(fmt::format("Preview: {} decorations ({} background). Tap Preview again to hide it. Nothing was added.",
                      prep.plan.items.size(), prep.plan.backgroundCount).c_str(),
          NotificationIcon::Info);
}

bool undoAutodecorate() {
    clearPreviewNode();
    auto lel = LevelEditorLayer::get();
    auto ui = EditorUI::get();
    if (!lel || !ui) return false;
    if (!lastUndoIsOurs(lel)) {
        toast(g_lastUndo ? "Something else was changed since. Use the editor's own Undo button."
                         : "Nothing to undo from Autodecorate.",
              NotificationIcon::Warning);
        return false;
    }
    g_lastUndo = nullptr;
    ui->undoLastAction(nullptr);
    toast("Autodecorate removed.", NotificationIcon::Success);
    return true;
}

void regenerateAutodecorate(Settings const& st) {
    auto lel = LevelEditorLayer::get();
    if (g_lastUndo) {
        if (!lastUndoIsOurs(lel)) {
            toast("Something else was changed since the last Autodecorate. Undo that first, then Regenerate.",
                  NotificationIcon::Warning);
            return;
        }
        g_lastUndo = nullptr;
        if (auto ui = EditorUI::get()) ui->undoLastAction(nullptr);
    }
    runAutodecorate(st);
}

}  // namespace autodeco
