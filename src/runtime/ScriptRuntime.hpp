#pragma once
#include <Geode/Geode.hpp>
#include "../dsl/Interpreter.hpp"
#include "../dsl/Lexer.hpp"
#include "../dsl/Parser.hpp"
#include "../objects/ScriptableObjectData.hpp"
#include "GameAPI.hpp"
#include <vector>
#include <memory>

using namespace geode::prelude;

namespace scriptable {

class ScriptRuntime {
public:
    struct ActiveScript {
        std::unique_ptr<Interpreter> interpreter;
        ScriptEntry* sourceEntry = nullptr;
        TriggerMode triggerMode;
        bool hasTriggered = false;
        bool parsed = false;
    };

private:
    std::vector<ActiveScript> m_scripts;
    PlayLayer* m_playLayer = nullptr;
    GameAPI m_gameAPI;
    float m_levelTime = 0.0f;

    void compileScript(ActiveScript& script);

public:
    void init(PlayLayer* layer);
    void registerScript(ScriptEntry* entry);
    void onLevelStart();
    void update(float dt);
    void onPlayerClick();
    void reset();
    void cleanup();

    static ScriptRuntime* get();
    static void set(ScriptRuntime* runtime);
};

} // namespace scriptable
