// ============================================================================
// interpreter.cpp — Phase 5 of Lucid.  Tree-walking evaluator.
// Executes the (optimized, already type-checked) AST and collects print output.
// ============================================================================
#include "interpreter.hpp"

namespace lucid {

std::string valueToString(const Value& v) {
    if (std::holds_alternative<int>(v))         return std::to_string(std::get<int>(v));
    if (std::holds_alternative<bool>(v))        return std::get<bool>(v) ? "true" : "false";
    return std::get<std::string>(v);
}

int Interpreter::asInt(const Value& v) { return std::get<int>(v); }
bool Interpreter::asBool(const Value& v) { return std::get<bool>(v); }
const std::string& Interpreter::asStr(const Value& v) { return std::get<std::string>(v); }

Interpreter::Interpreter(const std::string& source, ErrorList& errors)
    : source_(source), errors_(errors) {}

void Interpreter::define(const std::string& name, const Value& v) {
    scopes_.back()[name] = v;
}

bool Interpreter::assign(const std::string& name, const Value& v) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) { found->second = v; return true; }
    }
    return false;
}

bool Interpreter::lookup(const std::string& name, Value& out) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) { out = found->second; return true; }
    }
    return false;
}

void Interpreter::spendStep(int line) {
    if (++steps_ > kMaxSteps)
        throw RuntimeFault{ "Stopped after 2,000,000 steps. Is there an infinite loop, "
                            "or a loop variable that never changes?", line };
}

std::vector<std::string> Interpreter::run(Program& program) {
    output_.clear();
    steps_ = 0;
    callDepth_ = 0;
    scopes_.clear();
    scopes_.emplace_back();   // global scope

    // Register all top-level functions first (forward references allowed).
    for (auto& stmt : program.statements)
        if (stmt->kind == NodeKind::FuncDecl)
            funcs_[static_cast<FuncDecl*>(stmt.get())->name] = static_cast<FuncDecl*>(stmt.get());

    try {
        for (auto& stmt : program.statements) {
            if (stmt->kind == NodeKind::FuncDecl) continue;  // definitions, not executed here
            execStatement(stmt.get());
        }
    } catch (const RuntimeFault& f) {
        errors_.add("runtime", "RuntimeError", f.message, f.line, 0, sourceLine(source_, f.line));
    } catch (const ReturnSignal&) {
        // A stray top-level return — semantic analysis already flagged it.
    }
    return output_;
}

void Interpreter::execBlock(Block* block) {
    scopes_.emplace_back();
    struct Pop { std::vector<Scope>& s; ~Pop(){ s.pop_back(); } } pop{scopes_};
    for (auto& s : block->statements) execStatement(s.get());
}

void Interpreter::execStatement(Node* stmt) {
    switch (stmt->kind) {
        case NodeKind::VarDecl: {
            auto* d = static_cast<VarDecl*>(stmt);
            define(d->name, evalExpr(d->init.get()));
            break;
        }
        case NodeKind::Assign: {
            auto* a = static_cast<Assign*>(stmt);
            assign(a->name, evalExpr(a->value.get()));
            break;
        }
        case NodeKind::If: {
            auto* i = static_cast<If*>(stmt);
            if (asBool(evalExpr(i->cond.get())))
                execBlock(i->thenBlock.get());
            else if (i->elseBlock)
                execBlock(i->elseBlock.get());
            break;
        }
        case NodeKind::While: {
            auto* w = static_cast<While*>(stmt);
            while (asBool(evalExpr(w->cond.get()))) {
                spendStep(w->line);
                execBlock(w->body.get());
            }
            break;
        }
        case NodeKind::Print: {
            auto* p = static_cast<Print*>(stmt);
            if (output_.size() >= kMaxOutputLines)
                throw RuntimeFault{ "Output limit reached (10,000 lines). Is a loop printing forever?", p->line };
            output_.push_back(valueToString(evalExpr(p->expr.get())));
            break;
        }
        case NodeKind::Return: {
            auto* r = static_cast<Return*>(stmt);
            throw ReturnSignal{ evalExpr(r->value.get()) };
        }
        case NodeKind::Block:
            // The optimizer may replace an if(true){...} with its bare block.
            execBlock(static_cast<Block*>(stmt));
            break;
        case NodeKind::FuncDecl:
            break;   // nested definitions: skip (registered at top level)
        case NodeKind::Call:
            evalExpr(stmt);   // bare call statement, discard result
            break;
        default:
            evalExpr(stmt);
            break;
    }
}

Value Interpreter::evalExpr(Node* expr) {
    switch (expr->kind) {
        case NodeKind::IntLiteral:    return Value{ static_cast<IntLiteral*>(expr)->value };
        case NodeKind::StringLiteral: return Value{ static_cast<StringLiteral*>(expr)->value };
        case NodeKind::BoolLiteral:   return Value{ static_cast<BoolLiteral*>(expr)->value };
        case NodeKind::Identifier: {
            auto* id = static_cast<Identifier*>(expr);
            Value v;
            if (lookup(id->name, v)) return v;
            throw RuntimeFault{ "Undeclared variable '" + id->name + "' at runtime.", id->line };
        }
        case NodeKind::UnaryOp: {
            auto* u = static_cast<UnaryOp*>(expr);
            Value operand = evalExpr(u->operand.get());
            if (u->op == "!") return Value{ !asBool(operand) };
            return Value{ -asInt(operand) };   // unary '-'
        }
        case NodeKind::BinaryOp: return evalBinary(static_cast<BinaryOp*>(expr));
        case NodeKind::Call:     return callFunction(static_cast<Call*>(expr));
        default:
            throw RuntimeFault{ "Cannot evaluate expression.", expr->line };
    }
}

Value Interpreter::evalBinary(BinaryOp* bin) {
    const std::string& op = bin->op;

    // Short-circuit boolean operators.
    if (op == "&&") {
        if (!asBool(evalExpr(bin->left.get()))) return Value{ false };
        return Value{ asBool(evalExpr(bin->right.get())) };
    }
    if (op == "||") {
        if (asBool(evalExpr(bin->left.get()))) return Value{ true };
        return Value{ asBool(evalExpr(bin->right.get())) };
    }

    Value l = evalExpr(bin->left.get());
    Value r = evalExpr(bin->right.get());

    if (op == "+") {
        if (std::holds_alternative<std::string>(l))
            return Value{ asStr(l) + asStr(r) };
        return Value{ asInt(l) + asInt(r) };
    }
    if (op == "-") return Value{ asInt(l) - asInt(r) };
    if (op == "*") return Value{ asInt(l) * asInt(r) };
    if (op == "/") {
        int d = asInt(r);
        if (d == 0) throw RuntimeFault{ "Division by zero.", bin->line };
        return Value{ asInt(l) / d };
    }
    if (op == "%") {
        int d = asInt(r);
        if (d == 0) throw RuntimeFault{ "Modulo by zero.", bin->line };
        return Value{ asInt(l) % d };
    }
    if (op == "<")  return Value{ asInt(l) <  asInt(r) };
    if (op == ">")  return Value{ asInt(l) >  asInt(r) };
    if (op == "<=") return Value{ asInt(l) <= asInt(r) };
    if (op == ">=") return Value{ asInt(l) >= asInt(r) };
    if (op == "==") return Value{ l == r };
    if (op == "!=") return Value{ l != r };

    throw RuntimeFault{ "Unknown operator '" + op + "'.", bin->line };
}

Value Interpreter::callFunction(Call* call) {
    auto it = funcs_.find(call->callee);
    if (it == funcs_.end())
        throw RuntimeFault{ "Call to undefined function '" + call->callee + "'.", call->line };
    FuncDecl* fn = it->second;

    // Evaluate arguments in the *caller's* scope first.
    std::vector<Value> args;
    args.reserve(call->args.size());
    for (auto& a : call->args) args.push_back(evalExpr(a.get()));

    spendStep(call->line);
    if (++callDepth_ > kMaxCallDepth) {
        --callDepth_;
        throw RuntimeFault{ "Call stack overflow (runaway recursion?).", call->line };
    }

    // Functions run in an isolated scope chain: only params are visible.
    auto savedScopes = std::move(scopes_);
    scopes_.clear();
    scopes_.emplace_back();
    for (size_t i = 0; i < fn->params.size() && i < args.size(); ++i)
        define(fn->params[i].name, args[i]);

    Value result;
    bool returned = false;
    try {
        for (auto& s : fn->body->statements) execStatement(s.get());
    } catch (const ReturnSignal& rs) {
        result = rs.value;
        returned = true;
    }

    scopes_ = std::move(savedScopes);
    --callDepth_;

    if (returned) return result;

    // Fell off the end without returning: hand back a type-appropriate default.
    if (fn->returnType == "int")    return Value{ 0 };
    if (fn->returnType == "bool")   return Value{ false };
    return Value{ std::string("") };
}

} // namespace lucid
