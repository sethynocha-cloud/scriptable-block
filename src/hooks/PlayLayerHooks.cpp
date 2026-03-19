#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include "../runtime/ScriptRuntime.hpp"
#include "../objects/ScriptableObjectData.hpp"

using namespace geode::prelude;

class $modify(ScriptPlayLayer, PlayLayer) {
    struct Fields {
        std::unique_ptr<scriptable::ScriptRuntime> runtime;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Load scripts for this level
        if (level) {
            scriptable::LevelScriptStore::loadForLevel(level->m_levelID);
        }

        m_fields->runtime = std::make_unique<scriptable::ScriptRuntime>();
        m_fields->runtime->init(this);

        return true;
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (!m_fields->runtime) return;

        // Register all level scripts
        auto& scripts = scriptable::LevelScriptStore::scripts();
        for (auto& entry : scripts) {
            if (entry.enabled && !entry.scriptCode.empty()) {
                m_fields->runtime->registerScript(&entry);
            }
        }

        m_fields->runtime->onLevelStart();
    }

    void update(float dt) {
        PlayLayer::update(dt);
        if (m_fields->runtime) {
            m_fields->runtime->update(dt);
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->runtime) {
            m_fields->runtime->reset();
        }
    }

    void onQuit() {
        if (m_fields->runtime) {
            m_fields->runtime->cleanup();
        }
        PlayLayer::onQuit();
    }
};

class $modify(ScriptUILayer, UILayer) {
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        bool result = UILayer::ccTouchBegan(touch, event);

        auto* runtime = scriptable::ScriptRuntime::get();
        if (runtime) {
            runtime->onPlayerClick();
        }

        return result;
    }
};
