#include "Lexer.hpp"
#include "DSLError.hpp"
#include <cctype>

namespace scriptable {

Lexer::Lexer(const std::string& source) : m_source(source) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_pos];
}

char Lexer::peekNext() const {
    if (m_pos + 1 >= m_source.size()) return '\0';
    return m_source[m_pos + 1];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') {
        m_line++;
        m_col = 1;
    } else {
        m_col++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return m_pos >= m_source.size();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            skipComment();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) const {
    return Token{type, value, m_line, m_col};
}

Token Lexer::readNumber() {
    int startCol = m_col;
    std::string num;
    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        num += advance();
    }
    return Token{TokenType::Number, num, m_line, startCol};
}

Token Lexer::readIdentifier() {
    int startCol = m_col;
    std::string id;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        id += advance();
    }
    auto& kw = keywords();
    auto it = kw.find(id);
    TokenType type = (it != kw.end()) ? it->second : TokenType::Identifier;
    return Token{type, id, m_line, startCol};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    bool lastWasNewline = true;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();

        if (c == '\n') {
            advance();
            if (!lastWasNewline) {
                tokens.push_back(makeToken(TokenType::Newline, "\\n"));
                lastWasNewline = true;
            }
            continue;
        }

        lastWasNewline = false;

        if (std::isdigit(c) || (c == '.' && std::isdigit(peekNext()))) {
            tokens.push_back(readNumber());
            continue;
        }

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifier());
            continue;
        }

        int startCol = m_col;
        advance();

        switch (c) {
            case '+': tokens.push_back({TokenType::Plus, "+", m_line, startCol}); break;
            case '-': tokens.push_back({TokenType::Minus, "-", m_line, startCol}); break;
            case '*': tokens.push_back({TokenType::Star, "*", m_line, startCol}); break;
            case '/': tokens.push_back({TokenType::Slash, "/", m_line, startCol}); break;
            case '%': tokens.push_back({TokenType::Percent, "%", m_line, startCol}); break;
            case '(': tokens.push_back({TokenType::LParen, "(", m_line, startCol}); break;
            case ')': tokens.push_back({TokenType::RParen, ")", m_line, startCol}); break;
            case '{': tokens.push_back({TokenType::LBrace, "{", m_line, startCol}); break;
            case '}': tokens.push_back({TokenType::RBrace, "}", m_line, startCol}); break;
            case ',': tokens.push_back({TokenType::Comma, ",", m_line, startCol}); break;
            case '.': tokens.push_back({TokenType::Dot, ".", m_line, startCol}); break;
            case ';': tokens.push_back({TokenType::Semicolon, ";", m_line, startCol}); break;
            case '=':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back({TokenType::EqualEqual, "==", m_line, startCol});
                } else {
                    tokens.push_back({TokenType::Equal, "=", m_line, startCol});
                }
                break;
            case '!':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back({TokenType::BangEqual, "!=", m_line, startCol});
                } else {
                    tokens.push_back({TokenType::Bang, "!", m_line, startCol});
                }
                break;
            case '<':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back({TokenType::LessEqual, "<=", m_line, startCol});
                } else {
                    tokens.push_back({TokenType::Less, "<", m_line, startCol});
                }
                break;
            case '>':
                if (!isAtEnd() && peek() == '=') {
                    advance();
                    tokens.push_back({TokenType::GreaterEqual, ">=", m_line, startCol});
                } else {
                    tokens.push_back({TokenType::Greater, ">", m_line, startCol});
                }
                break;
            default:
                throw ParseError(
                    fmt::format("Unexpected character '{}'", c),
                    m_line, startCol
                );
        }
    }

    tokens.push_back({TokenType::Eof, "", m_line, m_col});
    return tokens;
}

} // namespace scriptable
