#pragma once
#include "AST.hpp"
#include "Environment.hpp"
#include <functional>
#include <string>
#include <vector>

namespace scriptable {

enum class ScriptState {
    Running,
    Waiting,
    Finished,
    Error
};

// Callback interface for game operations
struct GameCallbacks {
    // Object operations (group ID based)
    std::function<void(int, float, float)> moveObject;       // groupID, dx, dy
    std::function<void(int, float, float)> moveObjectTo;     // groupID, x, y
    std::function<void(int, float)> rotateObject;            // groupID, degrees
    std::function<void(int, float)> scaleObject;             // groupID, scale
    std::function<void(int, float, float)> scaleObjectXY;    // groupID, sx, sy
    std::function<void(int, bool)> toggleObject;             // groupID, visible
    std::function<void(int, float)> setObjectAlpha;          // groupID, alpha
    std::function<void(int, int, int, int)> setObjectColor;  // groupID, r, g, b

    // Player operations
    std::function<void(float)> setPlayerSpeed;
    std::function<void(bool)> setPlayerGravity;
    std::function<void(int, int, int, float)> pulsePlayer;   // r, g, b, duration
    std::function<void(bool)> enableTrail;
    std::function<void()> killPlayer;
    std::function<void()> forceJump;

    // Camera operations
    std::function<void(float, float)> moveCamera;
    std::function<void(float)> zoomCamera;
    std::function<void(float, float)> shakeCamera;           // strength, duration
    std::function<void(float)> rotateCamera;

    // Color channel operations
    std::function<void(int, int, int, int)> setColorChannel;     // channel, r, g, b
    std::function<void(int, int, int, int, float)> pulseColorChannel; // channel, r, g, b, dur

    // Read-only player state
    std::function<float()> getPlayerX;
    std::function<float()> getPlayerY;
    std::function<float()> getPlayerXVel;
    std::function<float()> getPlayerYVel;
    std::function<float()> getLevelTime;
    std::function<int()> getAttempt;
};

class Interpreter {
public:
    static constexpr int MAX_INSTRUCTIONS_PER_FRAME = 10000;
    static constexpr int MAX_LOOP_ITERATIONS = 1000;
    static constexpr int MAX_STACK_DEPTH = 64;

private:
    // Execution state for resumable execution
    struct Frame {
        const std::vector<ASTNodePtr>* statements;
        size_t index;
        // For loops
        int loopIteration = 0;
        int loopMax = 0;
    };

    ASTNodePtr m_program;
    Environment m_env;
    GameCallbacks* m_callbacks = nullptr;
    ScriptState m_state = ScriptState::Finished;
    float m_waitTimer = 0.0f;
    int m_instructionCount = 0;
    std::string m_errorMessage;

    std::vector<Frame> m_callStack;
    bool m_waitRequested = false;
    float m_waitDuration = 0.0f;

    // Interpreter core
    ExtValue evaluate(const ASTNode& node);
    void execute(const ASTNode& node);
    void executeBlock(const Block& block);

    // Helpers
    ExtValue callBuiltinFunction(const std::string& name, const std::vector<ExtValue>& args, int line);
    ExtValue callMethod(const ExtValue& object, const std::string& method,
                        const std::vector<ExtValue>& args, int line);
    ExtValue getMember(const ExtValue& object, const std::string& member, int line);
    void countInstruction();

    double requireNumber(const ExtValue& v, const std::string& context, int line);
    int requireInt(const ExtValue& v, const std::string& context, int line);

public:
    Interpreter();

    void load(ASTNodePtr program);
    void setCallbacks(GameCallbacks* cb);
    void start();
    void reset();

    // Returns true if script needs more frames (waiting or still running)
    bool step(float dt);

    ScriptState state() const { return m_state; }
    const std::string& errorMessage() const { return m_errorMessage; }
};

} // namespace scriptable
