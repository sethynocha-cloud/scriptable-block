#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace scriptable {

enum class TriggerMode : int {
    OnLevelStart = 0,
    Continuous = 1,
    OnClick = 2
};

inline const char* triggerModeName(TriggerMode mode) {
    switch (mode) {
        case TriggerMode::OnLevelStart: return "On Level Start";
        case TriggerMode::Continuous: return "Continuous";
        case TriggerMode::OnClick: return "On Click";
    }
    return "Unknown";
}

inline TriggerMode nextTriggerMode(TriggerMode mode) {
    int next = (static_cast<int>(mode) + 1) % 3;
    return static_cast<TriggerMode>(next);
}

struct ScriptEntry {
    std::string name = "Script 1";
    std::string scriptCode;
    TriggerMode triggerMode = TriggerMode::OnLevelStart;
    bool enabled = true;
};

// Per-level script storage using Geode saved values
class LevelScriptStore {
    static inline std::vector<ScriptEntry> s_scripts;
    static inline int s_currentLevelID = 0;

public:
    static std::string getLevelKey(int levelID) {
        return fmt::format("level-scripts-{}", levelID);
    }

    static void loadForLevel(int levelID) {
        s_currentLevelID = levelID;
        s_scripts.clear();

        auto key = getLevelKey(levelID);
        auto saved = Mod::get()->getSavedValue<std::string>(key, "");
        if (saved.empty()) return;

        // Format: name\x01code\x01triggerMode\x01enabled\x02name\x01code\x01...
        std::string entry;
        std::istringstream stream(saved);
        while (std::getline(stream, entry, '\x02')) {
            if (entry.empty()) continue;
            ScriptEntry se;
            std::istringstream es(entry);
            std::string part;
            int idx = 0;
            while (std::getline(es, part, '\x01')) {
                switch (idx) {
                    case 0: se.name = part; break;
                    case 1: se.scriptCode = part; break;
                    case 2: try { se.triggerMode = static_cast<TriggerMode>(std::stoi(part)); } catch (...) {} break;
                    case 3: se.enabled = (part == "1"); break;
                }
                idx++;
            }
            s_scripts.push_back(se);
        }
    }

    static void save() {
        std::string data;
        for (size_t i = 0; i < s_scripts.size(); i++) {
            auto& s = s_scripts[i];
            if (i > 0) data += '\x02';
            data += s.name + '\x01' + s.scriptCode + '\x01'
                  + std::to_string(static_cast<int>(s.triggerMode)) + '\x01'
                  + (s.enabled ? "1" : "0");
        }
        Mod::get()->setSavedValue(getLevelKey(s_currentLevelID), data);
    }

    static std::vector<ScriptEntry>& scripts() { return s_scripts; }

    static ScriptEntry& addScript() {
        ScriptEntry entry;
        entry.name = fmt::format("Script {}", s_scripts.size() + 1);
        s_scripts.push_back(entry);
        save();
        return s_scripts.back();
    }

    static void removeScript(size_t index) {
        if (index < s_scripts.size()) {
            s_scripts.erase(s_scripts.begin() + index);
            save();
        }
    }

    static void clear() {
        s_scripts.clear();
        s_currentLevelID = 0;
    }
};

} // namespace scriptable
