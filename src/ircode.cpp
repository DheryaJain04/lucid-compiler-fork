// ============================================================================
// ircode.cpp — Intermediate code generation (three-address code).
// See ircode.hpp for the quadruple format.
// ============================================================================
#include "ircode.hpp"

namespace lucid {

std::string Quad::text() const {
    if (op == "=")       return result + " = " + arg1;
    if (op == "neg")     return result + " = -" + arg1;
    if (op == "!")       return result + " = !" + arg1;
    if (op == "label")   return result + ":";
    if (op == "goto")    return "goto " + result;
    if (op == "iffalse") return "iffalse " + arg1 + " goto " + result;
    if (op == "param")   return "param " + arg1;
    if (op == "recv")    return "recv " + result;
    if (op == "call")    return result + " = call " + arg1 + ", " + arg2;
    if (op == "ret")     return "return " + arg1;
    if (op == "print")   return "print " + arg1;
    if (op == "func")    return "func " + arg1 + ":";
    if (op == "endfunc") return "endfunc " + arg1;
    return result + " = " + arg1 + " " + op + " " + arg2;   // binary operator
}

std::vector<Quad> IRGenerator::generate(const Program& program) {
    code_.clear();
    temps_ = labels_ = 0;
    for (const auto& s : program.statements) genStmt(*s);
    return code_;
}

void IRGenerator::genBlock(const Block& b) {
    for (const auto& s : b.statements) genStmt(*s);
}

void IRGenerator::genStmt(const Node& n) {
    switch (n.kind) {
        case NodeKind::Block:
            genBlock(static_cast<const Block&>(n));
            break;

        case NodeKind::VarDecl: {
            const auto& d = static_cast<const VarDecl&>(n);
            emit("=", genExpr(*d.init), "", d.name);
            break;
        }
        case NodeKind::Assign: {
            const auto& a = static_cast<const Assign&>(n);
            emit("=", genExpr(*a.value), "", a.name);
            break;
        }
        case NodeKind::Print:
            emit("print", genExpr(*static_cast<const Print&>(n).expr));
            break;

        case NodeKind::Return:
            emit("ret", genExpr(*static_cast<const Return&>(n).value));
            break;

        // if (c) A else B   ==>   iffalse c goto Lelse ; A ; goto Lend ;
        //                          Lelse: B ; Lend:
        case NodeKind::If: {
            const auto& i = static_cast<const If&>(n);
            std::string c = genExpr(*i.cond);
            std::string lElse = newLabel();
            emit("iffalse", c, "", lElse);
            genBlock(*i.thenBlock);
            if (i.elseBlock) {
                std::string lEnd = newLabel();
                emit("goto", "", "", lEnd);
                emit("label", "", "", lElse);
                genBlock(*i.elseBlock);
                emit("label", "", "", lEnd);
            } else {
                emit("label", "", "", lElse);
            }
            break;
        }

        // while (c) B   ==>   Lstart: iffalse c goto Lend ; B ; goto Lstart ; Lend:
        case NodeKind::While: {
            const auto& w = static_cast<const While&>(n);
            std::string lStart = newLabel();
            std::string lEnd = newLabel();
            emit("label", "", "", lStart);
            std::string c = genExpr(*w.cond);
            emit("iffalse", c, "", lEnd);
            genBlock(*w.body);
            emit("goto", "", "", lStart);
            emit("label", "", "", lEnd);
            break;
        }

        case NodeKind::FuncDecl: {
            const auto& f = static_cast<const FuncDecl&>(n);
            emit("func", f.name);
            for (const auto& p : f.params) emit("recv", "", "", p.name);
            genBlock(*f.body);
            emit("endfunc", f.name);
            break;
        }

        // A call used as a statement: evaluate it, discard the result.
        case NodeKind::Call:
            genExpr(n);
            break;

        default:
            break;
    }
}

std::string IRGenerator::genExpr(const Node& n) {
    switch (n.kind) {
        case NodeKind::IntLiteral:
            return std::to_string(static_cast<const IntLiteral&>(n).value);
        case NodeKind::StringLiteral:
            return "\"" + static_cast<const StringLiteral&>(n).value + "\"";
        case NodeKind::BoolLiteral:
            return static_cast<const BoolLiteral&>(n).value ? "true" : "false";
        case NodeKind::Identifier:
            return static_cast<const Identifier&>(n).name;

        case NodeKind::BinaryOp: {
            const auto& b = static_cast<const BinaryOp&>(n);
            std::string l = genExpr(*b.left);
            std::string r = genExpr(*b.right);
            std::string t = newTemp();
            emit(b.op, l, r, t);
            return t;
        }
        case NodeKind::UnaryOp: {
            const auto& u = static_cast<const UnaryOp&>(n);
            std::string a = genExpr(*u.operand);
            std::string t = newTemp();
            emit(u.op == "-" ? "neg" : "!", a, "", t);
            return t;
        }
        case NodeKind::Call: {
            const auto& c = static_cast<const Call&>(n);
            std::vector<std::string> args;
            for (const auto& a : c.args) args.push_back(genExpr(*a));
            for (const auto& a : args) emit("param", a);   // params after all are evaluated
            std::string t = newTemp();
            emit("call", c.callee, std::to_string(args.size()), t);
            return t;
        }
        default:
            return "?";
    }
}

nlohmann::json quadsToJson(const std::vector<Quad>& code) {
    nlohmann::json arr = nlohmann::json::array();
    for (size_t i = 0; i < code.size(); ++i) {
        const auto& q = code[i];
        arr.push_back({{"n", i}, {"op", q.op}, {"arg1", q.arg1},
                       {"arg2", q.arg2}, {"result", q.result}, {"text", q.text()}});
    }
    return arr;
}

} // namespace lucid
