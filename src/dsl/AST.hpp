#pragma once
#include "Token.hpp"
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace scriptable {

// Forward declare the node variant
struct NumberLiteral;
struct BoolLiteral;
struct Variable;
struct BinaryExpr;
struct UnaryExpr;
struct MemberAccess;
struct FunctionCall;
struct MethodCall;
struct LetStatement;
struct AssignStatement;
struct IfStatement;
struct WhileStatement;
struct RepeatStatement;
struct ExprStatement;
struct WaitStatement;
struct Block;

using ASTNode = std::variant<
    NumberLiteral, BoolLiteral, Variable,
    BinaryExpr, UnaryExpr, MemberAccess,
    FunctionCall, MethodCall,
    LetStatement, AssignStatement, IfStatement,
    WhileStatement, RepeatStatement, ExprStatement,
    WaitStatement, Block
>;

using ASTNodePtr = std::unique_ptr<ASTNode>;

// Helper to create nodes
template <typename T, typename... Args>
ASTNodePtr makeNode(Args&&... args) {
    return std::make_unique<ASTNode>(T{std::forward<Args>(args)...});
}

// Expression nodes
struct NumberLiteral {
    double value;
};

struct BoolLiteral {
    bool value;
};

struct Variable {
    std::string name;
    int line = 0;
};

struct BinaryExpr {
    ASTNodePtr left;
    TokenType op;
    ASTNodePtr right;
    int line = 0;
};

struct UnaryExpr {
    TokenType op;
    ASTNodePtr operand;
    int line = 0;
};

struct MemberAccess {
    ASTNodePtr object;
    std::string member;
    int line = 0;
};

struct FunctionCall {
    ASTNodePtr callee;
    std::vector<ASTNodePtr> args;
    int line = 0;
};

struct MethodCall {
    ASTNodePtr object;
    std::string method;
    std::vector<ASTNodePtr> args;
    int line = 0;
};

// Statement nodes
struct LetStatement {
    std::string name;
    ASTNodePtr initializer;
    int line = 0;
};

struct AssignStatement {
    std::string name;
    ASTNodePtr value;
    int line = 0;
};

struct IfStatement {
    ASTNodePtr condition;
    ASTNodePtr thenBranch; // Block
    ASTNodePtr elseBranch; // Block or nullptr
    int line = 0;
};

struct WhileStatement {
    ASTNodePtr condition;
    ASTNodePtr body; // Block
    int line = 0;
};

struct RepeatStatement {
    ASTNodePtr count;
    ASTNodePtr body; // Block
    int line = 0;
};

struct ExprStatement {
    ASTNodePtr expr;
    int line = 0;
};

struct WaitStatement {
    ASTNodePtr duration;
    int line = 0;
};

struct Block {
    std::vector<ASTNodePtr> statements;
};

} // namespace scriptable
