#pragma once
#include <Geode/Geode.hpp>

namespace autodeco {
// Adds the Autodecorate button directly beside the editor's Copy button,
// copying the Copy button's own look so it reads as a native button.
void attachButton(EditorUI* ui, cocos2d::CCObject* target, cocos2d::SEL_MenuHandler callback);
}  // namespace autodeco
