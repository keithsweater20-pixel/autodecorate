#include "Apply.hpp"
#include <Geode/ui/Notification.hpp>
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

class DecoJob : public CCNode {
public:
    static DecoJob* create(Plan plan, Settings st, RGB pri, RGB sec, V2 mn, V2 mx) {
        auto ret = new DecoJob();
        ret->m_plan = std::move(plan);
        ret->m_st = st;
        ret->m_pri = pri;
        ret->m_sec = sec;
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
                makeTrigger(lel, m_st.primaryChannel, m_pri, x, y);
                makeTrigger(lel, m_st.secondaryChannel, m_sec, x, y + 30.f);
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
            compat::sendBehind(obj);
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
    RGB m_pri, m_sec;
    V2 m_mn, m_mx;
    size_t m_next = 0;
    bool m_started = false;
    Ref<CCArray> m_created;
    CCLabelBMFont* m_label = nullptr;

    void makeTrigger(LevelEditorLayer* lel, int channel, RGB c, float x, float y) {
        auto t = lel->createObject(compat::kColorTriggerKey, CCPoint(x, y), true);
        if (!t) return;
        if (!compat::configureColorTrigger(t, channel, ccColor3B{c.r, c.g, c.b})) {
            lel->removeObject(t, true);
            return;
        }
        m_created->addObject(t);
    }

    void finish(LevelEditorLayer* lel) {
        this->unscheduleUpdate();
        if (m_created->count() > 0) {
            compat::registerUndo(lel, m_created);
            if (auto ui = EditorUI::get()) ui->updateButtons();
        }
        toast(fmt::format("Autodecorate added {} objects. Undo removes them.", m_plan.items.size()).c_str(),
              NotificationIcon::Success);
        this->removeFromParent();
    }
};

}  // namespace

void runAutodecorate(Settings const& st) {
    auto lel = LevelEditorLayer::get();
    auto ui = EditorUI::get();
    if (!lel || !ui) return;

    std::unordered_set<GameObject*> selected;
    if (st.selectedOnly) {
        if (auto arr = ui->getSelectedObjects())
            for (auto* o : CCArrayExt<GameObject*>(arr)) selected.insert(o);
        if (selected.empty()) {
            toast("Select some layout objects first, or turn off 'Selected only'.", NotificationIcon::Warning);
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
        toast("Autodecorate: no layout blocks, spikes or slopes found.", NotificationIcon::Warning);
        return;
    }

    ScanResult scan;
    buildScan(scan, descs);
    Plan plan = makePlan(scan, st, std::random_device{}(), queryDecoPool);
    if (plan.items.empty()) {
        auto const& ps = poolStats();
        std::string msg = plan.note.empty() ? std::string("Autodecorate: nothing to add.") : plan.note;
        msg += fmt::format(" [keys {} / noframe {} / noobj {} / notdeco {} / small {} / ok {} / size {}]", ps.keys, ps.noFrame, ps.noObject, ps.notDeco, ps.tooSmall, ps.verified, ps.sizeRej);
        toast(msg.c_str(), NotificationIcon::Warning);
        return;
    }

    StyleDef const& sd = styleDef(st.style);
    RGB pri = st.primaryColor >= 0 ? swatchColor(st.primaryColor) : sd.primary;
    RGB sec = st.secondaryColor >= 0 ? swatchColor(st.secondaryColor) : sd.secondary;
    if (auto job = DecoJob::create(std::move(plan), st, pri, sec, scan.mn, scan.mx)) ui->addChild(job, 1000);
}

}  // namespace autodeco
