#include "Interpreter.hpp"
#include "DSLError.hpp"
#include <cmath>
#include <fmt/format.h>

namespace scriptable {

Interpreter::Interpreter() {}

void Interpreter::load(ASTNodePtr program) {
    m_program = std::move(program);
    m_state = ScriptState::Finished;
}

void Interpreter::setCallbacks(GameCallbacks* cb) {
    m_callbacks = cb;
}

void Interpreter::start() {
    m_env.clear();
    m_callStack.clear();
    m_instructionCount = 0;
    m_waitTimer = 0.0f;
    m_waitRequested = false;
    m_errorMessage.clear();
    m_state = ScriptState::Running;
}

void Interpreter::reset() {
    m_state = ScriptState::Finished;
    m_env.clear();
    m_callStack.clear();
}

bool Interpreter::step(float dt) {
    if (m_state == ScriptState::Finished || m_state == ScriptState::Error) {
        return false;
    }

    if (m_state == ScriptState::Waiting) {
        m_waitTimer -= dt;
        if (m_waitTimer > 0.0f) return true;
        m_state = ScriptState::Running;
    }

    m_instructionCount = 0;
    m_waitRequested = false;

    try {
        if (m_program) {
            execute(*m_program);
        }
        if (!m_waitRequested) {
            m_state = ScriptState::Finished;
        }
    } catch (const InstructionLimitError&) {
        m_errorMessage = "Script exceeded instruction limit";
        m_state = ScriptState::Error;
        return false;
    } catch (const RuntimeError& e) {
        m_errorMessage = e.what();
        m_state = ScriptState::Error;
        return false;
    } catch (const std::exception& e) {
        m_errorMessage = fmt::format("Internal error: {}", e.what());
        m_state = ScriptState::Error;
        return false;
    }

    return m_state != ScriptState::Finished && m_state != ScriptState::Error;
}

void Interpreter::countInstruction() {
    if (++m_instructionCount > MAX_INSTRUCTIONS_PER_FRAME) {
        throw InstructionLimitError();
    }
}

double Interpreter::requireNumber(const ExtValue& v, const std::string& context, int line) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    if (auto* b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;
    throw RuntimeError(fmt::format("{}: expected number", context), line, 0);
}

int Interpreter::requireInt(const ExtValue& v, const std::string& context, int line) {
    return static_cast<int>(requireNumber(v, context, line));
}

// --- Execute statements ---

void Interpreter::execute(const ASTNode& node) {
    if (m_waitRequested) return;
    countInstruction();

    std::visit([this, &node](const auto& n) {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, Block>) {
            executeBlock(n);
        } else if constexpr (std::is_same_v<T, LetStatement>) {
            auto val = evaluate(*n.initializer);
            Value v;
            if (auto* d = std::get_if<double>(&val)) v = *d;
            else if (auto* b = std::get_if<bool>(&val)) v = *b;
            else v = 0.0;
            m_env.set(n.name, v);
        } else if constexpr (std::is_same_v<T, AssignStatement>) {
            auto val = evaluate(*n.value);
            Value v;
            if (auto* d = std::get_if<double>(&val)) v = *d;
            else if (auto* b = std::get_if<bool>(&val)) v = *b;
            else v = 0.0;
            m_env.set(n.name, v);
        } else if constexpr (std::is_same_v<T, IfStatement>) {
            auto cond = evaluate(*n.condition);
            bool truth = false;
            if (auto* b = std::get_if<bool>(&cond)) truth = *b;
            else if (auto* d = std::get_if<double>(&cond)) truth = (*d != 0.0);
            else truth = true;

            if (truth) {
                execute(*n.thenBranch);
            } else if (n.elseBranch) {
                execute(*n.elseBranch);
            }
        } else if constexpr (std::is_same_v<T, WhileStatement>) {
            int iters = 0;
            while (true) {
                if (m_waitRequested) return;
                if (++iters > MAX_LOOP_ITERATIONS) {
                    throw RuntimeError("While loop exceeded max iterations", n.line, 0);
                }
                auto cond = evaluate(*n.condition);
                bool truth = false;
                if (auto* b = std::get_if<bool>(&cond)) truth = *b;
                else if (auto* d = std::get_if<double>(&cond)) truth = (*d != 0.0);

                if (!truth) break;
                execute(*n.body);
            }
        } else if constexpr (std::is_same_v<T, RepeatStatement>) {
            auto countVal = evaluate(*n.count);
            int count = requireInt(countVal, "repeat count", n.line);
            if (count > MAX_LOOP_ITERATIONS) count = MAX_LOOP_ITERATIONS;
            for (int i = 0; i < count && !m_waitRequested; i++) {
                execute(*n.body);
            }
        } else if constexpr (std::is_same_v<T, WaitStatement>) {
            auto dur = evaluate(*n.duration);
            double secs = requireNumber(dur, "wait duration", n.line);
            m_waitTimer = static_cast<float>(secs);
            m_waitRequested = true;
            m_state = ScriptState::Waiting;
        } else if constexpr (std::is_same_v<T, ExprStatement>) {
            evaluate(*n.expr);
        } else {
            // Expression nodes used as statements - evaluate them
            evaluate(node);
        }
    }, node);
}

void Interpreter::executeBlock(const Block& block) {
    for (auto& stmt : block.statements) {
        if (m_waitRequested) return;
        execute(*stmt);
    }
}

// --- Evaluate expressions ---

ExtValue Interpreter::evaluate(const ASTNode& node) {
    countInstruction();

    return std::visit([this](const auto& n) -> ExtValue {
        using T = std::decay_t<decltype(n)>;

        if constexpr (std::is_same_v<T, NumberLiteral>) {
            return n.value;
        } else if constexpr (std::is_same_v<T, BoolLiteral>) {
            return n.value;
        } else if constexpr (std::is_same_v<T, Variable>) {
            // Check built-in read-only variables
            if (n.name == "time" && m_callbacks && m_callbacks->getLevelTime) {
                return static_cast<double>(m_callbacks->getLevelTime());
            }
            if (n.name == "attempt" && m_callbacks && m_callbacks->getAttempt) {
                return static_cast<double>(m_callbacks->getAttempt());
            }
            // Check for "player" and "camera" as namespace objects
            if (n.name == "player") {
                return ObjectHandle{-1}; // special: -1 = player handle
            }
            if (n.name == "camera") {
                return ObjectHandle{-2}; // special: -2 = camera handle
            }

            auto val = m_env.get(n.name);
            if (!val.has_value()) {
                throw RuntimeError(fmt::format("Undefined variable '{}'", n.name), n.line, 0);
            }
            if (auto* d = std::get_if<double>(&val.value())) return *d;
            if (auto* b = std::get_if<bool>(&val.value())) return *b;
            return 0.0;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            auto left = evaluate(*n.left);
            auto right = evaluate(*n.right);
            double l = requireNumber(left, "binary op", n.line);
            double r = requireNumber(right, "binary op", n.line);

            switch (n.op) {
                case TokenType::Plus: return l + r;
                case TokenType::Minus: return l - r;
                case TokenType::Star: return l * r;
                case TokenType::Slash:
                    if (r == 0.0) throw RuntimeError("Division by zero", n.line, 0);
                    return l / r;
                case TokenType::Percent:
                    if (r == 0.0) throw RuntimeError("Modulo by zero", n.line, 0);
                    return std::fmod(l, r);
                case TokenType::Less: return l < r;
                case TokenType::LessEqual: return l <= r;
                case TokenType::Greater: return l > r;
                case TokenType::GreaterEqual: return l >= r;
                case TokenType::EqualEqual: return l == r;
                case TokenType::BangEqual: return l != r;
                case TokenType::And: return (l != 0.0) && (r != 0.0);
                case TokenType::Or: return (l != 0.0) || (r != 0.0);
                default:
                    throw RuntimeError("Unknown binary operator", n.line, 0);
            }
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            auto operand = evaluate(*n.operand);
            if (n.op == TokenType::Minus) {
                return -requireNumber(operand, "negation", n.line);
            }
            if (n.op == TokenType::Bang) {
                double val = requireNumber(operand, "not", n.line);
                return val == 0.0;
            }
            throw RuntimeError("Unknown unary operator", n.line, 0);
        } else if constexpr (std::is_same_v<T, FunctionCall>) {
            // Evaluate callee
            // If callee is a Variable, it might be a built-in function
            if (auto* var = std::get_if<Variable>(n.callee.get())) {
                std::vector<ExtValue> args;
                for (auto& arg : n.args) {
                    args.push_back(evaluate(*arg));
                }
                return callBuiltinFunction(var->name, args, n.line);
            }
            throw RuntimeError("Only built-in functions are supported", n.line, 0);
        } else if constexpr (std::is_same_v<T, MethodCall>) {
            auto obj = evaluate(*n.object);
            std::vector<ExtValue> args;
            for (auto& arg : n.args) {
                args.push_back(evaluate(*arg));
            }
            return callMethod(obj, n.method, args, n.line);
        } else if constexpr (std::is_same_v<T, MemberAccess>) {
            auto obj = evaluate(*n.object);
            return getMember(obj, n.member, n.line);
        } else {
            // Statement nodes shouldn't be evaluated as expressions
            return 0.0;
        }
    }, node);
}

ExtValue Interpreter::callBuiltinFunction(const std::string& name,
                                           const std::vector<ExtValue>& args, int line) {
    if (name == "obj") {
        if (args.size() != 1)
            throw RuntimeError("obj() takes 1 argument (group ID)", line, 0);
        int groupID = requireInt(args[0], "obj() group ID", line);
        return ObjectHandle{groupID};
    }
    if (name == "color") {
        if (args.size() != 1)
            throw RuntimeError("color() takes 1 argument (channel ID)", line, 0);
        int channelID = requireInt(args[0], "color() channel ID", line);
        return ColorHandle{channelID};
    }
    if (name == "abs") {
        if (args.size() != 1) throw RuntimeError("abs() takes 1 argument", line, 0);
        return std::abs(requireNumber(args[0], "abs()", line));
    }
    if (name == "min") {
        if (args.size() != 2) throw RuntimeError("min() takes 2 arguments", line, 0);
        return std::min(requireNumber(args[0], "min()", line),
                        requireNumber(args[1], "min()", line));
    }
    if (name == "max") {
        if (args.size() != 2) throw RuntimeError("max() takes 2 arguments", line, 0);
        return std::max(requireNumber(args[0], "max()", line),
                        requireNumber(args[1], "max()", line));
    }
    if (name == "sin") {
        if (args.size() != 1) throw RuntimeError("sin() takes 1 argument", line, 0);
        return std::sin(requireNumber(args[0], "sin()", line));
    }
    if (name == "cos") {
        if (args.size() != 1) throw RuntimeError("cos() takes 1 argument", line, 0);
        return std::cos(requireNumber(args[0], "cos()", line));
    }

    throw RuntimeError(fmt::format("Unknown function '{}'", name), line, 0);
}

ExtValue Interpreter::callMethod(const ExtValue& object, const std::string& method,
                                  const std::vector<ExtValue>& args, int line) {
    if (!m_callbacks) {
        throw RuntimeError("No game callbacks available", line, 0);
    }

    // Object handle methods
    if (auto* oh = std::get_if<ObjectHandle>(&object)) {
        if (oh->groupID == -1) {
            // Player methods
            if (method == "speed" && args.size() == 1) {
                if (m_callbacks->setPlayerSpeed)
                    m_callbacks->setPlayerSpeed(static_cast<float>(requireNumber(args[0], "player.speed()", line)));
                return 0.0;
            }
            if (method == "gravity" && args.size() == 1) {
                double val = requireNumber(args[0], "player.gravity()", line);
                if (m_callbacks->setPlayerGravity)
                    m_callbacks->setPlayerGravity(val != 0.0);
                return 0.0;
            }
            if (method == "pulse" && args.size() == 4) {
                if (m_callbacks->pulsePlayer)
                    m_callbacks->pulsePlayer(
                        requireInt(args[0], "pulse r", line),
                        requireInt(args[1], "pulse g", line),
                        requireInt(args[2], "pulse b", line),
                        static_cast<float>(requireNumber(args[3], "pulse dur", line))
                    );
                return 0.0;
            }
            if (method == "trail" && args.size() == 1) {
                if (m_callbacks->enableTrail)
                    m_callbacks->enableTrail(requireNumber(args[0], "player.trail()", line) != 0.0);
                return 0.0;
            }
            if (method == "kill" && args.empty()) {
                if (m_callbacks->killPlayer) m_callbacks->killPlayer();
                return 0.0;
            }
            if (method == "jump" && args.empty()) {
                if (m_callbacks->forceJump) m_callbacks->forceJump();
                return 0.0;
            }
            throw RuntimeError(fmt::format("Unknown player method '{}'", method), line, 0);
        }

        if (oh->groupID == -2) {
            // Camera methods
            if (method == "move" && args.size() == 2) {
                if (m_callbacks->moveCamera)
                    m_callbacks->moveCamera(
                        static_cast<float>(requireNumber(args[0], "camera.move dx", line)),
                        static_cast<float>(requireNumber(args[1], "camera.move dy", line))
                    );
                return 0.0;
            }
            if (method == "zoom" && args.size() == 1) {
                if (m_callbacks->zoomCamera)
                    m_callbacks->zoomCamera(static_cast<float>(requireNumber(args[0], "camera.zoom", line)));
                return 0.0;
            }
            if (method == "shake" && args.size() == 2) {
                if (m_callbacks->shakeCamera)
                    m_callbacks->shakeCamera(
                        static_cast<float>(requireNumber(args[0], "camera.shake str", line)),
                        static_cast<float>(requireNumber(args[1], "camera.shake dur", line))
                    );
                return 0.0;
            }
            if (method == "rotate" && args.size() == 1) {
                if (m_callbacks->rotateCamera)
                    m_callbacks->rotateCamera(static_cast<float>(requireNumber(args[0], "camera.rotate", line)));
                return 0.0;
            }
            throw RuntimeError(fmt::format("Unknown camera method '{}'", method), line, 0);
        }

        // Regular object (group ID) methods
        int gid = oh->groupID;
        if (method == "move" && args.size() == 2) {
            if (m_callbacks->moveObject)
                m_callbacks->moveObject(gid,
                    static_cast<float>(requireNumber(args[0], "move dx", line)),
                    static_cast<float>(requireNumber(args[1], "move dy", line)));
            return 0.0;
        }
        if (method == "moveTo" && args.size() == 2) {
            if (m_callbacks->moveObjectTo)
                m_callbacks->moveObjectTo(gid,
                    static_cast<float>(requireNumber(args[0], "moveTo x", line)),
                    static_cast<float>(requireNumber(args[1], "moveTo y", line)));
            return 0.0;
        }
        if (method == "rotate" && args.size() == 1) {
            if (m_callbacks->rotateObject)
                m_callbacks->rotateObject(gid, static_cast<float>(requireNumber(args[0], "rotate deg", line)));
            return 0.0;
        }
        if (method == "scale" && args.size() == 1) {
            if (m_callbacks->scaleObject)
                m_callbacks->scaleObject(gid, static_cast<float>(requireNumber(args[0], "scale", line)));
            return 0.0;
        }
        if (method == "scaleXY" && args.size() == 2) {
            if (m_callbacks->scaleObjectXY)
                m_callbacks->scaleObjectXY(gid,
                    static_cast<float>(requireNumber(args[0], "scaleXY sx", line)),
                    static_cast<float>(requireNumber(args[1], "scaleXY sy", line)));
            return 0.0;
        }
        if (method == "toggle" && args.size() == 1) {
            if (m_callbacks->toggleObject)
                m_callbacks->toggleObject(gid, requireNumber(args[0], "toggle", line) != 0.0);
            return 0.0;
        }
        if (method == "alpha" && args.size() == 1) {
            if (m_callbacks->setObjectAlpha)
                m_callbacks->setObjectAlpha(gid, static_cast<float>(requireNumber(args[0], "alpha", line)));
            return 0.0;
        }
        if (method == "color" && args.size() == 3) {
            if (m_callbacks->setObjectColor)
                m_callbacks->setObjectColor(gid,
                    requireInt(args[0], "color r", line),
                    requireInt(args[1], "color g", line),
                    requireInt(args[2], "color b", line));
            return 0.0;
        }
        throw RuntimeError(fmt::format("Unknown object method '{}'", method), line, 0);
    }

    // Color handle methods
    if (auto* ch = std::get_if<ColorHandle>(&object)) {
        if (method == "set" && args.size() == 3) {
            if (m_callbacks->setColorChannel)
                m_callbacks->setColorChannel(ch->channelID,
                    requireInt(args[0], "color r", line),
                    requireInt(args[1], "color g", line),
                    requireInt(args[2], "color b", line));
            return 0.0;
        }
        if (method == "pulse" && args.size() == 4) {
            if (m_callbacks->pulseColorChannel)
                m_callbacks->pulseColorChannel(ch->channelID,
                    requireInt(args[0], "pulse r", line),
                    requireInt(args[1], "pulse g", line),
                    requireInt(args[2], "pulse b", line),
                    static_cast<float>(requireNumber(args[3], "pulse dur", line)));
            return 0.0;
        }
        throw RuntimeError(fmt::format("Unknown color method '{}'", method), line, 0);
    }

    throw RuntimeError(fmt::format("Cannot call method '{}' on this value", method), line, 0);
}

ExtValue Interpreter::getMember(const ExtValue& object, const std::string& member, int line) {
    if (auto* oh = std::get_if<ObjectHandle>(&object)) {
        if (oh->groupID == -1 && m_callbacks) {
            // Player properties
            if (member == "x" && m_callbacks->getPlayerX)
                return static_cast<double>(m_callbacks->getPlayerX());
            if (member == "y" && m_callbacks->getPlayerY)
                return static_cast<double>(m_callbacks->getPlayerY());
            if (member == "xvel" && m_callbacks->getPlayerXVel)
                return static_cast<double>(m_callbacks->getPlayerXVel());
            if (member == "yvel" && m_callbacks->getPlayerYVel)
                return static_cast<double>(m_callbacks->getPlayerYVel());
            throw RuntimeError(fmt::format("Unknown player property '{}'", member), line, 0);
        }
    }
    throw RuntimeError(fmt::format("Cannot access member '{}' on this value", member), line, 0);
}

} // namespace scriptable
