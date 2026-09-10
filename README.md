# Lucid

**Lucid** is a compiler for a small, statically-typed language, written in modern
C++17. It runs as a local HTTP server behind a browser *playground* that shows
every compiler phase live — tokens, a **graphical AST**, the symbol table, the
optimizer's before/after, and program output. When your code has an error, the
C++ compiler catches it deterministically and (optionally) a **Gemini Flash**
agent in the browser explains it in plain English and can auto-fix it.

> The C++ compiler is the star. The AI is an assistant layer: it never parses or
> generates program logic — it only *explains* errors and *proposes* fixes that
> are re-validated by re-running the real compiler.

---

## Architecture

```
Browser playground (HTML/CSS/JS)
   |  POST /compile  { "source": "..." }
   v
C++ HTTP server (cpp-httplib, single header)
   |
   v
C++ compiler:  Lexer -> Parser -> SemanticAnalyzer -> Optimizer -> Interpreter
   |
   returns JSON: { tokens, ast, symbolTable, optimized, output, errors }
```

- The **compiler** is pure C++17 — no framework, no network. Each phase lives in
  its own translation unit and is independently testable.
- The **server** (`lucid-server`) is a thin wrapper over the compiler using
  `cpp-httplib`. It also serves the static `web/` files, so there are no CORS
  issues.
- **Gemini** is called *from the browser* using a key you type into the UI. The
  whole app works with no key — you just lose the AI features.

Two executables share the same compiler core:

| Executable     | What it is                                                        |
|----------------|-------------------------------------------------------------------|
| `lucidc`       | Command-line compiler. No browser, no network. The deterministic proof + fallback demo. |
| `lucid-server` | Local playground server (serves `web/`, handles `POST /compile`). |

---

## The Lucid language

Statically typed. Three types: `int`, `string`, `bool`.

```lucid
int x = 5;
string name = "Divyanshu";
bool ready = true;

x = x + 10;

if (x > 10) {
    print("big");
} else {
    print("small");
}

int i = 0;
while (i < 5) {
    print(i);
    i = i + 1;
}

func add(int a, int b): int {
    return a + b;
}

print(add(3, 4));
```

**Features:** typed variable declarations, assignment, arithmetic (`+ - * / %`),
comparison (`< > <= >= == !=`), boolean logic (`&& || !`), `if/else`, `while`,
`print`, functions with typed params + return type, and `// line comments`.

**Type rules (crisp, for the viva):**
- `+` works on two `int`s (addition) **or** two `string`s (concatenation).
- `- * / %` require two `int`s.
- `< > <= >=` require two `int`s and produce `bool`.
- `== !=` compare two values **of the same type** and produce `bool`.
- `&& || !` operate on `bool`.
- `if`/`while` conditions must be `bool`.
- Functions run in an **isolated scope**: only their parameters (and other
  functions) are visible — never globals. The type checker and interpreter agree
  on this, so a program that type-checks won't hit a surprise at runtime.

---

## Build

Requirements: a C++17 compiler and CMake ≥ 3.16. The two dependencies
(`nlohmann/json` and `cpp-httplib`) are vendored in `third_party/` — nothing to
install.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces `build/lucidc` and `build/lucid-server`.

---

## Run

### Playground (server)

```bash
./build/lucid-server          # defaults to port 8080
# or: ./build/lucid-server 9000
```

Then open **http://localhost:8080** in a browser. Edit code on the left; the
tokens, AST, symbol table, output, optimizer, and errors update live as you type
(debounced ~300 ms). Use the dropdown to load the example gallery — one valid
program plus one intentionally-broken program per error category.

> Open it via `http://localhost:8080`, **not** by double-clicking
> `web/index.html` — the page needs the server's `/compile` endpoint.

### Command line

```bash
./build/lucidc examples/valid.lucid
```

Prints tokens, a readable AST tree, the symbol table, optimizer before/after
counts, any errors (with line + snippet), and the program output. Exit code is
`0` on success, `1` if the program had errors. This path uses **no browser and
no network**.

Sample programs live in `examples/` (a valid one plus one per error type).

---

## Gemini API key (AI features)

The AI features (explain error, auto-fix, explain program) are **optional**.

1. Get a key from Google AI Studio.
2. Paste it into the **"Gemini API key"** field in the playground (it is saved to
   your browser's `localStorage` only).
3. Now the **✨ Explain with AI** and **🔧 Fix it** buttons on each error, plus
   **Explain program**, call Gemini.

With no key, nothing hits the network: each AI action instead shows the *exact*
request payload that would be sent, so the pipeline is still demonstrable.

- Model: `gemini-3.5-flash`
- Endpoint: `POST https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash:generateContent`
- Auth header: `x-goog-api-key: <API_KEY>`

The model/endpoint constants are at the top of [`web/app.js`](web/app.js) if you
need to change them.

### The auto-fix loop (the "agent")

**🔧 Fix it** runs a real *perceive → act → observe* cycle: send the broken code
+ the compiler's error to Gemini, get corrected source, **re-POST it to
`/compile`**, and repeat up to 3 times until it compiles or the limit is hit.
Every attempt is shown. The compiler — not the model — decides success.

---

## Security note

Calling Gemini from the browser exposes the API key. That's fine for a **local
demo**, but **not** for anything deployed. A hosted version should proxy the
Gemini call through the C++ server, which would read the key from an environment
variable instead of shipping it to the browser. The compiler core already runs
server-side, so this is a small addition (a `POST /explain` handler using
libcurl) — intentionally left out here to keep the local demo dependency-free.

---

## Project layout

```
CMakeLists.txt
third_party/
  httplib.h            # cpp-httplib (single header)
  json.hpp             # nlohmann/json (single header)
src/
  token.hpp            # Token struct + TokenType enum
  error.hpp            # Error struct + ErrorList helpers
  ast.hpp              # AST node classes, each with toJson()
  lexer.hpp/.cpp       # Phase 1: text  -> tokens
  parser.hpp/.cpp      # Phase 2: tokens -> AST (recursive descent)
  semantic.hpp/.cpp    # Phase 3: type checking + symbol table + "did you mean?"
  optimizer.hpp/.cpp   # Phase 4: constant folding + dead-code elimination
  interpreter.hpp/.cpp # Phase 5: tree-walking evaluator
  compiler.hpp/.cpp    # orchestrates the phases, builds the result JSON
  main_cli.cpp         # `lucidc`  entry point
  main_server.cpp      # `lucid-server` entry point
web/
  index.html style.css app.js examples.js
examples/
  valid.lucid + one program per error type
README.md
```

Every compiler file starts with a comment naming the phase it implements and its
input/output, so each phase is easy to explain out loud.

---

## Error taxonomy

| Phase     | Errors caught |
|-----------|---------------|
| Lexical   | unknown character; unterminated string |
| Syntax    | unexpected token; missing `;`, `)`, `}`; malformed statement; integer overflow |
| Semantic  | undeclared / redeclared variable; type mismatch in declaration and operations; non-boolean condition; call to undefined function; wrong argument count / types; return-type mismatch |
| Runtime   | division / modulo by zero |

Semantic errors are reported **all in one pass** (the checker doesn't stop at the
first). Undeclared names get a Levenshtein-based *"Did you mean …?"* suggestion.

---

## Terminal-only variant (Path B)

If you don't want the web playground, the CLI (`lucidc`) already is a complete
interface: tokens, AST, symbol table, optimizer before/after, errors, and output
all print to the terminal with no browser and no network. To move the AI into
C++ as well, add an `--explain` flag that POSTs the same JSON body to the same
Gemini endpoint with libcurl and parses the reply with `nlohmann/json`; the
auto-fix loop becomes: get corrected source, re-run `lucid::compile` in-process,
repeat up to 3 times.
