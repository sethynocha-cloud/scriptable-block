#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

using namespace geode::prelude;

// All hooks are auto-registered via $modify in their respective files:
// - src/objects/ScriptableObject.cpp    (EffectGameObject hooks for data serialization)
// - src/hooks/PlayLayerHooks.cpp        (PlayLayer, GJBaseGameLayer, UILayer hooks)
// - src/hooks/EditorUIHooks.cpp         (EditorUI hooks for editor integration)
// - src/hooks/LevelEditorLayerHooks.cpp (LevelEditorLayer hooks)

$on_mod(Loaded) {
    geode::log::info("Scriptable Block mod loaded!");
    geode::log::info("DSL scripting is available in the level editor.");
    geode::log::info("Place a Scriptable Block and click Edit to write code.");
}
