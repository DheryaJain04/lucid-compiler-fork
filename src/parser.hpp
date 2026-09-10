// ============================================================================
// parser.hpp — Phase 2 of Lucid.
// Input : std::vector<Token>
// Output: std::unique_ptr<Program> AST  (+ syntax errors into an ErrorList)
//
// A hand-written recursive-descent parser. On a syntax error it records the
// diagnostic, then panic-mode-synchronizes to the next statement so more than
// one error can be reported per run.
// ============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>
#include "token.hpp"
#include "ast.hpp"
#include "error.hpp"

namespace lucid {

class Parser {
public:
    Parser(std::vector<Token> tokens, const std::string& source, ErrorList& errors);
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens_;
    const std::string& source_;
    ErrorList& errors_;
    size_t pos_ = 0;

    // Thrown internally to unwind to the nearest recovery point.
    struct ParseError {};

    // --- token cursor helpers ---
    const Token& peek() const { return tokens_[pos_]; }
    const Token& previous() const { return tokens_[pos_ - 1]; }
    bool isAtEnd() const { return peek().type == TokenType::EndOfFile; }
    bool check(TokenType t) const { return peek().type == t; }
    const Token& advance();
    bool match(TokenType t);
    const Token& expect(TokenType t, const std::string& what);
    [[noreturn]] void error(const Token& tok, const std::string& message);
    void synchronize();

    // --- grammar ---
    NodePtr topLevel();
    NodePtr statement();
    std::unique_ptr<Block> block();
    NodePtr varDecl();
    NodePtr simpleStatement();  // assignment or bare call, starting with an identifier
    NodePtr ifStatement();
    NodePtr whileStatement();
    NodePtr printStatement();
    NodePtr returnStatement();
    NodePtr funcDecl();
    std::string typeName();

    // --- expressions (precedence climbing) ---
    NodePtr expression();
    NodePtr logicalOr();
    NodePtr logicalAnd();
    NodePtr equality();
    NodePtr comparison();
    NodePtr term();
    NodePtr factor();
    NodePtr unary();
    NodePtr primary();
    NodePtr finishCall(const Token& name);
};

} // namespace lucid
