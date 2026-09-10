// ============================================================================
// compiler.hpp — Phase orchestrator for Lucid.
// Input : source text
// Output: one JSON object describing every phase:
//   { tokens, ast, symbolTable,
//     optimized: { astBefore, astAfter, nodesBefore, nodesAfter },
//     output, errors, success }
//
// Runs Lexer -> Parser -> SemanticAnalyzer -> Optimizer -> Interpreter in
// order. If a phase reports errors, the later phases are skipped but whatever
// was produced so far is still returned.
// ============================================================================
#pragma once

#include <string>
#include "../third_party/json.hpp"

namespace lucid {

// Compile `source` and return the full result document (never throws for
// ordinary program errors — those live in the "errors" array).
nlohmann::json compile(const std::string& source);

} // namespace lucid
