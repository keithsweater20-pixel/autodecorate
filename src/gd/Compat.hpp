#pragma once
// Every Geometry Dash binding call whose exact name or signature could differ between
// Geode versions lives in this one file, so a build error is a one-line fix here.
#include <Geode/Geode.hpp>
#include <string>

namespace autodeco::compat {
using namespace geode::prelude;

constexpr int kColorTriggerKey = 899;  // the editor's Color trigger
constexpr int kBackgroundChannel = 1000;  // the level's background colour channel

// Texture name an object key uses ("" when the key is unused).
inline std::string frameForKey(int key) {
    auto toolbox = ObjectToolbox::sharedState();
    if (!toolbox) return {};
    char const* name = toolbox->intKeyToFrame(key);
    return name ? std::string(name) : std::string();
}

// True for objects that are purely visual: no collision, no trigger behaviour.
inline bool isPlainDecoration(GameObject* o) {
    return o && o->m_objectType == GameObjectType::Decoration && !o->m_isTrigger;
}

// Colour channels for a decoration's base and detail layers.
inline void setChannels(GameObject* o, int baseChannel, int detailChannel) {
    if (o->m_baseColor) o->m_baseColor->m_colorID = baseChannel;
    if (o->m_detailColor) o->m_detailColor->m_colorID = detailChannel;
    o->updateObjectEditorColor();
}

// Draw behind the layout. layer: 0 = back, 1 = middle, 2 = front (still behind the layout).
inline void sendBehind(GameObject* o, int layer = 1) {
    o->m_zLayer = layer <= 0 ? ZLayer::B4 : (layer == 1 ? ZLayer::B3 : ZLayer::B2);
    o->m_zOrder = layer <= 0 ? -5 : (layer == 1 ? -3 : -1);
}

// Makes a Color trigger set `channel` to `color` instantly.
inline bool configureColorTrigger(GameObject* o, int channel, ccColor3B color, float opacity = 1.f) {
    auto t = typeinfo_cast<EffectGameObject*>(o);
    if (!t) return false;
    t->m_targetColor = channel;
    t->m_triggerTargetColor = color;
    t->m_duration = 0.f;
    t->m_opacity = opacity;
    return true;
}

// One undo step that removes everything in `objects`.
inline UndoObject* registerUndo(LevelEditorLayer* lel, CCArray* objects) {
    auto undo = UndoObject::createWithArray(objects, UndoCommand::Paste);
    lel->addToUndoList(undo, false);
    return undo;
}

}  // namespace autodeco::compat
