#include "ScriptRuntime.hpp"
#include "../dsl/DSLError.hpp"

namespace scriptable {

static ScriptRuntime* s_instance = nullptr;

ScriptRuntime* ScriptRuntime::get() {
    return s_instance;
}

void ScriptRuntime::set(ScriptRuntime* runtime) {
    s_instance = runtime;
}

void ScriptRuntime::init(PlayLayer* layer) {
    m_playLayer = layer;
    m_gameAPI.init(layer);
    m_levelTime = 0.0f;
    m_scripts.clear();
    set(this);
}

void ScriptRuntime::registerScript(ScriptEntry* entry) {
    if (!entry || entry->scriptCode.empty()) return;

    ActiveScript script;
    script.sourceEntry = entry;
    script.triggerMode = entry->triggerMode;
    script.interpreter = std::make_unique<Interpreter>();
    script.interpreter->setCallbacks(m_gameAPI.getCallbacks());

    compileScript(script);
    m_scripts.push_back(std::move(script));
}

void ScriptRuntime::compileScript(ActiveScript& script) {
    try {
        Lexer lexer(script.sourceEntry->scriptCode);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        script.interpreter->load(std::move(ast));
        script.parsed = true;
    } catch (const DSLError& e) {
        geode::log::error("Script parse error: {}", e.what());
        script.parsed = false;
    }
}

void ScriptRuntime::onLevelStart() {
    for (auto& script : m_scripts) {
        if (!script.parsed || !script.sourceEntry->enabled) continue;

        if (script.triggerMode == TriggerMode::OnLevelStart) {
            script.interpreter->start();
            script.hasTriggered = true;
        }
        if (script.triggerMode == TriggerMode::Continuous) {
            script.interpreter->start();
            script.hasTriggered = true;
        }
    }
}

void ScriptRuntime::update(float dt) {
    m_levelTime += dt;
    m_gameAPI.update(dt);

    for (auto& script : m_scripts) {
        if (!script.parsed || !script.sourceEntry->enabled) continue;

        auto state = script.interpreter->state();

        if (state == ScriptState::Running || state == ScriptState::Waiting) {
            script.interpreter->step(dt);
        }

        // For continuous scripts, restart when finished
        if (script.triggerMode == TriggerMode::Continuous
            && script.interpreter->state() == ScriptState::Finished
            && script.hasTriggered) {
            script.interpreter->start();
        }
    }
}

void ScriptRuntime::onPlayerClick() {
    for (auto& script : m_scripts) {
        if (!script.parsed || !script.sourceEntry->enabled) continue;
        if (script.triggerMode != TriggerMode::OnClick) continue;

        script.interpreter->start();
    }
}

void ScriptRuntime::reset() {
    for (auto& script : m_scripts) {
        script.interpreter->reset();
        script.hasTriggered = false;
    }
    m_levelTime = 0.0f;

    // Re-trigger level start and continuous scripts
    onLevelStart();
}

void ScriptRuntime::cleanup() {
    m_scripts.clear();
    m_playLayer = nullptr;
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

} // namespace scriptable
