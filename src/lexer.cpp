// ============================================================================
// lexer.cpp — Phase 1 of Lucid.  text -> tokens.
// Skips whitespace and // comments while tracking (line, col). Reports two
// lexical errors: unknown character and unterminated string.
// ============================================================================
#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace lucid {

// The reserved words of Lucid mapped to their token types.
static const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"int", TokenType::KwInt},       {"string", TokenType::KwString},
        {"bool", TokenType::KwBool},     {"if", TokenType::KwIf},
        {"else", TokenType::KwElse},     {"while", TokenType::KwWhile},
        {"print", TokenType::KwPrint},   {"func", TokenType::KwFunc},
        {"return", TokenType::KwReturn}, {"true", TokenType::KwTrue},
        {"false", TokenType::KwFalse},
    };
    return kw;
}

Lexer::Lexer(const std::string& source, ErrorList& errors)
    : src_(source), errors_(errors) {}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

bool Lexer::match(char expected) {
    if (atEnd() || src_[pos_] != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // line comment — run to end of line (or EOF)
            while (!atEnd() && peek() != '\n') advance();
        } else {
            return;
        }
    }
}

Token Lexer::makeString() {
    int startLine = line_, startCol = col_;
    advance();  // consume opening quote
    std::string value;
    while (!atEnd() && peek() != '"' && peek() != '\n') {
        char c = advance();
        if (c == '\\' && !atEnd()) {
            char esc = advance();
            switch (esc) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:   value += esc;  break;  // unknown escape: keep literal
            }
        } else {
            value += c;
        }
    }
    if (peek() != '"') {
        errors_.add("lexical", "UnterminatedString",
                    "String literal is missing its closing '\"'.",
                    startLine, startCol, sourceLine(src_, startLine));
        // Return a best-effort string token so the parser can keep going.
        return Token{TokenType::StringLiteral, value, startLine, startCol};
    }
    advance();  // consume closing quote
    return Token{TokenType::StringLiteral, value, startLine, startCol};
}

Token Lexer::makeNumber(int startLine, int startCol) {
    std::string num;
    while (std::isdigit(static_cast<unsigned char>(peek()))) num += advance();
    return Token{TokenType::IntLiteral, num, startLine, startCol};
}

Token Lexer::makeIdentifierOrKeyword(int startLine, int startCol) {
    std::string text;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        text += advance();
    auto it = keywords().find(text);
    TokenType type = (it != keywords().end()) ? it->second : TokenType::Identifier;
    return Token{type, text, startLine, startCol};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespaceAndComments();
        if (atEnd()) break;

        int startLine = line_, startCol = col_;
        char c = peek();

        // Identifiers / keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(makeIdentifierOrKeyword(startLine, startCol));
            continue;
        }
        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(makeNumber(startLine, startCol));
            continue;
        }
        // Strings
        if (c == '"') {
            tokens.push_back(makeString());
            continue;
        }

        // Operators & punctuation
        advance();  // consume the first char of the operator
        auto emit = [&](TokenType t, const std::string& v) {
            tokens.push_back(Token{t, v, startLine, startCol});
        };
        switch (c) {
            case '+': emit(TokenType::Plus, "+"); break;
            case '-': emit(TokenType::Minus, "-"); break;
            case '*': emit(TokenType::Star, "*"); break;
            case '/': emit(TokenType::Slash, "/"); break;
            case '%': emit(TokenType::Percent, "%"); break;
            case '(': emit(TokenType::LParen, "("); break;
            case ')': emit(TokenType::RParen, ")"); break;
            case '{': emit(TokenType::LBrace, "{"); break;
            case '}': emit(TokenType::RBrace, "}"); break;
            case ';': emit(TokenType::Semicolon, ";"); break;
            case ':': emit(TokenType::Colon, ":"); break;
            case ',': emit(TokenType::Comma, ","); break;
            case '<': if (match('=')) emit(TokenType::Le, "<="); else emit(TokenType::Lt, "<"); break;
            case '>': if (match('=')) emit(TokenType::Ge, ">="); else emit(TokenType::Gt, ">"); break;
            case '=': if (match('=')) emit(TokenType::EqEq, "=="); else emit(TokenType::Assign, "="); break;
            case '!': if (match('=')) emit(TokenType::NotEq, "!="); else emit(TokenType::Not, "!"); break;
            case '&':
                if (match('&')) emit(TokenType::AndAnd, "&&");
                else errors_.add("lexical", "UnknownCharacter",
                                 "Unexpected '&'. Did you mean '&&'?",
                                 startLine, startCol, sourceLine(src_, startLine));
                break;
            case '|':
                if (match('|')) emit(TokenType::OrOr, "||");
                else errors_.add("lexical", "UnknownCharacter",
                                 "Unexpected '|'. Did you mean '||'?",
                                 startLine, startCol, sourceLine(src_, startLine));
                break;
            default: {
                std::string ch(1, c);
                errors_.add("lexical", "UnknownCharacter",
                            "Unknown character '" + ch + "' in source.",
                            startLine, startCol, sourceLine(src_, startLine));
                break;
            }
        }
    }

    tokens.push_back(Token{TokenType::EndOfFile, "", line_, col_});
    return tokens;
}

} // namespace lucid
