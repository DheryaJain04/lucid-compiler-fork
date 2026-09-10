// ============================================================================
// main_cli.cpp — command-line front-end for Lucid (`lucidc`).
// Reads a .lucid file, runs the full compiler in-process, and prints tokens,
// a readable AST dump, the symbol table, optimizer before/after counts, any
// errors (with line + snippet), and the program output. Pure C++: no browser,
// no network — this is the deterministic core's proof and fallback demo.
// ============================================================================
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "compiler.hpp"
#include "../third_party/json.hpp"

using nlohmann::json;

static void printAst(const json& node, const std::string& prefix, bool last, bool root) {
    std::cout << prefix;
    if (!root) std::cout << (last ? "└─ " : "├─ ");
    std::cout << node.value("label", "?") << "  (line " << node.value("line", 0) << ")\n";
    if (!node.contains("children")) return;
    const auto& kids = node["children"];
    std::string childPrefix = prefix;
    if (!root) childPrefix += (last ? "   " : "│  ");
    for (size_t i = 0; i < kids.size(); ++i)
        printAst(kids[i], childPrefix, i + 1 == kids.size(), false);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: lucidc <program.lucid>\n";
        return 2;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "error: cannot open '" << argv[1] << "'\n";
        return 2;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string source = buffer.str();

    json result = lucid::compile(source);

    std::cout << "=== Lucid ===  " << argv[1] << "\n\n";

    // --- Tokens ---
    std::cout << "--- Tokens (" << result["tokens"].size() << ") ---\n";
    for (const auto& t : result["tokens"]) {
        std::cout << "  " << t.value("line", 0) << ":" << t.value("col", 0)
                  << "  " << t.value("type", "") << "  '" << t.value("value", "") << "'\n";
    }
    std::cout << "\n";

    // --- AST ---
    std::cout << "--- AST ---\n";
    if (!result["ast"].is_null()) printAst(result["ast"], "", true, true);
    else std::cout << "  (not built — earlier phase failed)\n";
    std::cout << "\n";

    // --- Symbol table ---
    std::cout << "--- Symbol Table ---\n";
    if (!result["symbolTable"].is_null()) {
        const auto& st = result["symbolTable"];
        std::cout << "  Variables:\n";
        for (const auto& v : st.value("variables", json::array()))
            std::cout << "    " << v.value("name", "") << " : " << v.value("type", "")
                      << "  [" << v.value("kind", "variable") << " @ " << v.value("scope", "") << "]\n";
        std::cout << "  Functions:\n";
        for (const auto& f : st.value("functions", json::array()))
            std::cout << "    " << f.value("signature", "") << "\n";
    } else {
        std::cout << "  (not built)\n";
    }
    std::cout << "\n";

    // --- Optimizer ---
    std::cout << "--- Optimizer ---\n";
    if (!result["optimized"].is_null()) {
        const auto& o = result["optimized"];
        std::cout << "  nodes before: " << o.value("nodesBefore", 0)
                  << "   after: " << o.value("nodesAfter", 0)
                  << "   removed: " << o.value("removed", 0) << "\n";
    } else {
        std::cout << "  (not run)\n";
    }
    std::cout << "\n";

    // --- Output ---
    std::cout << "--- Output ---\n";
    for (const auto& line : result["output"])
        std::cout << line.get<std::string>() << "\n";
    if (result["output"].empty()) std::cout << "  (no output)\n";
    std::cout << "\n";

    // --- Errors ---
    std::cout << "--- Errors (" << result["errors"].size() << ") ---\n";
    for (const auto& e : result["errors"]) {
        std::cout << "  [" << e.value("phase", "") << "] " << e.value("type", "")
                  << " (line " << e.value("line", 0) << "): " << e.value("message", "") << "\n";
        std::string snip = e.value("snippet", "");
        if (!snip.empty()) std::cout << "      | " << snip << "\n";
    }
    if (result["errors"].empty()) std::cout << "  (none)\n";

    return result.value("success", false) ? 0 : 1;
}
