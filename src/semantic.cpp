// ============================================================================
// semantic.cpp — Phase 3 of Lucid.  Type checking + symbol table.
// Reports all semantic errors in a single pass. See error taxonomy in the
// build brief: undeclared/redeclared variables, type mismatches, non-boolean
// conditions, unknown functions, wrong argument count/types, return mismatch.
// ============================================================================
#include "semantic.hpp"
#include <algorithm>

namespace lucid {

const char* tyName(Ty t) {
    switch (t) {
        case Ty::Int:    return "int";
        case Ty::String: return "string";
        case Ty::Bool:   return "bool";
        case Ty::Error:  return "<error>";
    }
    return "<error>";
}

Ty SemanticAnalyzer::tyFromName(const std::string& name) {
    if (name == "int")    return Ty::Int;
    if (name == "string") return Ty::String;
    if (name == "bool")   return Ty::Bool;
    return Ty::Error;
}

// ---- Levenshtein edit distance + nearest-name suggestion -----------------
int levenshtein(const std::string& a, const std::string& b) {
    const size_t n = a.size(), m = b.size();
    std::vector<int> prev(m + 1), cur(m + 1);
    for (size_t j = 0; j <= m; ++j) prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= n; ++i) {
        cur[0] = static_cast<int>(i);
        for (size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = std::min({ prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost });
        }
        std::swap(prev, cur);
    }
    return prev[m];
}

std::string suggestName(const std::string& target, const std::vector<std::string>& candidates) {
    std::string best;
    int bestDist = 1 << 30;
    for (const auto& c : candidates) {
        if (c == target) continue;
        int d = levenshtein(target, c);
        if (d < bestDist) { bestDist = d; best = c; }
    }
    // Only suggest when the names are genuinely close.
    int threshold = std::max(2, static_cast<int>(target.size()) / 3);
    if (!best.empty() && bestDist <= threshold) return best;
    return "";
}

// ---- setup ---------------------------------------------------------------
SemanticAnalyzer::SemanticAnalyzer(const std::string& source, ErrorList& errors)
    : source_(source), errors_(errors) {}

void SemanticAnalyzer::reportTypeError(const std::string& type, const std::string& msg, int line) {
    errors_.add("semantic", type, msg, line, 0, sourceLine(source_, line));
}

void SemanticAnalyzer::pushScope(const std::string& label) {
    scopes_.emplace_back();
    scopeLabels_.push_back(label);
}

void SemanticAnalyzer::popScope() {
    scopes_.pop_back();
    scopeLabels_.pop_back();
}

bool SemanticAnalyzer::declareVar(const std::string& name, Ty type) {
    auto& scope = scopes_.back();
    if (scope.count(name)) return false;   // already declared in this scope
    scope[name] = VarEntry{type};
    return true;
}

bool SemanticAnalyzer::lookupVar(const std::string& name, Ty& out) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) { out = found->second.type; return true; }
    }
    return false;
}

std::vector<std::string> SemanticAnalyzer::knownVarNames() const {
    std::vector<std::string> names;
    for (const auto& scope : scopes_)
        for (const auto& kv : scope) names.push_back(kv.first);
    return names;
}

// ---- top level -----------------------------------------------------------
nlohmann::json SemanticAnalyzer::analyze(Program& program) {
    pushScope("global");

    // Pass 1: hoist top-level function signatures so calls may precede decls.
    for (auto& stmt : program.statements) {
        if (stmt->kind == NodeKind::FuncDecl) {
            auto* fn = static_cast<FuncDecl*>(stmt.get());
            if (funcs_.count(fn->name)) {
                reportTypeError("Redeclaration",
                    "Function '" + fn->name + "' is already defined.", fn->line);
                continue;
            }
            FuncSig sig;
            for (const auto& p : fn->params) {
                sig.paramTypes.push_back(tyFromName(p.type));
                sig.paramNames.push_back(p.name);
            }
            sig.returnType = tyFromName(fn->returnType);
            sig.returnTypeName = fn->returnType;
            sig.line = fn->line;
            funcs_[fn->name] = sig;

            nlohmann::json params = nlohmann::json::array();
            for (const auto& p : fn->params)
                params.push_back({{"name", p.name}, {"type", p.type}});
            functionsSnapshot_.push_back({
                {"name", fn->name},
                {"returnType", fn->returnType},
                {"params", params},
                {"signature", fn->signature()},
                {"scope", "global"},
                {"line", fn->line}
            });
        }
    }

    // Pass 2: check every statement (function bodies included).
    for (auto& stmt : program.statements) checkStatement(stmt.get());

    popScope();

    return nlohmann::json{
        {"variables", variablesSnapshot_},
        {"functions", functionsSnapshot_}
    };
}

void SemanticAnalyzer::checkBlock(Block* block, bool newScope, const std::string& label) {
    if (newScope) pushScope(label);
    for (auto& s : block->statements) checkStatement(s.get());
    if (newScope) popScope();
}

void SemanticAnalyzer::checkFuncBody(FuncDecl* fn) {
    // Functions get an ISOLATED scope chain: only their parameters (and other
    // functions, via funcs_) are visible — never global or enclosing locals.
    // This mirrors how the interpreter scopes a call, so a program that passes
    // the type check will not hit a surprise "undeclared" at runtime.
    auto savedScopes = std::move(scopes_);
    auto savedLabels = std::move(scopeLabels_);
    scopes_.clear();
    scopeLabels_.clear();

    pushScope("func " + fn->name);
    for (const auto& p : fn->params) {
        Ty pty = tyFromName(p.type);
        if (!declareVar(p.name, pty)) {
            reportTypeError("Redeclaration",
                "Parameter '" + p.name + "' is declared twice in '" + fn->name + "'.", fn->line);
        }
        variablesSnapshot_.push_back({
            {"name", p.name}, {"type", p.type},
            {"scope", "func " + fn->name}, {"kind", "parameter"}, {"line", fn->line}
        });
    }
    returnStack_.push_back(tyFromName(fn->returnType));
    // Body reuses the function scope (params + locals share it).
    checkBlock(fn->body.get(), /*newScope=*/false, "");
    returnStack_.pop_back();
    popScope();

    scopes_ = std::move(savedScopes);
    scopeLabels_ = std::move(savedLabels);
}

void SemanticAnalyzer::checkStatement(Node* stmt) {
    switch (stmt->kind) {
        case NodeKind::VarDecl: {
            auto* d = static_cast<VarDecl*>(stmt);
            Ty declTy = tyFromName(d->declType);
            Ty initTy = checkExpr(d->init.get());
            if (initTy != Ty::Error && initTy != declTy) {
                reportTypeError("TypeMismatch",
                    "Cannot initialize '" + d->name + "' of type " + d->declType +
                    " with a value of type " + tyName(initTy) + ".", d->line);
            }
            if (!declareVar(d->name, declTy)) {
                reportTypeError("Redeclaration",
                    "Variable '" + d->name + "' is already declared in this scope.", d->line);
            }
            variablesSnapshot_.push_back({
                {"name", d->name}, {"type", d->declType},
                {"scope", scopeLabels_.back()}, {"kind", "variable"}, {"line", d->line}
            });
            break;
        }
        case NodeKind::Assign: {
            auto* a = static_cast<Assign*>(stmt);
            Ty varTy;
            if (!lookupVar(a->name, varTy)) {
                std::string hint = suggestName(a->name, knownVarNames());
                std::string msg = "Assignment to undeclared variable '" + a->name + "'.";
                if (!hint.empty()) msg += " Did you mean '" + hint + "'?";
                reportTypeError("UndeclaredVariable", msg, a->line);
                varTy = Ty::Error;
            }
            Ty valTy = checkExpr(a->value.get());
            if (varTy != Ty::Error && valTy != Ty::Error && varTy != valTy) {
                reportTypeError("TypeMismatch",
                    "Cannot assign a value of type " + std::string(tyName(valTy)) +
                    " to '" + a->name + "' of type " + tyName(varTy) + ".", a->line);
            }
            break;
        }
        case NodeKind::If: {
            auto* i = static_cast<If*>(stmt);
            Ty c = checkExpr(i->cond.get());
            if (c != Ty::Error && c != Ty::Bool)
                reportTypeError("ConditionNotBoolean",
                    "The condition of 'if' must be bool, but it is " + std::string(tyName(c)) + ".", i->line);
            checkBlock(i->thenBlock.get(), true, "if");
            if (i->elseBlock) checkBlock(i->elseBlock.get(), true, "else");
            break;
        }
        case NodeKind::While: {
            auto* w = static_cast<While*>(stmt);
            Ty c = checkExpr(w->cond.get());
            if (c != Ty::Error && c != Ty::Bool)
                reportTypeError("ConditionNotBoolean",
                    "The condition of 'while' must be bool, but it is " + std::string(tyName(c)) + ".", w->line);
            checkBlock(w->body.get(), true, "while");
            break;
        }
        case NodeKind::Print: {
            auto* p = static_cast<Print*>(stmt);
            checkExpr(p->expr.get());   // any well-typed value may be printed
            break;
        }
        case NodeKind::Return: {
            auto* r = static_cast<Return*>(stmt);
            Ty valTy = checkExpr(r->value.get());
            if (returnStack_.empty()) {
                reportTypeError("ReturnOutsideFunction",
                    "'return' can only appear inside a function.", r->line);
            } else {
                Ty want = returnStack_.back();
                if (valTy != Ty::Error && want != Ty::Error && valTy != want)
                    reportTypeError("ReturnTypeMismatch",
                        "Function should return " + std::string(tyName(want)) +
                        " but this returns " + tyName(valTy) + ".", r->line);
            }
            break;
        }
        case NodeKind::FuncDecl: {
            checkFuncBody(static_cast<FuncDecl*>(stmt));
            break;
        }
        case NodeKind::Call: {
            checkCall(static_cast<Call*>(stmt));   // bare call statement
            break;
        }
        default:
            checkExpr(stmt);   // any stray expression
            break;
    }
}

// ---- expression typing ---------------------------------------------------
Ty SemanticAnalyzer::checkExpr(Node* expr) {
    if (!expr) return Ty::Error;
    switch (expr->kind) {
        case NodeKind::IntLiteral:    return Ty::Int;
        case NodeKind::StringLiteral: return Ty::String;
        case NodeKind::BoolLiteral:   return Ty::Bool;
        case NodeKind::Identifier: {
            auto* id = static_cast<Identifier*>(expr);
            Ty t;
            if (lookupVar(id->name, t)) return t;
            std::string hint = suggestName(id->name, knownVarNames());
            std::string msg = "Use of undeclared variable '" + id->name + "'.";
            if (!hint.empty()) msg += " Did you mean '" + hint + "'?";
            reportTypeError("UndeclaredVariable", msg, id->line);
            return Ty::Error;
        }
        case NodeKind::UnaryOp: {
            auto* u = static_cast<UnaryOp*>(expr);
            Ty t = checkExpr(u->operand.get());
            if (t == Ty::Error) return Ty::Error;
            if (u->op == "!") {
                if (t != Ty::Bool) {
                    reportTypeError("TypeMismatch",
                        "Operator '!' needs a bool but got " + std::string(tyName(t)) + ".", u->line);
                    return Ty::Error;
                }
                return Ty::Bool;
            }
            // unary '-'
            if (t != Ty::Int) {
                reportTypeError("TypeMismatch",
                    "Unary '-' needs an int but got " + std::string(tyName(t)) + ".", u->line);
                return Ty::Error;
            }
            return Ty::Int;
        }
        case NodeKind::BinaryOp: return checkBinary(static_cast<BinaryOp*>(expr));
        case NodeKind::Call:     return checkCall(static_cast<Call*>(expr));
        default:                 return Ty::Error;
    }
}

Ty SemanticAnalyzer::checkBinary(BinaryOp* bin) {
    Ty l = checkExpr(bin->left.get());
    Ty r = checkExpr(bin->right.get());
    if (l == Ty::Error || r == Ty::Error) return Ty::Error;

    const std::string& op = bin->op;

    // '+' : int addition OR string concatenation.
    if (op == "+") {
        if (l == Ty::Int && r == Ty::Int) return Ty::Int;
        if (l == Ty::String && r == Ty::String) return Ty::String;
        reportTypeError("TypeMismatch",
            "Operator '+' needs two ints or two strings, not " +
            std::string(tyName(l)) + " and " + tyName(r) + ".", bin->line);
        return Ty::Error;
    }
    // other arithmetic: int only
    if (op == "-" || op == "*" || op == "/" || op == "%") {
        if (l == Ty::Int && r == Ty::Int) return Ty::Int;
        reportTypeError("TypeMismatch",
            "Operator '" + op + "' needs two ints, not " +
            std::string(tyName(l)) + " and " + tyName(r) + ".", bin->line);
        return Ty::Error;
    }
    // ordering comparisons: int only, produce bool
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (l == Ty::Int && r == Ty::Int) return Ty::Bool;
        reportTypeError("TypeMismatch",
            "Operator '" + op + "' needs two ints, not " +
            std::string(tyName(l)) + " and " + tyName(r) + ".", bin->line);
        return Ty::Error;
    }
    // equality: same type on both sides, produce bool
    if (op == "==" || op == "!=") {
        if (l == r) return Ty::Bool;
        reportTypeError("TypeMismatch",
            "Operator '" + op + "' compares values of the same type, not " +
            std::string(tyName(l)) + " and " + tyName(r) + ".", bin->line);
        return Ty::Error;
    }
    // boolean logic: bool only
    if (op == "&&" || op == "||") {
        if (l == Ty::Bool && r == Ty::Bool) return Ty::Bool;
        reportTypeError("TypeMismatch",
            "Operator '" + op + "' needs two bools, not " +
            std::string(tyName(l)) + " and " + tyName(r) + ".", bin->line);
        return Ty::Error;
    }
    return Ty::Error;
}

Ty SemanticAnalyzer::checkCall(Call* call) {
    // Type each argument first (also surfaces nested errors).
    std::vector<Ty> argTypes;
    argTypes.reserve(call->args.size());
    for (auto& a : call->args) argTypes.push_back(checkExpr(a.get()));

    auto it = funcs_.find(call->callee);
    if (it == funcs_.end()) {
        std::vector<std::string> names;
        for (const auto& kv : funcs_) names.push_back(kv.first);
        std::string hint = suggestName(call->callee, names);
        std::string msg = "Call to undefined function '" + call->callee + "'.";
        if (!hint.empty()) msg += " Did you mean '" + hint + "'?";
        reportTypeError("UndefinedFunction", msg, call->line);
        return Ty::Error;
    }
    const FuncSig& sig = it->second;
    if (argTypes.size() != sig.paramTypes.size()) {
        reportTypeError("ArgumentCount",
            "Function '" + call->callee + "' expects " +
            std::to_string(sig.paramTypes.size()) + " argument(s) but got " +
            std::to_string(argTypes.size()) + ".", call->line);
        return sig.returnType;
    }
    for (size_t i = 0; i < argTypes.size(); ++i) {
        if (argTypes[i] != Ty::Error && argTypes[i] != sig.paramTypes[i]) {
            reportTypeError("ArgumentType",
                "Argument " + std::to_string(i + 1) + " of '" + call->callee +
                "' should be " + tyName(sig.paramTypes[i]) + " but got " +
                tyName(argTypes[i]) + ".", call->line);
        }
    }
    return sig.returnType;
}

} // namespace lucid
