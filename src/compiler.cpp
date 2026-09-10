// ============================================================================
// compiler.cpp — Phase orchestrator for Lucid.
// Wires the five phases together and assembles the JSON the front-end renders.
// ============================================================================
#include "compiler.hpp"

#include "token.hpp"
#include "error.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "optimizer.hpp"
#include "interpreter.hpp"

namespace lucid {

nlohmann::json compile(const std::string& source) {
    using nlohmann::json;

    ErrorList errors;
    json result = {
        {"tokens", json::array()},
        {"ast", nullptr},
        {"symbolTable", nullptr},
        {"optimized", nullptr},
        {"output", json::array()},
        {"errors", json::array()},
        {"success", false}
    };

    // ---- Phase 1: Lexer ----
    Lexer lexer(source, errors);
    std::vector<Token> tokens = lexer.tokenize();
    for (const auto& t : tokens) result["tokens"].push_back(tokenToJson(t));
    if (!errors.empty()) { result["errors"] = errors.toJson(); return result; }

    // ---- Phase 2: Parser ----
    Parser parser(tokens, source, errors);
    std::unique_ptr<Program> program = parser.parse();
    result["ast"] = program->toJson();
    if (!errors.empty()) { result["errors"] = errors.toJson(); return result; }

    // ---- Phase 3: Semantic analysis ----
    SemanticAnalyzer sema(source, errors);
    result["symbolTable"] = sema.analyze(*program);
    if (!errors.empty()) { result["errors"] = errors.toJson(); return result; }

    // ---- Phase 4: Optimizer ----
    json astBefore = result["ast"];                 // pre-optimization tree
    int nodesBefore = countJsonNodes(astBefore);
    Optimizer optimizer;
    optimizer.optimize(*program);
    json astAfter = program->toJson();
    int nodesAfter = countJsonNodes(astAfter);
    result["optimized"] = {
        {"astBefore", astBefore},
        {"astAfter", astAfter},
        {"nodesBefore", nodesBefore},
        {"nodesAfter", nodesAfter},
        {"removed", nodesBefore - nodesAfter}
    };

    // ---- Phase 5: Interpreter (runs on the optimized tree) ----
    Interpreter interp(source, errors);
    std::vector<std::string> out = interp.run(*program);
    for (const auto& line : out) result["output"].push_back(line);

    result["errors"] = errors.toJson();
    result["success"] = errors.empty();
    return result;
}

} // namespace lucid
