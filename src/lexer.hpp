// ============================================================================
// lexer.hpp — Phase 1 of Lucid.
// Input : raw source text (std::string)
// Output: std::vector<Token>  (+ lexical errors into an ErrorList)
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include "token.hpp"
#include "error.hpp"

namespace lucid {

class Lexer {
public:
    Lexer(const std::string& source, ErrorList& errors);

    // Scan the whole source into tokens. Always ends with an EndOfFile token.
    std::vector<Token> tokenize();

private:
    const std::string& src_;
    ErrorList& errors_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    bool atEnd() const { return pos_ >= src_.size(); }
    char peek() const { return atEnd() ? '\0' : src_[pos_]; }
    char peekNext() const { return (pos_ + 1 >= src_.size()) ? '\0' : src_[pos_ + 1]; }
    char advance();                 // consume one char, updating line/col
    bool match(char expected);      // consume if next char == expected

    void skipWhitespaceAndComments();
    Token makeString();
    Token makeNumber(int startLine, int startCol);
    Token makeIdentifierOrKeyword(int startLine, int startCol);
};

} // namespace lucid
