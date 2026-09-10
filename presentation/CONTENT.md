# Lucid — Project Review Content Pack
### Compiler Design · Ideation Review
**Deliverable:** speaker content for the 17-slide deck (`Lucid_Review_Deck.pptx`)
**Timing:** full script ≈ 20 min talk + 4 min demo. For a 15-minute slot: drop slides 7 and 12, and cut slides 3, 9, 10, 11 to their first two paragraphs — never cut slide 13.
**Marks map:** Section 1 = 20 (Problem 10 + Objectives & Scope 10) · Section 2 = 30 (Architecture 10 + Methodology & Compiler Concepts 20)

---

## One-line pitch (memorise this)

> **Lucid is a compiler for a small statically-typed language, written from scratch in C++17, wrapped in a browser playground that makes every internal phase — tokens, AST, symbol table, optimised tree, output — visible live as you type.**

If the panel asks only one question, this sentence must answer it.

---

# SECTION 1 — PROBLEM AND OBJECTIVES (20 marks)

## Slide 2 — The Problem

**On slide:**
- A compiler is the most-used tool in software, and the least understood.
- Production compilers (GCC, Clang, javac) are **black boxes**: source goes in, a binary or a one-line error comes out.
- The learner never sees the *intermediate representations* that the theory is entirely about.
- Textbook artefacts — token streams, parse trees, symbol tables, optimised IR — exist only as **static figures on paper**, never as live objects tied to code the student wrote.
- When compilation fails, the diagnostic is terse (`error: expected ';' before 'print'`) and **positional, not conceptual** — it says *where*, never *which phase rejected it and why*.

**What to say (≈90 seconds):**
"Everyone in this room has used a compiler today. Almost nobody has seen inside one. That gap is the problem we picked.

There are two halves to it. The first is *opacity*: GCC will not show you its token stream or its AST. The theory we are taught — lexical analysis, parsing, semantic analysis, optimisation — describes structures that no tool actually lets you look at. We learn them as diagrams in a textbook, disconnected from any program we ever wrote.

The second is *diagnostic poverty*. When a real compiler rejects your code it tells you a line number. It does not tell you which phase rejected it. A missing semicolon and a type mismatch are fundamentally different failures — one is syntactic, one is semantic — but to the student they look identical: red text in a terminal.

So the problem statement is: **compiler internals are invisible, and compiler errors are unexplained.**"

---

## Slide 3 — What Lucid Is (project overview)

**On slide:**
- **Lucid** = a complete compiler for a small statically-typed language + a live browser playground for its own internals.
- Language: 3 types (`int`, `string`, `bool`), variables, arithmetic, boolean logic, `if/else`, `while`, `print`, typed functions with return types, comments.
- Front end: **five real phases** — Lexer → Parser → Semantic Analyzer → Optimizer → Interpreter — each in its own translation unit.
- Every phase **emits its intermediate representation as JSON**; the browser renders each one as a tab that updates ~300 ms after every keystroke.
- Two executables over one shared core: `lucidc` (offline CLI) and `lucid-server` (playground).
- An **optional AI layer** explains errors in plain English and attempts auto-fixes — but the C++ compiler, never the model, decides whether code is correct.
- Scale: ~3,600 lines — ~2,400 C++17, ~1,150 front-end. Zero frameworks; two vendored single-header libraries.

**What to say (≈90 seconds):**
"Lucid is our answer, and it is two things bolted together.

The first is a genuine compiler. Not a simulation, not a tutorial toy that pattern-matches strings — a real five-phase front end in C++17 that takes source text and produces either a running program or a precise diagnostic. It compiles its own language: statically typed, three types, control flow, and functions with typed parameters and return types.

The second is what makes it a *teaching* compiler. Every phase serialises its intermediate representation to JSON, and a browser playground renders each one as a live tab. You type a character; three hundred milliseconds later the token table, the graphical syntax tree, the symbol table, the optimiser's before-and-after, and the program output all refresh. The compiler stops being a black box because you are watching its internals move.

And the compiler stays the authority throughout. There is an AI assistant that explains errors, but it never parses program logic and it never decides correctness — that verdict always comes from our C++ code."

---

## Slide 4 — Objectives

**On slide (six objectives, numbered):**

| # | Objective |
|---|---|
| **O1** | **Build a complete compiler front end from scratch** in C++17 — lexer, parser, semantic analyser, optimiser, evaluator — with no parser generator, no framework, no external runtime. |
| **O2** | **Make every intermediate representation observable.** Each phase must serialise its own IR so the token stream, AST, symbol table and optimised tree can be inspected, not just described. |
| **O3** | **Classify every error by the phase that caught it** — lexical / syntax / semantic / runtime — and report it with line, column, source snippet and a phase label. |
| **O4** | **Demonstrate real optimisation with measurable effect** — constant folding and dead-code elimination, reported as a verifiable before/after AST node count. |
| **O5** | **Recover from errors instead of stopping at the first one** — panic-mode recovery in the parser, single-pass exhaustive reporting in the semantic analyser. |
| **O6** | **Layer AI as an explainer, never an oracle** — Gemini rephrases the compiler's diagnostics and proposes fixes that are re-validated by re-running the real compiler. |

**What to say (≈75 seconds):**
"Six objectives, and the ordering is deliberate — the first five are compiler engineering, and only the sixth touches AI.

O1 is the non-negotiable one: everything is hand-written. No Lex, no Yacc, no ANTLR. If we generated the parser we would be demonstrating a tool, not the theory.

O2 is what makes the project distinct: observability is a design requirement, not a feature we bolted on. Every phase must be able to describe its own output.

O3 attacks the diagnostic problem directly — an error is not just a message, it carries the identity of the phase that raised it.

O4 forces us past a pure interpreter: we must actually transform the tree and prove the transformation happened with numbers.

O5 is about quality of implementation — a compiler that dies on the first error is not usable, so we implement real recovery.

O6 is scoped defensively on purpose. The AI is a presentation layer over our diagnostics. It is optional, it runs in the browser, and the project is complete without it."

---

## Slide 5 — Scope

**IN SCOPE**
- A statically-typed language: `int`, `string`, `bool`; declarations, assignment, arithmetic (`+ - * / %`), comparison (`< > <= >= == !=`), boolean logic (`&& || !`), `if/else`, `while`, `print`, functions with typed params and return type, `//` comments.
- All five phases, hand-written, each independently testable.
- Full static type checking with a scoped symbol table.
- Two AST-level optimisations: constant folding, dead-code elimination.
- Tree-walking interpreter as the execution back end.
- Four-category error taxonomy with line/column/snippet and "did you mean …?" suggestions.
- Browser playground (5 live tabs) + offline CLI.

**OUT OF SCOPE (deliberate, defensible boundaries)**
- **No machine-code / assembly generation** — the back end is a tree-walking interpreter. The academic weight of this project is in the *front end*; codegen and register allocation are a separate problem.
- **No three-address code / SSA IR** — optimisation is performed directly on the typed AST, which keeps the transformation visually explainable in the playground.
- **No floats, arrays, structs, pointers, or classes** — three types keep the type-checker rules small enough to state exhaustively and defend.
- **No recursion-heavy stdlib, no modules, no imports, no I/O beyond `print`.**
- **No deployment / multi-user hosting** — it is a local tool; the browser-side API key is acceptable locally and explicitly documented as unsafe for hosting.

**What to say (≈75 seconds):**
"Scope is where projects like this usually go wrong, so we drew the line early and we can defend both sides of it.

In scope: the entire front end, done properly. Five phases, a real type system, a real symbol table, real optimisation, real error recovery.

Out of scope, and this is the important half: we do not generate machine code. The back end is a tree-walking interpreter. That is a conscious trade — the intellectual content of a compiler-design course is lexing, parsing, semantic analysis and optimisation, and by stopping at execution we can do all four thoroughly instead of doing six things shallowly.

We also optimise on the AST rather than lowering to three-address code. That is a trade too: a TAC listing is a wall of text, whereas an AST shrinking from thirty-eight nodes to seventeen is something you can *watch* happen. Given that observability is objective two, the AST was the correct IR.

And we cap the language at three types. Every typing rule in Lucid can be written on one slide — which means every one of them can be defended in this room."

---

# SECTION 2 — PROPOSED METHODOLOGY (30 marks)

## Slide 6 — Proposed Architecture (system view)

**On slide — the layered diagram:**

```
┌──────────────────────────────────────────────────────────┐
│  PRESENTATION LAYER — browser playground (HTML/CSS/JS)   │
│  Tokens · AST · Symbol Table · Output · Optimizer tabs   │
│  Debounced live compile (~300 ms)   +  optional Gemini   │
└───────────────────────┬──────────────────────────────────┘
                        │  POST /compile  { "source": "…" }
                        │  ← JSON { tokens, ast, symbolTable,
                        │           optimized, output, errors }
┌───────────────────────▼──────────────────────────────────┐
│  SERVICE LAYER — lucid-server (cpp-httplib, single hdr)  │
│  Serves static web/ + one endpoint. No compiler logic.   │
└───────────────────────┬──────────────────────────────────┘
                        │  lucid::compile(source)
┌───────────────────────▼──────────────────────────────────┐
│  CORE LAYER — liblucid_core.a  (pure C++17, no network)  │
│   Lexer → Parser → Semantic → Optimizer → Interpreter    │
│   compiler.cpp orchestrates; every phase emits JSON IR   │
└──────────────────────────────────────────────────────────┘
        ▲                                    ▲
        │                                    │
   lucidc (CLI)                        lucid-server
   offline, no network                 playground host
```

**Key architectural decisions (put these as bullets beside the diagram):**
1. **Three layers, one direction of dependency.** The core knows nothing about HTTP or the browser. It is a static library, `liblucid_core.a`, linked by both executables.
2. **Two front ends, one core.** `lucidc` proves the compiler is independent of the web — it runs offline with no network. `lucid-server` is a thin HTTP shell.
3. **JSON is the contract.** Every phase produces `toJson()`. This is what makes the IRs renderable — and it is why the same core serves both a terminal tree-view and a browser SVG tree.
4. **The service layer is deliberately thin** (~90 lines). It parses a request, calls `lucid::compile`, returns the JSON. No compilation logic leaks upward.
5. **AI is outside the trust boundary** — it lives in `web/app.js`, in the browser, and can be removed entirely without touching a line of C++.
6. **Dependencies are vendored, not installed** — `nlohmann/json` and `cpp-httplib` are single headers in `third_party/`, so the project builds anywhere with CMake ≥ 3.16 and a C++17 compiler.

**What to say (≈2 minutes):**
"The architecture is three layers with a single direction of dependency, and that constraint is what buys us everything else.

At the bottom is the core — pure C++17, the five phases, no networking, no I/O beyond reading a string. It compiles to a static library. Notice what it does *not* know: it has never heard of HTTP or of a browser.

Above it sits a service layer, about ninety lines, using cpp-httplib. It serves the static web files and exposes exactly one endpoint — POST /compile. It takes a source string, calls `lucid::compile`, and returns the JSON result. There is deliberately no compiler logic in it.

On top is the presentation layer, plain HTML, CSS and JavaScript, which renders that JSON as five tabs.

Two things follow from this shape. First, the same core is linked by two executables — the CLI and the server. The CLI is our proof of independence and our fallback: if the network, the browser, or the projector fails tomorrow, `lucidc` still prints the tokens, the tree, the symbol table, the optimiser counts and the output to a terminal.

Second, the AI sits entirely outside this stack, in the browser. It is optional. Delete it, and the compiler is unchanged.

And the contract between the layers is JSON — every AST node, every token, every symbol table entry knows how to serialise itself. That single decision is what turns an ordinary compiler into an observable one."

---

## Slide 7 — Architecture: module decomposition

| File | Phase | Input → Output | LOC |
|---|---|---|---|
| `token.hpp` | shared | Token struct + `TokenType` enum (37 kinds) | 120 |
| `error.hpp` | shared | `Error` + `ErrorList` (phase, type, line, col, snippet) | 90 |
| `ast.hpp` | shared | AST node classes, each with `toJson()` | 264 |
| `lexer.cpp/.hpp` | **1** | source text → `vector<Token>` | 215 |
| `parser.cpp/.hpp` | **2** | tokens → `Program` AST | 446 |
| `semantic.cpp/.hpp` | **3** | AST → type-checked AST + symbol-table JSON | 487 |
| `optimizer.cpp/.hpp` | **4** | typed AST → smaller equivalent AST | 214 |
| `interpreter.cpp/.hpp` | **5** | optimised AST → program output | 295 |
| `compiler.cpp/.hpp` | orchestrator | source → one JSON result | 98 |
| `main_cli.cpp` / `main_server.cpp` | entry points | CLI / HTTP shells | 200 |

**Talking point:** "One phase, one translation unit, one responsibility. Every file opens with a comment naming its phase and its input/output type — so any phase can be opened and explained in isolation, which is exactly what a viva demands."

---

## Slide 8 — Methodology: the five-phase pipeline

**On slide — pipeline with concrete data for `valid.lucid`:**

```
 source text
     │  25 lines of Lucid
     ▼
┌─────────────┐  PHASE 1 · LEXICAL ANALYSIS
│   LEXER     │  → 95 tokens, each with (type, lexeme, line, col)
└─────┬───────┘  catches: unknown character, unterminated string
      ▼
┌─────────────┐  PHASE 2 · SYNTAX ANALYSIS
│   PARSER    │  → AST (Program → statements → expressions)
└─────┬───────┘  recursive descent + precedence climbing
      ▼          catches: unexpected token, missing ; ) }, int overflow
┌─────────────┐  PHASE 3 · SEMANTIC ANALYSIS
│  SEMANTIC   │  → typed AST + symbol table snapshot
└─────┬───────┘  scope stack, type rules, function signatures
      ▼          catches: undeclared/redeclared, type mismatch,
┌─────────────┐          non-bool condition, bad call, bad return
│  OPTIMIZER  │  PHASE 4 · OPTIMISATION
└─────┬───────┘  → equivalent smaller AST   (38 → 17 nodes, −55%)
      ▼          constant folding + dead-code elimination
┌─────────────┐  PHASE 5 · EXECUTION
│ INTERPRETER │  → program output
└─────┬───────┘  tree-walking evaluator over the *optimised* tree
      ▼          catches: division / modulo by zero
   output + errors + every IR, as one JSON document
```

**Methodological rule shown on slide:**
> **Fail fast between phases.** `compiler.cpp` runs each phase and returns immediately if that phase produced errors. You never type-check a program that did not parse. Every IR produced *before* the failure is still returned and still rendered — so a broken program still shows you its tokens.

**What to say (≈90 seconds):**
"This is the methodology in one picture, and I have put the real numbers from our test program on it.

Twenty-five lines of Lucid go into the lexer and ninety-five tokens come out, each carrying its type, its text, and its line and column — those coordinates are what every later error message is built from.

The parser turns that flat token stream into a tree. The semantic analyser walks the tree, builds a symbol table, and annotates types. The optimiser rewrites the tree into a smaller equivalent one — on our optimiser test case, thirty-eight nodes down to seventeen. The interpreter then walks the *optimised* tree, not the original.

The orchestration rule matters: phases fail fast. If lexing produced errors we return before parsing; if parsing failed we never attempt type-checking. That is not laziness, it is correctness — type-checking a malformed tree produces cascades of nonsense errors.

But — and this is the teaching part — everything produced *before* the failure is still returned. Break your program, and you still get the token table. You watch the pipeline stop at exactly the phase that rejected your code."

---

## Slide 9 — Compiler concepts I: Lexical & Syntax analysis

### Phase 1 — Lexical Analysis (`lexer.cpp`)
| Concept | How Lucid implements it |
|---|---|
| Token / lexeme / pattern | `Token { type, value, line, col }`; 37 `TokenType` values |
| Keyword recognition | A **reserved-word table** (`unordered_map`): identifiers are scanned maximally, then looked up — the standard "identifier-then-keyword-check" technique |
| Maximal munch | Two-character operators (`<=`, `>=`, `==`, `!=`, `&&`, `||`) resolved by one-character lookahead (`match()`) |
| Input buffering & position tracking | Single pass with `pos_`, `line_`, `col_`; `advance()` updates line/column on `\n` |
| Whitespace / comment stripping | `skipWhitespaceAndComments()` — `//` runs to end of line |
| Lexical errors | Unknown character; unterminated string literal |

### Phase 2 — Syntax Analysis (`parser.cpp`)
| Concept | How Lucid implements it |
|---|---|
| Context-free grammar | Lucid's grammar is encoded **as the call graph of the parser** — one function per non-terminal |
| Top-down parsing | **Recursive descent**, hand-written (predictive, LL(1) with one-token lookahead: `peek()`, `check()`, `match()`) |
| Operator precedence | **Precedence climbing** — an 8-function cascade: `expression → logicalOr → logicalAnd → equality → comparison → term → factor → unary → primary` |
| Left associativity | Each level uses a `while` loop that folds left, so `a - b - c` parses as `(a-b)-c` |
| Parse tree → AST | The parser builds an **abstract** syntax tree — no punctuation nodes, only meaning-bearing nodes; every node stores its source line |
| **Panic-mode error recovery** | On error, throw `ParseError`, then `synchronize()` discards tokens until a statement boundary (`;`, or a leading keyword `int/if/while/func/return/print`, or `}`) and parsing resumes |
| Robustness | A **progress guard** forces one token forward if an iteration consumed nothing (prevents infinite loops on a stray `}`); error flood capped at 100 |

**What to say (≈2 minutes):**
"Now the part the marks are actually for — where the theory shows up in the code.

Phase one, lexical analysis. We scan the character stream once, tracking line and column as we go, because every diagnostic in the entire compiler is built from those two numbers. Keywords are handled the textbook way: we scan an identifier maximally, then look it up in a reserved-word table — that is why a variable called `integer` is an identifier but `int` is a keyword. Two-character operators use one-character lookahead, which is maximal munch: on seeing `<` we peek for `=` before deciding between less-than and less-or-equal.

Phase two, syntax analysis. This is a hand-written recursive-descent parser — one function per non-terminal, so the grammar of Lucid literally *is* the call graph of the parser. You can read the grammar by reading the function names.

Precedence and associativity come from the shape of the descent. There is an eight-function cascade from logical-or down to primary, and each level loops rather than recursing on the left, which gives us left associativity for free. That is why `2 + 3 * 4` produces the correct tree without a single precedence table.

And we implement real error recovery — panic mode. When a statement fails to parse we throw, discard tokens until we reach a plausible statement boundary, and restart. That is why one missing semicolon does not produce forty errors. We also added a progress guard: `synchronize` can legitimately stop on a token it will not consume, so we force one token forward if a whole iteration consumed nothing — otherwise a stray closing brace becomes an infinite loop. That bug is real and we fixed it."

---

## Slide 10 — Compiler concepts II: Semantic analysis

| Concept | How Lucid implements it |
|---|---|
| **Symbol table** | `vector<unordered_map<string, VarEntry>>` — a stack of scopes; `pushScope()` / `popScope()` on block entry and exit |
| **Scope resolution** | `lookupVar()` searches innermost → outermost (`rbegin` → `rend`) — classic lexical scoping |
| **Function signatures** | Separate `funcs_` map holding `FuncSig { paramTypes, paramNames, returnType, line }` |
| **Static type system** | `enum class Ty { Int, String, Bool, Error }` — `Error` is a sentinel that **suppresses cascading errors** once a sub-expression is already known bad |
| **Type checking of expressions** | `checkExpr()` returns the type of every node bottom-up; `checkBinary()` applies the operator rules |
| **Type rules** | `+` = int+int **or** string+string · `- * / %` = int only · `< > <= >=` = int→bool · `== !=` = same type→bool · `&& \|\| !` = bool · `if`/`while` conditions **must** be bool |
| **Function-call checking** | Undefined function, wrong argument **count**, wrong argument **types**, return-type mismatch |
| **Scope isolation rule** | Function bodies see **only their parameters** — never globals. The type-checker and the interpreter both enforce it, so *type-check success implies no runtime scope surprise* |
| **Exhaustive single-pass reporting** | The checker never stops at the first error — `err_type_mismatch.lucid` reports **all three** type errors in one run |
| **Error suggestions** | **Levenshtein edit distance** against all in-scope names; suggests when distance ≤ max(2, len/3) → `'cont'` ⇒ *"Did you mean 'count'?"* |

**Live evidence to show:**
```
[semantic] UndeclaredVariable (line 2): Use of undeclared variable 'cont'. Did you mean 'count'?
      | print(cont);
```

**What to say (≈2 minutes):**
"Phase three is where a compiler stops checking *form* and starts checking *meaning*, and it is the densest phase in the project.

The symbol table is a stack of hash maps — one map per scope. Entering a block pushes a scope, leaving pops it, and a lookup walks from the innermost scope outward. That is lexical scoping, implemented exactly as the theory describes it.

The type system is four values: int, string, bool, and an internal Error type. That fourth one is a technique worth pointing out. When a sub-expression is already known to be bad we give it type Error, and every rule treats Error as compatible with everything. The effect is that one mistake produces one message instead of a cascade of five.

The typing rules are deliberately small enough to state completely. Plus is overloaded — integer addition or string concatenation — and every other arithmetic operator is int-only. Comparisons take ints and produce bool. Equality requires both sides to be the same type. Conditions in `if` and `while` must be bool, which is why `if (x)` on an integer is a compile error in Lucid, and that is a design decision we can defend: it eliminates the entire class of accidental-assignment-in-condition bugs.

Two things I want to draw attention to. First, function bodies are scope-isolated — a function sees its parameters and nothing else, no globals. Both the type-checker and the interpreter enforce that same rule, which gives us a real soundness property: if a program type-checks, it will not hit a scope surprise at runtime.

Second, error reporting is exhaustive in a single pass. We do not stop at the first error. And undeclared names get a Levenshtein-based suggestion — the compiler measures edit distance against every name in scope, so typing `cont` when you declared `count` tells you exactly that."

---

## Slide 11 — Compiler concepts III: Optimisation

**Two classical AST-level transformations, applied bottom-up in one pass:**

**1. Constant folding** — evaluate at compile time what does not depend on runtime state
```
int a = 2 + 3 * 4;        →   int a = 14;
bool t = 1 < 2 && true;   →   bool t = true;
return 10 + 5;            →   return 15;
"a" + "b"                 →   "ab"
!true                     →   false
```

**2. Dead-code elimination** — remove code that provably cannot execute
```
if (false) { print("unreachable"); } else { print("kept"); }
                          →   print("kept");        // branch collapsed
if (2 > 1) { print("always"); }
                          →   print("always");      // condition folded, then unwrapped
while (false) { … }       →   removed entirely
return 15; print("dead"); →   print("dead") removed  // unreachable after return
```

**Measured result — `examples/optimize.lucid` (verified output):**

| | AST nodes |
|---|---|
| Before optimisation | **38** |
| After optimisation | **17** |
| **Removed** | **21  (−55 %)** |

**Ordering justification (a likely viva question):**
- Runs **after** semantic analysis → the tree is already well-typed, so folding `2 + 3` is guaranteed type-safe and needs no re-checking.
- Runs **before** the interpreter → execution happens on the smaller equivalent program.
- Folding and elimination are **mutually enabling**: folding `2 > 1` into `true` is what lets dead-code elimination unwrap the `if`. That is why they are one bottom-up pass, not two.

**What to say (≈90 seconds):**
"Phase four. Two classical optimisations, and the interesting part is that they feed each other.

Constant folding evaluates at compile time anything that does not depend on runtime state. `2 + 3 * 4` becomes the literal 14 before the program ever runs. It works on strings too — concatenation of two literals folds — and on booleans.

Dead-code elimination removes what cannot execute. An `if (false)` collapses to its else branch. A `while (false)` disappears. Anything after a `return` in the same block is deleted.

Now, why one pass and not two: folding `2 > 1` into `true` is precisely what *enables* the dead-code eliminator to unwrap that `if`. They are mutually enabling, so we apply them together bottom-up.

And we measure it. Our optimiser test program goes from thirty-eight AST nodes to seventeen — a fifty-five percent reduction — and the playground shows the before and after trees side by side. That number is computed by the compiler, not claimed by us.

The placement in the pipeline is also deliberate: after semantic analysis, so the tree is already well-typed and folding is guaranteed safe; before the interpreter, so we execute the smaller program."

---

## Slide 12 — Compiler concepts IV: Execution & runtime

| Concept | How Lucid implements it |
|---|---|
| Back end | **Tree-walking interpreter** over the optimised, type-checked AST |
| Runtime values | `std::variant<int, bool, std::string>` — a tagged union, type already guaranteed by phase 3 |
| Environment / activation records | Same stack-of-scopes design as the symbol table, holding *values* instead of *types* |
| Function invocation | Push a fresh scope containing **only** the bound parameters (isolation rule), execute body, pop |
| Return handling | Implemented as a control-flow signal that unwinds to the call site |
| Runtime errors | Division by zero and modulo by zero — the only failures that survive static checking, reported with line and snippet like any other error |

**The key line for the panel:**
> Phase 3 and phase 5 implement the *same* scoping rule in two different places, and they agree. A program that type-checks cannot fail at runtime for a scope or type reason — the only runtime error Lucid can produce is division by zero.

**What to say (≈60 seconds):**
"Phase five executes. It is a tree-walking interpreter — it recursively evaluates the AST that came out of the optimiser.

Runtime values are a variant over int, bool and string. Note that we never need to check types here: phase three already proved them. The interpreter simply trusts the type-checker, which is the whole point of having a static type system.

The environment mirrors the symbol table — the same stack-of-scopes structure, holding values instead of types. Calling a function pushes a fresh scope containing only the bound parameters, runs the body, and pops.

And that leaves exactly one class of runtime error: division or modulo by zero. Everything else was caught statically. That is a claim we can make precisely because the type-checker and the interpreter were written to agree with each other."

---

## Slide 13 — Concept → implementation map (the summary slide)

| Compiler-design concept | Where it lives in Lucid | Evidence in the demo |
|---|---|---|
| Lexical analysis, tokens, maximal munch | `lexer.cpp` | Tokens tab — 95 tokens with line/col |
| Reserved-word table | `keywords()` in `lexer.cpp` | `int` vs `integer` |
| CFG & top-down parsing | `parser.cpp` — one function per non-terminal | AST tab |
| Recursive descent, LL(1) lookahead | `peek()` / `check()` / `match()` | — |
| Operator precedence & associativity | 8-function precedence cascade | `2 + 3 * 4` tree shape |
| Panic-mode error recovery | `synchronize()` | `err_missing_semicolon.lucid` |
| Abstract syntax tree | `ast.hpp`, every node `toJson()` | Graphical tree |
| Symbol table & scope stack | `scopes_` in `semantic.cpp` | Symbol Table tab |
| Static type checking | `checkExpr` / `checkBinary` / `checkCall` | `err_type_mismatch.lucid` → 3 errors at once |
| Type-error suppression sentinel | `Ty::Error` | no cascades |
| Constant folding | `foldExpr()` | Optimizer tab, before/after |
| Dead-code elimination | `optStmtList()` / `optStmt()` | 38 → 17 nodes |
| Activation records / environments | `scopes_` in `interpreter.cpp` | function calls |
| Error taxonomy by phase | `error.hpp` `ErrorList` | Errors panel, colour-coded |

**Talking point:** "Fourteen concepts from the syllabus, each with a named file, a named function, and a demo that shows it running. That is the case we are making: this project does not *describe* compiler theory, it *executes* it."

---

## Slide 14 — Error handling as a cross-cutting concern

**The four-category taxonomy:**

| Phase | Errors caught |
|---|---|
| **Lexical** | unknown character; unterminated string |
| **Syntax** | unexpected token; missing `;` `)` `}`; malformed statement; integer overflow |
| **Semantic** | undeclared / redeclared variable; type mismatch in declaration and in operations; non-boolean condition; call to undefined function; wrong argument count; wrong argument types; return-type mismatch |
| **Runtime** | division by zero; modulo by zero |

**Every error carries:** phase · error type · message · line · column · the source line itself.

**Three deliberate quality behaviours:**
1. **Exhaustive semantic reporting** — all errors in one pass, not just the first.
2. **Cascade suppression** — `Ty::Error` stops one mistake becoming five messages.
3. **Suggestions** — Levenshtein "Did you mean …?" on undeclared names.

**Validation strategy:** `examples/` holds one valid program plus **one intentionally-broken program per error category** — `err_missing_semicolon`, `err_undeclared`, `err_type_mismatch`, `err_condition`, `err_argcount`, `err_divzero`. Each is a regression test with a known expected diagnostic, loadable from the playground dropdown.

**Talking point:** "Errors are not an afterthought here, they are the product. This is the slide that answers the second half of our problem statement — a Lucid error tells you *which phase* rejected your code and *why*, not just where."

---

## Slide 15 — The AI assistant layer (correctly scoped)

**Position:** the compiler is the authority; the model is a translator.

- Lives **only** in `web/app.js`, in the browser. Zero C++ dependency on it.
- **Fully optional** — with no API key the app works completely; each AI action instead displays the exact request payload that *would* be sent, so the pipeline stays demonstrable offline.
- Three actions: **Explain error** (plain-English rephrasing of *our* diagnostic) · **Fix it** (auto-repair loop) · **Explain program**.

**The auto-fix loop — a genuine perceive → act → observe cycle:**
```
   ┌────────────────────────────────────────────────┐
   │  1. PERCEIVE  broken source + our compiler's   │
   │               structured error                 │
   │  2. ACT       Gemini proposes corrected source │
   │  3. OBSERVE   re-POST to /compile — the REAL   │
   │               C++ compiler re-validates        │
   └───────────── repeat ≤ 3 times ─────────────────┘
        Success is decided by the compiler, never by the model.
```

**Why this framing protects the project:** the model never tokenises, never parses, never type-checks, and never decides correctness. Remove it and the compiler is unchanged. It is a UX layer over deterministic diagnostics — which is exactly why it strengthens the project instead of diluting it.

**Talking point (be ready for this question):** "If the panel asks 'is the AI doing the compiling?' — the answer is no, and the architecture proves it: the AI is JavaScript in a browser, the compiler is C++ in a static library, and the only thing crossing between them is a source string and a JSON verdict that our code produced."

---

## Slide 16 — Deliverables, demo plan & expected outcome

**Deliverables**
- `liblucid_core.a` — the five-phase compiler, ~2,400 lines of C++17
- `lucidc` — offline CLI compiler (tokens, AST, symbol table, optimiser counts, errors, output)
- `lucid-server` + playground — five live tabs, debounced compile
- `examples/` — a valid program + one per error category, as regression tests
- README with grammar, type rules, error taxonomy and build instructions

**Demo sequence (4 minutes, rehearse this order)**
1. `valid.lucid` — type, watch 95 tokens and the tree appear live.
2. Delete one semicolon — watch the pipeline stop at **Syntax**, tokens still shown.
3. `err_type_mismatch.lucid` — **three** semantic errors reported at once.
4. `err_undeclared.lucid` — *"Did you mean 'count'?"*
5. `optimize.lucid` — Optimizer tab, **38 → 17** nodes side by side.
6. (Optional) **Fix it** — AI proposes, compiler re-validates, error clears.

**Fallback:** if anything fails, `./build/lucidc examples/valid.lucid` shows the entire pipeline in a terminal — no browser, no network.

**Expected outcome:** a working compiler that is also a teaching instrument — one where every concept from the syllabus is not merely implemented but *visible while it runs*.

---

# APPENDIX — Likely viva questions & answers

**Q. Why not use Lex/Yacc or ANTLR?**
Because the objective is to demonstrate the theory, not a tool. A generated parser hides precedence handling and error recovery — the two things we most wanted to show. Hand-writing recursive descent also gave us control over diagnostics, which a generator's default error messages would not.

**Q. Is this really a compiler if it doesn't produce machine code?**
It is a complete compiler *front end* with an interpretive back end — the same architecture as CPython or early Java. All the analysis phases are real; the code generator is replaced by a tree-walking evaluator. We scoped that consciously, and it is stated on the scope slide.

**Q. Why optimise the AST instead of generating three-address code?**
Two reasons. Correctness: the AST is already type-annotated, so folding is provably safe. Pedagogy: an AST shrinking from 38 nodes to 17 is *visible*; a TAC listing is not. Since observability is a stated objective, the AST was the right IR.

**Q. What is your parser's grammar class?**
It is a predictive top-down parser — LL(1) with one token of lookahead, using precedence climbing for expressions so we avoid the left-recursion problem that a naive expression grammar would create.

**Q. How do you handle left recursion?**
We do not write it. Each expression level is a loop, not a left-recursive rule: `term → factor (('+'|'-') factor)*`. The loop gives left associativity without left recursion — the standard transformation.

**Q. Why must `if` conditions be bool?**
Because implicit int-to-bool coercion is the source of a whole class of bugs, and because a small language should have explicit rules. `if (x)` on an int is a compile error, and the type-checker catches it in phase 3.

**Q. What happens if the AI suggests wrong code?**
Nothing bad — the fix is re-submitted to the real compiler, which rejects it. The loop tries at most three times and shows every attempt. The model can never mark its own work.

**Q. What is the most difficult part you implemented?**
Error recovery in the parser. Panic-mode synchronisation can stop on a token it will not consume — a stray `}` at top level — and the statement parser then fails on the same token forever. We added a progress guard that forces one token forward if an iteration consumed nothing, plus a 100-error cap.

**Q. How do you know the optimiser is correct?**
It runs after type-checking on a well-typed tree, it only replaces expressions with literals of the same type, and the interpreter runs on the optimised tree — so the output of `optimize.lucid` (`kept`, `always`, `15`) is itself the check that the transformation preserved semantics.

**Q. What would you add next?**
Three-address code generation as a sixth visible tab, `for` loops and arrays, and moving the Gemini call server-side behind `POST /explain` so the API key never reaches the browser — which the README already documents as the correct fix for any hosted version.
