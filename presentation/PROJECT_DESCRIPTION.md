# Lucid — Project Description

*Compiler Design project. Pick whichever length the form or report asks for.*

---

## 1. One line

> Lucid is a compiler for a small statically-typed language, written from scratch in C++17, with a browser playground that makes every internal phase visible live as you type.

---

## 2. Short abstract (~70 words)

> Lucid is a five-phase compiler for a small statically-typed language, implemented from scratch in C++17 without any parser generator or framework. Unlike a conventional compiler, every phase publishes its intermediate representation, so the token stream, syntax tree, symbol table, optimised tree and program output are rendered live in a browser playground as the user types. An optional AI assistant explains and repairs errors, while the compiler alone decides correctness.

---

## 3. Full description (~280 words)

> A compiler is among the most widely used tools in software and among the least understood. Production compilers behave as black boxes: source text goes in, and a binary or a single terse diagnostic comes out. The intermediate representations that compiler theory is built on — token streams, syntax trees, symbol tables, optimised code — are never exposed, and when compilation fails the message identifies a line but not the phase that rejected the code or the reason.
>
> **Lucid** addresses both problems. It is a complete compiler front end for a small statically-typed language, written from scratch in C++17. The language supports three types (`int`, `string`, `bool`), variable declarations and assignment, arithmetic and boolean expressions, comparison operators, `if`/`else`, `while`, `print`, and functions with typed parameters and declared return types.
>
> The compiler runs in five phases — lexical analysis, syntax analysis, semantic analysis, optimisation and execution — each implemented by hand in its own translation unit. The lexer produces positioned tokens; a recursive-descent parser with precedence climbing builds an abstract syntax tree and recovers from syntax errors using panic-mode synchronisation; a semantic analyser performs full static type checking over a stack-based symbol table and reports every error in a single pass; an optimiser applies constant folding and dead-code elimination; and a tree-walking interpreter executes the result.
>
> What distinguishes Lucid is observability. Every phase serialises its intermediate representation, which a browser playground renders as five live panels refreshed roughly 300 ms after each keystroke. Errors are classified by the phase that caught them and reported with line, column and source context, including edit-distance suggestions for misspelled names. An optional AI assistant explains errors in plain English and proposes fixes, which are then re-validated by re-running the real compiler — the model never decides whether a program is correct.

---

## 4. Key features

- **Five real compiler phases**, hand-written — no Lex, Yacc, ANTLR or framework
- **Live playground** — tokens, syntax tree, symbol table, optimiser and output, updated as you type
- **Visual syntax tree** — the AST rendered graphically rather than described
- **Full static type checking** with a scoped symbol table and function signatures
- **Real optimisation, measured** — constant folding and dead-code elimination reduce the test program from 38 AST nodes to 17
- **Errors classified by phase** — lexical, syntax, semantic and runtime, each with line, column and source snippet
- **Exhaustive error reporting** — all semantic errors in one pass, never just the first
- **"Did you mean…?" suggestions** — Levenshtein edit distance over names in scope
- **Panic-mode error recovery** — one missing semicolon produces one error, not a cascade
- **AI assistant (optional)** — explains errors and auto-repairs code, with the compiler as the sole authority on correctness
- **Two front ends** — a browser playground and an offline command-line compiler sharing one core

---

## 5. Technology

| Layer | Built with |
|---|---|
| Compiler core | C++17, no external runtime — compiled as a static library |
| Server | `cpp-httplib` (single header), one endpoint |
| Playground | HTML, CSS and JavaScript — no framework |
| Data format | `nlohmann/json` (single header) |
| Build | CMake ≥ 3.16 |
| AI assistant | Google Gemini, called from the browser, fully optional |

Both dependencies are vendored as single headers, so the project builds anywhere with CMake and a C++17 compiler.

---

## 6. Compiler phases

| # | Phase | Input → Output | Key techniques |
|---|---|---|---|
| 1 | **Lexer** | source text → tokens | reserved-word table, maximal munch, line/column tracking |
| 2 | **Parser** | tokens → syntax tree | recursive descent, precedence climbing, panic-mode recovery |
| 3 | **Semantic analyser** | tree → typed tree + symbol table | scope stack, static type rules, single-pass error reporting |
| 4 | **Optimizer** | typed tree → smaller tree | constant folding, dead-code elimination |
| 5 | **Interpreter** | optimised tree → output | tree-walking evaluation, activation records |

---

## 7. Compiler-design concepts demonstrated

Tokens and maximal munch · reserved-word tables · context-free grammar · top-down predictive parsing · recursive descent · operator precedence and associativity · panic-mode error recovery · abstract syntax trees · symbol tables and lexical scoping · static type checking · cascade suppression · constant folding · dead-code elimination · activation records · phase-based error taxonomy
