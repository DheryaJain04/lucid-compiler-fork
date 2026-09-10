// ============================================================================
// error.hpp — Diagnostic type shared by every phase.
// Phase: (shared)  Input: n/a  Output: Error struct + collection helpers.
// Any phase can append an Error; the compiler serializes them all for the UI.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include "../third_party/json.hpp"

namespace lucid {

// One diagnostic. `phase` is "lexical" | "syntax" | "semantic" | "runtime".
// `snippet` is the offending source line, so the UI/CLI can show context.
struct Error {
    std::string phase;
    std::string type;      // short category, e.g. "TypeMismatch"
    std::string message;   // human-readable explanation
    int line = 0;
    int col = 0;
    std::string snippet;   // the source line the error sits on

    nlohmann::json toJson() const {
        return nlohmann::json{
            {"phase", phase},
            {"type", type},
            {"message", message},
            {"line", line},
            {"col", col},
            {"snippet", snippet}
        };
    }
};

// A growable list of diagnostics. Each phase gets its own ErrorList so the
// compiler can decide whether to continue to the next phase.
class ErrorList {
public:
    void add(const std::string& phase, const std::string& type,
             const std::string& message, int line, int col,
             const std::string& snippet = "") {
        errors_.push_back(Error{phase, type, message, line, col, snippet});
    }

    void add(const Error& e) { errors_.push_back(e); }

    bool empty() const { return errors_.empty(); }
    size_t size() const { return errors_.size(); }
    const std::vector<Error>& all() const { return errors_; }

    nlohmann::json toJson() const {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& e : errors_) arr.push_back(e.toJson());
        return arr;
    }

private:
    std::vector<Error> errors_;
};

// Pull one 1-based line out of the source for use as an Error snippet.
inline std::string sourceLine(const std::string& source, int line) {
    if (line < 1) return "";
    int current = 1;
    size_t start = 0;
    for (size_t i = 0; i < source.size(); ++i) {
        if (current == line) {
            size_t end = source.find('\n', i);
            if (end == std::string::npos) end = source.size();
            std::string s = source.substr(start, end - start);
            // trim trailing carriage return (Windows line endings)
            if (!s.empty() && s.back() == '\r') s.pop_back();
            return s;
        }
        if (source[i] == '\n') {
            ++current;
            start = i + 1;
        }
    }
    // Last line with no trailing newline.
    if (current == line) {
        std::string s = source.substr(start);
        if (!s.empty() && s.back() == '\r') s.pop_back();
        return s;
    }
    return "";
}

} // namespace lucid
