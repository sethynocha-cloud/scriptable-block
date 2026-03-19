#pragma once
#include "Token.hpp"
#include <string>
#include <vector>

namespace scriptable {

class Lexer {
    std::string m_source;
    size_t m_pos = 0;
    int m_line = 1;
    int m_col = 1;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    void skipComment();
    Token makeToken(TokenType type, const std::string& value) const;
    Token readNumber();
    Token readIdentifier();

public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();
};

} // namespace scriptable
