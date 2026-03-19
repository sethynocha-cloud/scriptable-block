#pragma once
#include "AST.hpp"
#include "Token.hpp"
#include <vector>

namespace scriptable {

class Parser {
    std::vector<Token> m_tokens;
    size_t m_pos = 0;

    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& msg);
    void skipNewlines();
    void expectEnd(); // expect newline, semicolon, or eof

    // Grammar rules
    ASTNodePtr parseStatement();
    ASTNodePtr parseLetStatement();
    ASTNodePtr parseIfStatement();
    ASTNodePtr parseWhileStatement();
    ASTNodePtr parseRepeatStatement();
    ASTNodePtr parseExpressionStatement();
    ASTNodePtr parseBlock();

    ASTNodePtr parseExpression();
    ASTNodePtr parseOr();
    ASTNodePtr parseAnd();
    ASTNodePtr parseEquality();
    ASTNodePtr parseComparison();
    ASTNodePtr parseAddition();
    ASTNodePtr parseMultiplication();
    ASTNodePtr parseUnary();
    ASTNodePtr parseCall();
    ASTNodePtr parsePrimary();
    std::vector<ASTNodePtr> parseArguments();

public:
    explicit Parser(const std::vector<Token>& tokens);
    ASTNodePtr parse(); // Returns a Block containing all statements
};

} // namespace scriptable
