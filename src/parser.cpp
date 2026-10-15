// ============================================================================
// parser.cpp — Phase 2 of Lucid.  tokens -> AST.
// Recursive descent with precedence-climbing expressions and panic-mode
// recovery. Every node records the source line it came from.
// ============================================================================
#include "parser.hpp"

#include <unordered_map>

namespace lucid {

Parser::Parser(std::vector<Token> tokens, const std::string& source, ErrorList& errors)
    : tokens_(std::move(tokens)), source_(source), errors_(errors) {}

const Token& Parser::advance() {
    if (!isAtEnd()) ++pos_;
    return previous();
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& what) {
    if (check(t)) return advance();
    error(peek(), "expected " + what + " but found '" +
                  (peek().type == TokenType::EndOfFile ? "end of file" : peek().value) + "'");
}

void Parser::error(const Token& tok, const std::string& message) {
    errors_.add("syntax", "UnexpectedToken", message, tok.line, tok.col,
                sourceLine(source_, tok.line));
    throw ParseError{};
}

// Skip tokens until we reach a likely statement boundary, so parsing can
// resume after a syntax error instead of cascading.
void Parser::synchronize() {
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;
        switch (peek().type) {
            case TokenType::KwInt: case TokenType::KwString: case TokenType::KwBool:
            case TokenType::KwIf: case TokenType::KwWhile: case TokenType::KwPrint:
            case TokenType::KwReturn: case TokenType::KwFunc:
            case TokenType::RBrace:
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    while (!isAtEnd()) {
        size_t before = pos_;
        try {
            program->statements.push_back(topLevel());
        } catch (const ParseError&) {
            synchronize();
        }
        // Progress guard: synchronize() can legitimately stop *on* a token it
        // won't consume (e.g. a stray top-level '}'), and statement() then fails
        // on that same token — an infinite loop. Force one token forward if a
        // whole iteration consumed nothing. Also cap runaway error floods.
        if (pos_ == before && !isAtEnd()) advance();
        if (errors_.size() > 100) break;
    }
    return program;
}

NodePtr Parser::topLevel() {
    if (check(TokenType::KwFunc)) return funcDecl();
    return statement();
}

std::string Parser::typeName() {
    if (match(TokenType::KwInt))    return "int";
    if (match(TokenType::KwString)) return "string";
    if (match(TokenType::KwBool))   return "bool";
    error(peek(), "expected a type (int, string, or bool) but found '" + peek().value + "'");
}

NodePtr Parser::statement() {
    switch (peek().type) {
        case TokenType::KwInt:
        case TokenType::KwString:
        case TokenType::KwBool:   return varDecl();
        case TokenType::KwIf:     return ifStatement();
        case TokenType::KwWhile:  return whileStatement();
        case TokenType::KwFor:    return forStatement();
        case TokenType::KwPrint:  return printStatement();
        case TokenType::KwReturn: return returnStatement();
        case TokenType::KwFunc:   return funcDecl();
        case TokenType::Identifier: return simpleStatement();
        default:
            error(peek(), "expected a statement but found '" + peek().value + "'");
    }
}

std::unique_ptr<Block> Parser::block() {
    int ln = peek().line;
    expect(TokenType::LBrace, "'{'");
    auto blk = std::make_unique<Block>(ln);
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        size_t before = pos_;
        try {
            blk->statements.push_back(statement());
        } catch (const ParseError&) {
            synchronize();
        }
        if (pos_ == before && !check(TokenType::RBrace) && !isAtEnd()) advance();
    }
    expect(TokenType::RBrace, "'}'");
    return blk;
}

NodePtr Parser::varDecl() {
    int ln = peek().line;
    std::string ty = typeName();
    const Token& name = expect(TokenType::Identifier, "a variable name");
    expect(TokenType::Assign, "'='");
    auto init = expression();
    expect(TokenType::Semicolon, "';'");

    auto node = std::make_unique<VarDecl>(ln);
    node->declType = ty;
    node->name = name.value;
    node->init = std::move(init);
    return node;
}

NodePtr Parser::simpleStatement() {
    if (check(TokenType::Identifier) &&
        (tokens_[pos_ + 1].type == TokenType::Assign || isCompoundAssign(tokens_[pos_ + 1].type))) {
        auto node = assignmentNoSemi();
        expect(TokenType::Semicolon, "';'");
        return node;
    }
    const Token& name = advance();  // the identifier
    if (check(TokenType::LParen)) {
        auto call = finishCall(name);
        expect(TokenType::Semicolon, "';'");
        return call;
    }
    error(peek(), "expected '=' or '(' after '" + name.value + "'");
}

NodePtr Parser::ifStatement() {
    int ln = peek().line;
    advance();  // 'if'
    expect(TokenType::LParen, "'(' after 'if'");
    auto cond = expression();
    expect(TokenType::RParen, "')'");
    auto thenB = block();
    std::unique_ptr<Block> elseB;
    if (match(TokenType::KwElse)) {
        if (check(TokenType::KwIf)) {
            // `else if` is sugar: the else branch is a block holding one If.
            int nested = peek().line;
            elseB = std::make_unique<Block>(nested);
            elseB->statements.push_back(ifStatement());
        } else {
            elseB = block();
        }
    }
    auto node = std::make_unique<If>(ln);
    node->cond = std::move(cond);
    node->thenBlock = std::move(thenB);
    node->elseBlock = std::move(elseB);
    return node;
}

bool Parser::isCompoundAssign(TokenType t) const {
    return t == TokenType::PlusAssign  || t == TokenType::MinusAssign ||
           t == TokenType::StarAssign  || t == TokenType::SlashAssign ||
           t == TokenType::PercentAssign;
}

bool Parser::isTypeStart() const {
    return check(TokenType::KwInt) || check(TokenType::KwString) || check(TokenType::KwBool);
}

// `x = e`, or a compound assignment desugared to `x = x op e`.
NodePtr Parser::assignmentNoSemi() {
    const Token& name = expect(TokenType::Identifier, "a variable name");
    auto node = std::make_unique<Assign>(name.line);
    node->name = name.value;

    if (match(TokenType::Assign)) {
        node->value = expression();
        return node;
    }

    static const std::unordered_map<int, std::string> ops = {
        {(int)TokenType::PlusAssign, "+"},  {(int)TokenType::MinusAssign, "-"},
        {(int)TokenType::StarAssign, "*"},  {(int)TokenType::SlashAssign, "/"},
        {(int)TokenType::PercentAssign, "%"},
    };
    auto it = ops.find((int)peek().type);
    if (it == ops.end()) error(peek(), "expected '=' after '" + name.value + "'");
    const Token& opTok = advance();

    // x += e  ==>  x = x + (e)
    auto bin = std::make_unique<BinaryOp>(opTok.line);
    bin->op = it->second;
    auto lhs = std::make_unique<Identifier>(name.line);
    lhs->name = name.value;
    bin->left = std::move(lhs);
    bin->right = expression();
    node->value = std::move(bin);
    return node;
}

// `for (init; cond; step) { body }` is sugar for
// `{ init; while (cond) { body...; step; } }` — no new AST node is needed,
// so every later phase handles for-loops without changing at all.
NodePtr Parser::forStatement() {
    int ln = peek().line;
    advance();  // 'for'
    expect(TokenType::LParen, "'(' after 'for'");

    NodePtr init;
    if (isTypeStart())      init = varDecl();              // consumes its ';'
    else if (check(TokenType::Semicolon)) advance();       // empty initialiser
    else { init = assignmentNoSemi(); expect(TokenType::Semicolon, "';'"); }

    auto cond = expression();
    expect(TokenType::Semicolon, "';' after the loop condition");

    NodePtr step;
    if (!check(TokenType::RParen)) step = assignmentNoSemi();
    expect(TokenType::RParen, "')'");

    auto body = block();
    if (step) body->statements.push_back(std::move(step));

    auto loop = std::make_unique<While>(ln);
    loop->cond = std::move(cond);
    loop->body = std::move(body);

    auto wrapper = std::make_unique<Block>(ln);
    if (init) wrapper->statements.push_back(std::move(init));
    wrapper->statements.push_back(std::move(loop));
    return wrapper;
}

NodePtr Parser::whileStatement() {
    int ln = peek().line;
    advance();  // 'while'
    expect(TokenType::LParen, "'(' after 'while'");
    auto cond = expression();
    expect(TokenType::RParen, "')'");
    auto body = block();
    auto node = std::make_unique<While>(ln);
    node->cond = std::move(cond);
    node->body = std::move(body);
    return node;
}

NodePtr Parser::printStatement() {
    int ln = peek().line;
    advance();  // 'print'
    expect(TokenType::LParen, "'(' after 'print'");
    auto expr = expression();
    expect(TokenType::RParen, "')'");
    expect(TokenType::Semicolon, "';'");
    auto node = std::make_unique<Print>(ln);
    node->expr = std::move(expr);
    return node;
}

NodePtr Parser::returnStatement() {
    int ln = peek().line;
    advance();  // 'return'
    auto value = expression();
    expect(TokenType::Semicolon, "';'");
    auto node = std::make_unique<Return>(ln);
    node->value = std::move(value);
    return node;
}

NodePtr Parser::funcDecl() {
    int ln = peek().line;
    advance();  // 'func'
    const Token& name = expect(TokenType::Identifier, "a function name");
    expect(TokenType::LParen, "'(' after function name");

    auto node = std::make_unique<FuncDecl>(ln);
    node->name = name.value;
    if (!check(TokenType::RParen)) {
        do {
            std::string pty = typeName();
            const Token& pname = expect(TokenType::Identifier, "a parameter name");
            node->params.push_back(Param{pty, pname.value});
        } while (match(TokenType::Comma));
    }
    expect(TokenType::RParen, "')'");
    expect(TokenType::Colon, "':' before the return type");
    node->returnType = typeName();
    node->body = block();
    return node;
}

// ---- Expressions ---------------------------------------------------------
NodePtr Parser::expression() { return logicalOr(); }

NodePtr Parser::logicalOr() {
    auto left = logicalAnd();
    while (check(TokenType::OrOr)) {
        Token op = advance();
        auto right = logicalAnd();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::logicalAnd() {
    auto left = equality();
    while (check(TokenType::AndAnd)) {
        Token op = advance();
        auto right = equality();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::equality() {
    auto left = comparison();
    while (check(TokenType::EqEq) || check(TokenType::NotEq)) {
        Token op = advance();
        auto right = comparison();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::comparison() {
    auto left = term();
    while (check(TokenType::Lt) || check(TokenType::Gt) ||
           check(TokenType::Le) || check(TokenType::Ge)) {
        Token op = advance();
        auto right = term();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::term() {
    auto left = factor();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token op = advance();
        auto right = factor();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::factor() {
    auto left = unary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        Token op = advance();
        auto right = unary();
        auto node = std::make_unique<BinaryOp>(op.line);
        node->op = op.value; node->left = std::move(left); node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::unary() {
    if (check(TokenType::Not) || check(TokenType::Minus)) {
        Token op = advance();
        auto operand = unary();
        auto node = std::make_unique<UnaryOp>(op.line);
        node->op = op.value;
        node->operand = std::move(operand);
        return node;
    }
    return primary();
}

NodePtr Parser::finishCall(const Token& name) {
    expect(TokenType::LParen, "'('");
    auto call = std::make_unique<Call>(name.line);
    call->callee = name.value;
    if (!check(TokenType::RParen)) {
        do {
            call->args.push_back(expression());
        } while (match(TokenType::Comma));
    }
    expect(TokenType::RParen, "')'");
    return call;
}

NodePtr Parser::primary() {
    const Token& tok = peek();
    switch (tok.type) {
        case TokenType::IntLiteral: {
            advance();
            auto node = std::make_unique<IntLiteral>(tok.line);
            try {
                node->value = std::stoi(tok.value);
            } catch (const std::out_of_range&) {
                node->value = 0;
                errors_.add("syntax", "IntegerOverflow",
                            "Integer literal '" + tok.value + "' is too large for int.",
                            tok.line, tok.col, sourceLine(source_, tok.line));
            }
            return node;
        }
        case TokenType::StringLiteral: {
            advance();
            auto node = std::make_unique<StringLiteral>(tok.line);
            node->value = tok.value;
            return node;
        }
        case TokenType::KwTrue:
        case TokenType::KwFalse: {
            advance();
            auto node = std::make_unique<BoolLiteral>(tok.line);
            node->value = (tok.type == TokenType::KwTrue);
            return node;
        }
        case TokenType::Identifier: {
            advance();
            if (check(TokenType::LParen)) return finishCall(tok);
            auto node = std::make_unique<Identifier>(tok.line);
            node->name = tok.value;
            return node;
        }
        case TokenType::LParen: {
            advance();
            auto expr = expression();
            expect(TokenType::RParen, "')'");
            return expr;
        }
        default:
            error(tok, "expected an expression but found '" +
                       (tok.type == TokenType::EndOfFile ? "end of file" : tok.value) + "'");
    }
}

} // namespace lucid
