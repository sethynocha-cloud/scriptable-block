#include "Parser.hpp"
#include "DSLError.hpp"
#include <fmt/format.h>

namespace scriptable {

Parser::Parser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

const Token& Parser::peek() const {
    return m_tokens[m_pos];
}

const Token& Parser::previous() const {
    return m_tokens[m_pos - 1];
}

const Token& Parser::advance() {
    if (peek().type != TokenType::Eof) m_pos++;
    return previous();
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

Token Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    throw ParseError(msg, peek().line, peek().col);
}

void Parser::skipNewlines() {
    while (match(TokenType::Newline) || match(TokenType::Semicolon)) {}
}

void Parser::expectEnd() {
    if (check(TokenType::Eof) || check(TokenType::RBrace)) return;
    if (!match(TokenType::Newline) && !match(TokenType::Semicolon)) {
        throw ParseError("Expected newline or ';' after statement", peek().line, peek().col);
    }
    skipNewlines();
}

ASTNodePtr Parser::parse() {
    std::vector<ASTNodePtr> statements;
    skipNewlines();
    while (!check(TokenType::Eof)) {
        statements.push_back(parseStatement());
        skipNewlines();
    }
    return std::make_unique<ASTNode>(Block{std::move(statements)});
}

ASTNodePtr Parser::parseStatement() {
    if (check(TokenType::Let)) return parseLetStatement();
    if (check(TokenType::If)) return parseIfStatement();
    if (check(TokenType::While)) return parseWhileStatement();
    if (check(TokenType::Repeat)) return parseRepeatStatement();

    // Check for assignment: IDENTIFIER = expr
    if (check(TokenType::Identifier) && m_pos + 1 < m_tokens.size()
        && m_tokens[m_pos + 1].type == TokenType::Equal) {
        int line = peek().line;
        std::string name = advance().value;
        advance(); // consume '='
        auto value = parseExpression();
        expectEnd();
        auto node = std::make_unique<ASTNode>(AssignStatement{name, std::move(value), line});
        return node;
    }

    return parseExpressionStatement();
}

ASTNodePtr Parser::parseLetStatement() {
    int line = peek().line;
    consume(TokenType::Let, "Expected 'let'");
    std::string name = consume(TokenType::Identifier, "Expected variable name").value;
    consume(TokenType::Equal, "Expected '=' after variable name");
    auto init = parseExpression();
    expectEnd();
    return std::make_unique<ASTNode>(LetStatement{name, std::move(init), line});
}

ASTNodePtr Parser::parseIfStatement() {
    int line = peek().line;
    consume(TokenType::If, "Expected 'if'");
    auto condition = parseExpression();
    auto thenBranch = parseBlock();
    ASTNodePtr elseBranch = nullptr;
    skipNewlines();
    if (match(TokenType::Else)) {
        if (check(TokenType::If)) {
            // else if: wrap in a block containing a single if statement
            auto elseIf = parseIfStatement();
            std::vector<ASTNodePtr> stmts;
            stmts.push_back(std::move(elseIf));
            elseBranch = std::make_unique<ASTNode>(Block{std::move(stmts)});
        } else {
            elseBranch = parseBlock();
        }
    }
    return std::make_unique<ASTNode>(IfStatement{
        std::move(condition), std::move(thenBranch), std::move(elseBranch), line
    });
}

ASTNodePtr Parser::parseWhileStatement() {
    int line = peek().line;
    consume(TokenType::While, "Expected 'while'");
    auto condition = parseExpression();
    auto body = parseBlock();
    return std::make_unique<ASTNode>(WhileStatement{
        std::move(condition), std::move(body), line
    });
}

ASTNodePtr Parser::parseRepeatStatement() {
    int line = peek().line;
    consume(TokenType::Repeat, "Expected 'repeat'");
    auto count = parseExpression();
    auto body = parseBlock();
    return std::make_unique<ASTNode>(RepeatStatement{
        std::move(count), std::move(body), line
    });
}

ASTNodePtr Parser::parseExpressionStatement() {
    int line = peek().line;

    // Check for wait() as a special statement
    if (check(TokenType::Identifier) && peek().value == "wait") {
        int wline = peek().line;
        advance(); // consume 'wait'
        consume(TokenType::LParen, "Expected '(' after 'wait'");
        auto duration = parseExpression();
        consume(TokenType::RParen, "Expected ')' after wait duration");
        expectEnd();
        return std::make_unique<ASTNode>(WaitStatement{std::move(duration), wline});
    }

    auto expr = parseExpression();
    expectEnd();
    return std::make_unique<ASTNode>(ExprStatement{std::move(expr), line});
}

ASTNodePtr Parser::parseBlock() {
    skipNewlines();
    consume(TokenType::LBrace, "Expected '{'");
    skipNewlines();
    std::vector<ASTNodePtr> statements;
    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        statements.push_back(parseStatement());
        skipNewlines();
    }
    consume(TokenType::RBrace, "Expected '}'");
    return std::make_unique<ASTNode>(Block{std::move(statements)});
}

// Expression parsing (precedence climbing)

ASTNodePtr Parser::parseExpression() {
    return parseOr();
}

ASTNodePtr Parser::parseOr() {
    auto left = parseAnd();
    while (match(TokenType::Or)) {
        int line = previous().line;
        auto right = parseAnd();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), TokenType::Or, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseAnd() {
    auto left = parseEquality();
    while (match(TokenType::And)) {
        int line = previous().line;
        auto right = parseEquality();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), TokenType::And, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EqualEqual) || check(TokenType::BangEqual)) {
        auto op = advance().type;
        int line = previous().line;
        auto right = parseComparison();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), op, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseComparison() {
    auto left = parseAddition();
    while (check(TokenType::Less) || check(TokenType::LessEqual)
        || check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
        auto op = advance().type;
        int line = previous().line;
        auto right = parseAddition();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), op, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseAddition() {
    auto left = parseMultiplication();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        auto op = advance().type;
        int line = previous().line;
        auto right = parseMultiplication();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), op, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseMultiplication() {
    auto left = parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        auto op = advance().type;
        int line = previous().line;
        auto right = parseUnary();
        left = std::make_unique<ASTNode>(BinaryExpr{
            std::move(left), op, std::move(right), line
        });
    }
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (check(TokenType::Bang) || check(TokenType::Minus)) {
        auto op = advance().type;
        int line = previous().line;
        auto operand = parseUnary();
        return std::make_unique<ASTNode>(UnaryExpr{op, std::move(operand), line});
    }
    return parseCall();
}

ASTNodePtr Parser::parseCall() {
    auto expr = parsePrimary();
    int line = previous().line;

    while (true) {
        if (match(TokenType::LParen)) {
            auto args = parseArguments();
            consume(TokenType::RParen, "Expected ')' after arguments");
            expr = std::make_unique<ASTNode>(FunctionCall{
                std::move(expr), std::move(args), line
            });
        } else if (match(TokenType::Dot)) {
            std::string member = consume(TokenType::Identifier, "Expected property name after '.'").value;
            if (match(TokenType::LParen)) {
                auto args = parseArguments();
                consume(TokenType::RParen, "Expected ')' after arguments");
                expr = std::make_unique<ASTNode>(MethodCall{
                    std::move(expr), member, std::move(args), line
                });
            } else {
                expr = std::make_unique<ASTNode>(MemberAccess{
                    std::move(expr), member, line
                });
            }
        } else {
            break;
        }
    }

    return expr;
}

std::vector<ASTNodePtr> Parser::parseArguments() {
    std::vector<ASTNodePtr> args;
    if (!check(TokenType::RParen)) {
        args.push_back(parseExpression());
        while (match(TokenType::Comma)) {
            args.push_back(parseExpression());
        }
    }
    return args;
}

ASTNodePtr Parser::parsePrimary() {
    if (match(TokenType::Number)) {
        return std::make_unique<ASTNode>(NumberLiteral{std::stod(previous().value)});
    }
    if (match(TokenType::True)) {
        return std::make_unique<ASTNode>(BoolLiteral{true});
    }
    if (match(TokenType::False)) {
        return std::make_unique<ASTNode>(BoolLiteral{false});
    }
    if (match(TokenType::Identifier)) {
        return std::make_unique<ASTNode>(Variable{previous().value, previous().line});
    }
    if (match(TokenType::LParen)) {
        auto expr = parseExpression();
        consume(TokenType::RParen, "Expected ')'");
        return expr;
    }

    throw ParseError(
        fmt::format("Unexpected token '{}'", peek().value),
        peek().line, peek().col
    );
}

} // namespace scriptable
