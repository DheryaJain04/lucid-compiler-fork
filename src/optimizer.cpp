// ============================================================================
// optimizer.cpp — Phase 4 of Lucid.  Constant folding + dead code elimination.
// Runs after semantic analysis (so the tree is well-typed) and before the
// interpreter (so it executes the smaller, equivalent program).
// ============================================================================
#include "optimizer.hpp"

namespace lucid {

// ---- literal helpers -----------------------------------------------------
static bool isInt(const Node* n)  { return n && n->kind == NodeKind::IntLiteral; }
static bool isBool(const Node* n) { return n && n->kind == NodeKind::BoolLiteral; }
static bool isStr(const Node* n)  { return n && n->kind == NodeKind::StringLiteral; }

static int  intVal(const Node* n)  { return static_cast<const IntLiteral*>(n)->value; }
static bool boolVal(const Node* n) { return static_cast<const BoolLiteral*>(n)->value; }
static const std::string& strVal(const Node* n) { return static_cast<const StringLiteral*>(n)->value; }

static NodePtr makeInt(int v, int line) {
    auto n = std::make_unique<IntLiteral>(line); n->value = v; return n;
}
static NodePtr makeBool(bool v, int line) {
    auto n = std::make_unique<BoolLiteral>(line); n->value = v; return n;
}
static NodePtr makeStr(std::string v, int line) {
    auto n = std::make_unique<StringLiteral>(line); n->value = std::move(v); return n;
}

// ---- entry ---------------------------------------------------------------
void Optimizer::optimize(Program& program) {
    optStmtList(program.statements);
}

void Optimizer::optStmtList(std::vector<NodePtr>& stmts) {
    std::vector<NodePtr> out;
    for (auto& s : stmts) {
        NodePtr opt = optStmt(std::move(s));
        if (!opt) continue;                       // dropped (dead code)
        bool isReturn = (opt->kind == NodeKind::Return);
        out.push_back(std::move(opt));
        if (isReturn) break;                      // everything after return is unreachable
    }
    stmts = std::move(out);
}

NodePtr Optimizer::optStmt(NodePtr stmt) {
    switch (stmt->kind) {
        case NodeKind::VarDecl: {
            auto* d = static_cast<VarDecl*>(stmt.get());
            d->init = foldExpr(std::move(d->init));
            return stmt;
        }
        case NodeKind::Assign: {
            auto* a = static_cast<Assign*>(stmt.get());
            a->value = foldExpr(std::move(a->value));
            return stmt;
        }
        case NodeKind::Print: {
            auto* p = static_cast<Print*>(stmt.get());
            p->expr = foldExpr(std::move(p->expr));
            return stmt;
        }
        case NodeKind::Return: {
            auto* r = static_cast<Return*>(stmt.get());
            r->value = foldExpr(std::move(r->value));
            return stmt;
        }
        case NodeKind::If: {
            auto* i = static_cast<If*>(stmt.get());
            i->cond = foldExpr(std::move(i->cond));
            optStmtList(i->thenBlock->statements);
            if (i->elseBlock) optStmtList(i->elseBlock->statements);

            if (isBool(i->cond.get())) {           // condition is a known constant
                if (boolVal(i->cond.get()))
                    return std::move(i->thenBlock);       // keep only the taken branch
                if (i->elseBlock)
                    return std::move(i->elseBlock);
                return nullptr;                            // if(false) with no else -> gone
            }
            return stmt;
        }
        case NodeKind::While: {
            auto* w = static_cast<While*>(stmt.get());
            w->cond = foldExpr(std::move(w->cond));
            optStmtList(w->body->statements);
            if (isBool(w->cond.get()) && !boolVal(w->cond.get()))
                return nullptr;                            // while(false) never runs
            return stmt;
        }
        case NodeKind::FuncDecl: {
            auto* fn = static_cast<FuncDecl*>(stmt.get());
            optStmtList(fn->body->statements);
            return stmt;
        }
        case NodeKind::Block: {
            auto* b = static_cast<Block*>(stmt.get());
            optStmtList(b->statements);
            return stmt;
        }
        case NodeKind::Call: {
            auto* c = static_cast<Call*>(stmt.get());
            for (auto& a : c->args) a = foldExpr(std::move(a));
            return stmt;
        }
        default:
            return stmt;
    }
}

NodePtr Optimizer::foldExpr(NodePtr expr) {
    if (!expr) return expr;

    if (expr->kind == NodeKind::UnaryOp) {
        auto* u = static_cast<UnaryOp*>(expr.get());
        u->operand = foldExpr(std::move(u->operand));
        Node* o = u->operand.get();
        if (u->op == "!" && isBool(o)) return makeBool(!boolVal(o), u->line);
        if (u->op == "-" && isInt(o))  return makeInt(-intVal(o), u->line);
        return expr;
    }

    if (expr->kind == NodeKind::Call) {
        auto* c = static_cast<Call*>(expr.get());
        for (auto& a : c->args) a = foldExpr(std::move(a));
        return expr;
    }

    if (expr->kind != NodeKind::BinaryOp) return expr;   // literals, identifiers

    auto* b = static_cast<BinaryOp*>(expr.get());
    b->left  = foldExpr(std::move(b->left));
    b->right = foldExpr(std::move(b->right));
    const std::string& op = b->op;
    Node* L = b->left.get();
    Node* R = b->right.get();
    int line = b->line;

    // Boolean short-circuit folding on a constant LEFT operand (side-effect safe,
    // because && / || never evaluate the right side when the left decides it).
    if ((op == "&&" || op == "||") && isBool(L)) {
        bool lv = boolVal(L);
        if (op == "&&") return lv ? std::move(b->right) : makeBool(false, line);
        else            return lv ? makeBool(true, line) : std::move(b->right);
    }

    // Integer arithmetic on two int literals.
    if (isInt(L) && isInt(R)) {
        int a = intVal(L), c = intVal(R);
        if (op == "+") return makeInt(a + c, line);
        if (op == "-") return makeInt(a - c, line);
        if (op == "*") return makeInt(a * c, line);
        if (op == "/") { if (c == 0) return expr; return makeInt(a / c, line); }  // keep 1/0 for runtime
        if (op == "%") { if (c == 0) return expr; return makeInt(a % c, line); }
        if (op == "<")  return makeBool(a <  c, line);
        if (op == ">")  return makeBool(a >  c, line);
        if (op == "<=") return makeBool(a <= c, line);
        if (op == ">=") return makeBool(a >= c, line);
        if (op == "==") return makeBool(a == c, line);
        if (op == "!=") return makeBool(a != c, line);
    }

    // String concatenation / comparison on two string literals.
    if (isStr(L) && isStr(R)) {
        if (op == "+")  return makeStr(strVal(L) + strVal(R), line);
        if (op == "==") return makeBool(strVal(L) == strVal(R), line);
        if (op == "!=") return makeBool(strVal(L) != strVal(R), line);
    }

    // Boolean literal comparison / logic on two bool literals.
    if (isBool(L) && isBool(R)) {
        bool a = boolVal(L), c = boolVal(R);
        if (op == "==") return makeBool(a == c, line);
        if (op == "!=") return makeBool(a != c, line);
        if (op == "&&") return makeBool(a && c, line);
        if (op == "||") return makeBool(a || c, line);
    }

    return expr;
}

} // namespace lucid
