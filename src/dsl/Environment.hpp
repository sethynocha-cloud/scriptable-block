#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>

namespace scriptable {

using Value = std::variant<double, bool>;

// A special handle type for obj() and color() references
struct ObjectHandle { int groupID; };
struct ColorHandle { int channelID; };
using ExtValue = std::variant<double, bool, ObjectHandle, ColorHandle>;

class Environment {
    std::unordered_map<std::string, Value> m_variables;

public:
    void set(const std::string& name, Value val) {
        m_variables[name] = val;
    }

    std::optional<Value> get(const std::string& name) const {
        auto it = m_variables.find(name);
        if (it != m_variables.end()) return it->second;
        return std::nullopt;
    }

    bool has(const std::string& name) const {
        return m_variables.contains(name);
    }

    void clear() {
        m_variables.clear();
    }
};

inline double toNumber(const Value& v) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    if (auto* b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
    return 0.0;
}

inline bool toBool(const Value& v) {
    if (auto* b = std::get_if<bool>(&v)) return *b;
    if (auto* d = std::get_if<double>(&v)) return *d != 0.0;
    return false;
}

} // namespace scriptable
