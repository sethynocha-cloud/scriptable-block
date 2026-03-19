#pragma once
#include <string>
#include <unordered_map>

namespace scriptable {

enum class TokenType {
    // Literals
    Number, True, False,

    // Identifiers & Keywords
    Identifier, Let, If, Else, While, Repeat, And, Or,

    // Operators
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, Bang, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,

    // Punctuation
    LParen, RParen, LBrace, RBrace, Comma, Dot, Semicolon,

    // Special
    Newline, Eof
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;
};

inline const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"let", TokenType::Let},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"repeat", TokenType::Repeat},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"and", TokenType::And},
        {"or", TokenType::Or},
    };
    return kw;
}

} // namespace scriptable
