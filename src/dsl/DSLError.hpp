#pragma once
#include <stdexcept>
#include <string>
#include <fmt/format.h>

namespace scriptable {

class DSLError : public std::runtime_error {
public:
    int line;
    int col;

    DSLError(const std::string& msg, int line, int col)
        : std::runtime_error(fmt::format("Line {}: {}", line, msg))
        , line(line), col(col) {}
};

class ParseError : public DSLError {
public:
    using DSLError::DSLError;
};

class RuntimeError : public DSLError {
public:
    using DSLError::DSLError;
};

class InstructionLimitError : public std::runtime_error {
public:
    InstructionLimitError()
        : std::runtime_error("Script exceeded instruction limit") {}
};

} // namespace scriptable
