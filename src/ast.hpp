// ============================================================================
// ast.hpp — Abstract Syntax Tree node classes for Lucid.
// Phase: (shared)  Input: built by the Parser.  Output: consumed by the
// SemanticAnalyzer, Optimizer, and Interpreter; serialized via toJson() for
// the playground's graphical tree.
//
// Ownership: every child is a std::unique_ptr<Node> (no raw new/delete).
// Each toJson() emits a uniform shape { kind, label, line, children:[...] }
// so the front-end can draw any node the same way.
// ============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../third_party/json.hpp"

namespace lucid {

enum class NodeKind {
    Program, Block,
    VarDecl, Assign, If, While, Print, FuncDecl, Return,
    Call, BinaryOp, UnaryOp,
    Identifier, IntLiteral, StringLiteral, BoolLiteral
};

inline const char* nodeKindName(NodeKind k) {
    switch (k) {
        case NodeKind::Program:       return "Program";
        case NodeKind::Block:         return "Block";
        case NodeKind::VarDecl:       return "VarDecl";
        case NodeKind::Assign:        return "Assign";
        case NodeKind::If:            return "If";
        case NodeKind::While:         return "While";
        case NodeKind::Print:         return "Print";
        case NodeKind::FuncDecl:      return "FuncDecl";
        case NodeKind::Return:        return "Return";
        case NodeKind::Call:          return "Call";
        case NodeKind::BinaryOp:      return "BinaryOp";
        case NodeKind::UnaryOp:       return "UnaryOp";
        case NodeKind::Identifier:    return "Identifier";
        case NodeKind::IntLiteral:    return "IntLiteral";
        case NodeKind::StringLiteral: return "StringLiteral";
        case NodeKind::BoolLiteral:   return "BoolLiteral";
    }
    return "Unknown";
}

// ---- Base ----------------------------------------------------------------
struct Node {
    NodeKind kind;
    int line;
    Node(NodeKind k, int ln) : kind(k), line(ln) {}
    virtual ~Node() = default;
    virtual nlohmann::json toJson() const = 0;
};
using NodePtr = std::unique_ptr<Node>;

// Helper: base JSON envelope every node shares.
inline nlohmann::json nodeBase(NodeKind k, const std::string& label, int line) {
    return nlohmann::json{
        {"kind", nodeKindName(k)},
        {"label", label},
        {"line", line},
        {"children", nlohmann::json::array()}
    };
}

// A typed function parameter, e.g. `int a`.
struct Param {
    std::string type;   // "int" | "string" | "bool"
    std::string name;
};

// ---- Container nodes -----------------------------------------------------
struct Program : Node {
    std::vector<NodePtr> statements;
    Program() : Node(NodeKind::Program, 0) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Program", line);
        for (const auto& s : statements) j["children"].push_back(s->toJson());
        return j;
    }
};

struct Block : Node {
    std::vector<NodePtr> statements;
    explicit Block(int ln) : Node(NodeKind::Block, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Block", line);
        for (const auto& s : statements) j["children"].push_back(s->toJson());
        return j;
    }
};

// ---- Statements ----------------------------------------------------------
struct VarDecl : Node {
    std::string declType;   // "int" | "string" | "bool"
    std::string name;
    NodePtr init;           // required initializer
    explicit VarDecl(int ln) : Node(NodeKind::VarDecl, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "VarDecl: " + declType + " " + name, line);
        if (init) j["children"].push_back(init->toJson());
        return j;
    }
};

struct Assign : Node {
    std::string name;
    NodePtr value;
    explicit Assign(int ln) : Node(NodeKind::Assign, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Assign: " + name, line);
        if (value) j["children"].push_back(value->toJson());
        return j;
    }
};

struct If : Node {
    NodePtr cond;
    std::unique_ptr<Block> thenBlock;
    std::unique_ptr<Block> elseBlock;   // may be null
    explicit If(int ln) : Node(NodeKind::If, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "If", line);
        if (cond)      j["children"].push_back(cond->toJson());
        if (thenBlock) { auto t = thenBlock->toJson(); t["label"] = "then"; j["children"].push_back(t); }
        if (elseBlock) { auto e = elseBlock->toJson(); e["label"] = "else"; j["children"].push_back(e); }
        return j;
    }
};

struct While : Node {
    NodePtr cond;
    std::unique_ptr<Block> body;
    explicit While(int ln) : Node(NodeKind::While, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "While", line);
        if (cond) j["children"].push_back(cond->toJson());
        if (body) { auto b = body->toJson(); b["label"] = "body"; j["children"].push_back(b); }
        return j;
    }
};

struct Print : Node {
    NodePtr expr;
    explicit Print(int ln) : Node(NodeKind::Print, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Print", line);
        if (expr) j["children"].push_back(expr->toJson());
        return j;
    }
};

struct FuncDecl : Node {
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::unique_ptr<Block> body;
    explicit FuncDecl(int ln) : Node(NodeKind::FuncDecl, ln) {}
    std::string signature() const {
        std::string s = name + "(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) s += ", ";
            s += params[i].type + " " + params[i].name;
        }
        s += "): " + returnType;
        return s;
    }
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "func " + signature(), line);
        if (body) { auto b = body->toJson(); b["label"] = "body"; j["children"].push_back(b); }
        return j;
    }
};

struct Return : Node {
    NodePtr value;   // required in Lucid (every function returns a value)
    explicit Return(int ln) : Node(NodeKind::Return, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Return", line);
        if (value) j["children"].push_back(value->toJson());
        return j;
    }
};

// ---- Expressions ---------------------------------------------------------
struct Call : Node {
    std::string callee;
    std::vector<NodePtr> args;
    explicit Call(int ln) : Node(NodeKind::Call, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, "Call: " + callee, line);
        for (const auto& a : args) j["children"].push_back(a->toJson());
        return j;
    }
};

struct BinaryOp : Node {
    std::string op;         // + - * / % < > <= >= == != && ||
    NodePtr left, right;
    explicit BinaryOp(int ln) : Node(NodeKind::BinaryOp, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, op, line);
        if (left)  j["children"].push_back(left->toJson());
        if (right) j["children"].push_back(right->toJson());
        return j;
    }
};

struct UnaryOp : Node {
    std::string op;         // ! or -
    NodePtr operand;
    explicit UnaryOp(int ln) : Node(NodeKind::UnaryOp, ln) {}
    nlohmann::json toJson() const override {
        auto j = nodeBase(kind, op + " (unary)", line);
        if (operand) j["children"].push_back(operand->toJson());
        return j;
    }
};

struct Identifier : Node {
    std::string name;
    explicit Identifier(int ln) : Node(NodeKind::Identifier, ln) {}
    nlohmann::json toJson() const override {
        return nodeBase(kind, name, line);
    }
};

struct IntLiteral : Node {
    int value = 0;
    explicit IntLiteral(int ln) : Node(NodeKind::IntLiteral, ln) {}
    nlohmann::json toJson() const override {
        return nodeBase(kind, std::to_string(value), line);
    }
};

struct StringLiteral : Node {
    std::string value;
    explicit StringLiteral(int ln) : Node(NodeKind::StringLiteral, ln) {}
    nlohmann::json toJson() const override {
        return nodeBase(kind, "\"" + value + "\"", line);
    }
};

struct BoolLiteral : Node {
    bool value = false;
    explicit BoolLiteral(int ln) : Node(NodeKind::BoolLiteral, ln) {}
    nlohmann::json toJson() const override {
        return nodeBase(kind, value ? "true" : "false", line);
    }
};

// Count every node in a serialized AST (containers included). Used by the
// optimizer to report node counts before/after.
inline int countJsonNodes(const nlohmann::json& j) {
    int n = 1;
    if (j.contains("children"))
        for (const auto& c : j["children"]) n += countJsonNodes(c);
    return n;
}

} // namespace lucid
