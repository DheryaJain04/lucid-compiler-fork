// ============================================================================
// app.js — Lucid playground front-end.
// Responsibilities:
//   • send source to the C++ server (POST /compile) with a ~300ms debounce
//   • render tokens, a graphical AST, the symbol table, output, optimizer
//   • wire the Gemini Flash assistant (explain error, auto-fix loop, explain
//     program). All AI is optional: with no key the pipeline still works and we
//     show the exact request that *would* be sent.
//
// The C++ compiler is the source of truth. The AI never parses or generates
// program logic — it only explains errors and proposes fixes, which are then
// re-validated by re-compiling on the server.
// ============================================================================

// ---- Gemini config (per the build brief) ----------------------------------
const GEMINI_MODEL = "gemini-3.5-flash";
const GEMINI_URL =
  `https://generativelanguage.googleapis.com/v1beta/models/${GEMINI_MODEL}:generateContent`;

// ---- DOM handles -----------------------------------------------------------
const $ = (id) => document.getElementById(id);
const editor = $("editor");
const gutter = $("gutter");
const statusPill = $("statusPill");

let lastResult = null;   // most recent compile result JSON

// ===========================================================================
// Editor plumbing
// ===========================================================================
function updateGutter() {
  const lines = editor.value.split("\n").length;
  let s = "";
  for (let i = 1; i <= lines; i++) s += i + "\n";
  gutter.textContent = s;
}
editor.addEventListener("scroll", () => { gutter.scrollTop = editor.scrollTop; });

// Insert real tabs (4 spaces) instead of losing focus.
editor.addEventListener("keydown", (e) => {
  if (e.key === "Tab") {
    e.preventDefault();
    const s = editor.selectionStart, en = editor.selectionEnd;
    editor.value = editor.value.slice(0, s) + "    " + editor.value.slice(en);
    editor.selectionStart = editor.selectionEnd = s + 4;
    scheduleCompile();
    updateGutter();
  }
});

let debounceTimer = null;
function scheduleCompile() {
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(runCompile, 300);
}
editor.addEventListener("input", () => { updateGutter(); scheduleCompile(); });

// ===========================================================================
// Compile (talk to the C++ server)
// ===========================================================================
async function compileRemote(source) {
  const res = await fetch("/compile", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ source }),
  });
  if (!res.ok) throw new Error("server responded " + res.status);
  return res.json();
}

async function runCompile() {
  setStatus("busy", "compiling…");
  try {
    const result = await compileRemote(editor.value);
    lastResult = result;
    renderAll(result);
    setStatus(result.success ? "ok" : "err",
              result.success ? "compiled ✓" : (result.errors.length + " error(s)"));
  } catch (err) {
    setStatus("err", "server offline");
    showServerOffline(err);
  }
}

function setStatus(cls, text) {
  statusPill.className = "pill " + cls;
  statusPill.textContent = text;
}

function showServerOffline(err) {
  const msg = `<p class="muted">Could not reach the C++ server at <code>/compile</code>.</p>
    <p class="muted">Start it with <code>./build/lucid-server</code> and open
    <code>http://localhost:8080</code> (don't open index.html directly as a file).</p>
    <p class="muted">Details: ${escapeHtml(String(err))}</p>`;
  $("panel-tokens").innerHTML = msg;
}

// ===========================================================================
// Rendering
// ===========================================================================
function renderAll(r) {
  // Each panel renders independently: a failure in one (say the AST layout for
  // some odd tree) must never stop the errors panel from updating — otherwise a
  // stale error would linger after you've already fixed the code. Errors and
  // the pipeline update first for exactly that reason.
  const safe = (fn, name) => { try { fn(); } catch (e) { console.error("render " + name + " failed:", e); } };
  safe(() => renderPipeline(r), "pipeline");
  safe(() => renderErrors(r.errors || []), "errors");
  safe(() => renderTokens(r.tokens || []), "tokens");
  safe(() => renderAst(r.ast), "ast");
  safe(() => renderSymbols(r.symbolTable), "symbols");
  safe(() => renderOutput(r.output || []), "output");
  safe(() => renderOptimizer(r.optimized), "optimizer");
}

function renderPipeline(r) {
  const order = ["lexical", "syntax", "semantic", "optimizer", "runtime"];
  // find earliest failing phase
  let failIdx = -1;
  for (const e of (r.errors || [])) {
    const idx = order.indexOf(e.phase);
    if (idx !== -1 && (failIdx === -1 || idx < failIdx)) failIdx = idx;
  }
  document.querySelectorAll(".pipeline .phase").forEach((el) => {
    const idx = order.indexOf(el.dataset.phase);
    el.classList.remove("ok", "err", "skip");
    if (failIdx === -1) el.classList.add("ok");
    else if (idx < failIdx) el.classList.add("ok");
    else if (idx === failIdx) el.classList.add("err");
    else el.classList.add("skip");
  });
}

function renderTokens(tokens) {
  const rows = tokens
    .filter((t) => t.type !== "EndOfFile")
    .map((t) => `<tr>
        <td class="pos">${t.line}:${t.col}</td>
        <td class="type">${escapeHtml(t.type)}</td>
        <td class="val">${escapeHtml(t.value)}</td>
      </tr>`)
    .join("");
  $("panel-tokens").innerHTML =
    `<div class="tok-count">${tokens.filter(t=>t.type!=="EndOfFile").length} tokens</div>
     <table class="tokens">
       <thead><tr><th>Pos</th><th>Type</th><th>Value</th></tr></thead>
       <tbody>${rows}</tbody>
     </table>`;
}

// ---- graphical AST (SVG) ----
const KIND_COLORS = {
  Program:   ["#30363d", "#8b98a9"],
  Block:     ["#30363d", "#8b98a9"],
  VarDecl:   ["#3a2c17", "#ffa657"],
  Assign:    ["#3a2c17", "#ffa657"],
  If:        ["#3a2c17", "#ffa657"],
  While:     ["#3a2c17", "#ffa657"],
  Print:     ["#3a2c17", "#ffa657"],
  FuncDecl:  ["#3a2c17", "#ffa657"],
  Return:    ["#3a2c17", "#ffa657"],
  Call:      ["#132a3f", "#6ea8fe"],
  BinaryOp:  ["#2a1f3d", "#d2a8ff"],
  UnaryOp:   ["#2a1f3d", "#d2a8ff"],
  Identifier:["#0f2f24", "#7ee787"],
  IntLiteral:   ["#12331f", "#56d364"],
  StringLiteral:["#12331f", "#56d364"],
  BoolLiteral:  ["#12331f", "#56d364"],
};

function renderAst(ast) {
  const panel = $("panel-ast");
  if (!ast) {
    panel.innerHTML = `<p class="muted">No AST — the lexer or parser stopped early. Fix the errors below.</p>`;
    return;
  }
  // Geometry.
  const vGap = 74, boxH = 30, marginX = 24, marginY = 24, sibGap = 16;
  const trunc = (s) => (s.length > 22 ? s.slice(0, 21) + "…" : s);
  const boxW = (label) => Math.min(190, Math.max(42, trunc(label).length * 6.8 + 18));

  // Layout: a single left-to-right pass. Each node is centered over its
  // children, but never allowed to overlap the previous node on the same row
  // (per-depth cursor accounts for real box widths — no overlap at any depth).
  let maxDepth = 0;
  const nodes = [], edges = [];
  const prevOnRow = {};   // depth -> {right edge x}
  (function walk(n, depth) {
    maxDepth = Math.max(maxDepth, depth);
    const w = boxW(n.label || "?");
    const kids = n.children || [];
    const node = { label: n.label || "?", kind: n.kind, line: n.line, depth, w };
    let x;
    if (kids.length === 0) {
      x = (prevOnRow[depth] ?? 0) + w / 2;
    } else {
      const cs = kids.map((k) => walk(k, depth + 1));
      const center = (cs[0].x + cs[cs.length - 1].x) / 2;
      const minX = (prevOnRow[depth] ?? 0) + w / 2;
      x = Math.max(center, minX);
      cs.forEach((c) => edges.push([node, c]));
    }
    node.x = x;
    prevOnRow[depth] = x + w / 2 + sibGap;
    nodes.push(node);
    return node;
  })(ast, 0);

  const px = (n) => ({ x: marginX + n.x, y: marginY + n.depth * vGap });
  let maxRight = 0;
  nodes.forEach((n) => { maxRight = Math.max(maxRight, n.x + n.w / 2); });
  const width = marginX * 2 + maxRight;
  const height = marginY * 2 + maxDepth * vGap + boxH;

  // Build SVG.
  let svg = `<svg class="ast" width="${Math.max(width, 200)}" height="${height}"
                  viewBox="0 0 ${Math.max(width, 200)} ${height}">`;
  for (const [p, c] of edges) {
    const a = px(p), b = px(c);
    const y1 = a.y + boxH, y2 = b.y;
    const my = (y1 + y2) / 2;
    svg += `<path class="edge" d="M ${a.x} ${y1} C ${a.x} ${my}, ${b.x} ${my}, ${b.x} ${y2}"/>`;
  }
  for (const n of nodes) {
    const { x, y } = px(n);
    const [fill, stroke] = KIND_COLORS[n.kind] || ["#30363d", "#8b98a9"];
    svg += `<g class="node"><title>${escapeHtml(n.kind)} — line ${n.line}</title>
      <rect x="${x - n.w / 2}" y="${y}" width="${n.w}" height="${boxH}" rx="7"
            fill="${fill}" stroke="${stroke}"/>
      <text class="label" x="${x}" y="${y + boxH / 2 + 4}" text-anchor="middle"
            fill="${stroke}">${escapeHtml(trunc(n.label))}</text>
    </g>`;
  }
  svg += `</svg>`;
  panel.innerHTML = `<div class="ast-hint">Graphical AST — hover a node for its kind &amp; source line. Scroll to pan.</div>
                     <div id="astSvgWrap">${svg}</div>`;
}

function renderSymbols(st) {
  const panel = $("panel-symbols");
  if (!st) { panel.innerHTML = `<p class="muted">No symbol table — semantic analysis did not run.</p>`; return; }
  const vars = st.variables || [];
  const funcs = st.functions || [];
  const varRows = vars.map((v) => `<tr>
      <td class="name">${escapeHtml(v.name)}</td>
      <td class="ty">${escapeHtml(v.type)}</td>
      <td><span class="badge">${escapeHtml(v.kind || "variable")}</span></td>
      <td class="muted">${escapeHtml(v.scope || "")}</td>
      <td class="muted">${v.line || ""}</td>
    </tr>`).join("");
  const funcRows = funcs.map((f) => `<tr>
      <td class="name">${escapeHtml(f.name)}</td>
      <td class="ty">${escapeHtml(f.returnType)}</td>
      <td class="muted">${escapeHtml(f.signature)}</td>
      <td class="muted">${f.line || ""}</td>
    </tr>`).join("");
  panel.innerHTML = `
    <div class="sym-group">
      <h3>Variables &amp; Parameters (${vars.length})</h3>
      ${vars.length ? `<table class="sym">
        <thead><tr><th>Name</th><th>Type</th><th>Kind</th><th>Scope</th><th>Line</th></tr></thead>
        <tbody>${varRows}</tbody></table>` : `<p class="muted">none</p>`}
    </div>
    <div class="sym-group">
      <h3>Functions (${funcs.length})</h3>
      ${funcs.length ? `<table class="sym">
        <thead><tr><th>Name</th><th>Returns</th><th>Signature</th><th>Line</th></tr></thead>
        <tbody>${funcRows}</tbody></table>` : `<p class="muted">none</p>`}
    </div>`;
}

function renderOutput(out) {
  const panel = $("panel-output");
  if (!out.length) { panel.innerHTML = `<div class="output-box output-empty">(no output)</div>`; return; }
  panel.innerHTML = `<div class="output-box">${out.map(escapeHtml).join("\n")}</div>`;
}

function renderOptimizer(opt) {
  const panel = $("panel-optimizer");
  if (!opt) { panel.innerHTML = `<p class="muted">Optimizer did not run (fix errors first).</p>`; return; }
  const before = opt.nodesBefore, after = opt.nodesAfter, removed = opt.removed;
  const pct = before > 0 ? Math.round((removed / before) * 100) : 0;
  panel.innerHTML = `
    <div class="opt-metrics">
      <div class="metric"><div class="num">${before}</div><div class="lbl">nodes before</div></div>
      <div class="metric"><div class="num">${after}</div><div class="lbl">nodes after</div></div>
      <div class="metric good"><div class="num">−${removed}</div><div class="lbl">removed (${pct}%)</div></div>
    </div>
    <div class="opt-bar-wrap">
      <div class="opt-bar"><span style="width:${100 - pct}%"></span></div>
      <div class="opt-sub">Constant folding + dead-code elimination shrank the tree by ${pct}%.</div>
    </div>
    <div class="opt-cols">
      <div><h4>Before</h4><div class="opt-tree">${escapeHtml(astText(opt.astBefore))}</div></div>
      <div><h4>After</h4><div class="opt-tree">${escapeHtml(astText(opt.astAfter))}</div></div>
    </div>`;
}

// Compact indented text of an AST (for the optimizer before/after columns).
function astText(node, prefix = "", last = true, root = true) {
  if (!node) return "";
  let line = prefix + (root ? "" : (last ? "└─ " : "├─ ")) + node.label + "\n";
  const kids = node.children || [];
  const cp = prefix + (root ? "" : (last ? "   " : "│  "));
  kids.forEach((k, i) => { line += astText(k, cp, i === kids.length - 1, false); });
  return line;
}

function renderErrors(errors) {
  const pane = $("errorsPane");
  const list = $("errorsList");
  $("errorsTitle").textContent = errors.length ? `Errors (${errors.length})` : "Errors";
  if (!errors.length) {
    list.innerHTML = `<div class="no-errors">✓ No errors — the program compiled and ran.</div>`;
    return;
  }
  list.innerHTML = errors.map((e, i) => {
    const runtime = e.phase === "runtime";
    return `<div class="error-item ${runtime ? "runtime" : ""}">
      <div class="error-top">
        <span class="error-phase">${escapeHtml(e.phase)}</span>
        <strong>${escapeHtml(e.type)}</strong>
        <span class="error-loc">line ${e.line}${e.col ? ":" + e.col : ""}</span>
      </div>
      <div class="error-msg">${escapeHtml(e.message)}</div>
      ${e.snippet ? `<div class="error-snip">${escapeHtml(e.snippet)}</div>` : ""}
      <div class="error-actions">
        <button class="explain" data-idx="${i}">✨ Explain with AI</button>
        <button class="fix" data-idx="${i}">🔧 Fix it</button>
      </div>
    </div>`;
  }).join("");

  list.querySelectorAll(".explain").forEach((b) =>
    b.addEventListener("click", () => explainError(errors[+b.dataset.idx])));
  list.querySelectorAll(".fix").forEach((b) =>
    b.addEventListener("click", () => runAutoFix(errors[+b.dataset.idx])));
}

// ===========================================================================
// Tabs
// ===========================================================================
function activateTab(name) {
  const tab = document.querySelector(`.tab[data-tab="${name}"]`);
  if (!tab) return;
  document.querySelectorAll(".tab").forEach((t) => t.classList.remove("active"));
  document.querySelectorAll(".panel").forEach((p) => p.classList.remove("active"));
  tab.classList.add("active");
  $("panel-" + name).classList.add("active");
}
document.querySelectorAll(".tab").forEach((tab) => {
  tab.addEventListener("click", () => {
    activateTab(tab.dataset.tab);
    history.replaceState(null, "", "#" + tab.dataset.tab);
  });
});

// ===========================================================================
// Gemini Flash assistant
// ===========================================================================
function getKey() {
  const k = $("apiKey").value.trim();
  if (k) localStorage.setItem("lucid_gemini_key", k);
  return k;
}

function buildPayload(prompt) {
  return { contents: [{ parts: [{ text: prompt }] }] };
}

function offlinePayloadView(title, prompt) {
  const payload = JSON.stringify(buildPayload(prompt), null, 2);
  return `<h3>${title} — no API key set</h3>
    <p class="muted">Enter a Gemini API key above to enable AI. The pipeline still works without it.
    This is the <em>exact</em> request that would be sent:</p>
    <pre>POST ${GEMINI_URL}
header: x-goog-api-key: &lt;YOUR_KEY&gt;

${escapeHtml(payload)}</pre>`;
}

async function geminiCall(prompt, apiKey) {
  const res = await fetch(GEMINI_URL, {
    method: "POST",
    headers: { "Content-Type": "application/json", "x-goog-api-key": apiKey },
    body: JSON.stringify(buildPayload(prompt)),
  });
  if (!res.ok) {
    const body = await res.text();
    throw new Error(`Gemini HTTP ${res.status}. ${body.slice(0, 300)}`);
  }
  const data = await res.json();
  const text = data?.candidates?.[0]?.content?.parts?.[0]?.text;
  if (!text) throw new Error("Gemini returned no text (check model name / key).");
  return text;
}

// Strip ```lang ... ``` fences the model may wrap code in.
function stripFences(s) {
  const m = s.match(/```[a-zA-Z]*\n?([\s\S]*?)```/);
  return (m ? m[1] : s).trim() + "\n";
}

function relevantLines(source, line, radius = 3) {
  const lines = source.split("\n");
  const start = Math.max(0, line - 1 - radius);
  const end = Math.min(lines.length, line - 1 + radius + 1);
  return lines.slice(start, end)
    .map((t, i) => `${start + i + 1}: ${t}`).join("\n");
}

// (1) explainError -> beginner explanation + concrete fix
async function explainError(errorObj) {
  const apiKey = getKey();
  const prompt =
`You are a friendly teaching assistant for a small language called Lucid.
A real C++ compiler has ALREADY diagnosed this error — trust it completely and
do NOT re-diagnose or invent a different error. In 3-4 short sentences, explain
in plain, beginner-friendly English what this error means and why it happened,
then give one concrete fix.

Compiler error (authoritative):
  phase:   ${errorObj.phase}
  type:    ${errorObj.type}
  line:    ${errorObj.line}
  message: ${errorObj.message}

Relevant source lines:
${relevantLines(editor.value, errorObj.line)}

Answer with two short sections titled "Explanation:" and "Fix:".`;

  openModal("✨ Explain: " + errorObj.type,
            `<p><span class="spinner"></span> Asking Gemini…</p>`);
  if (!apiKey) { setModalBody(offlinePayloadView("Explain error", prompt)); return; }
  try {
    const text = await geminiCall(prompt, apiKey);
    setModalBody(renderMarkdownish(text));
  } catch (e) {
    setModalBody(`<h3>AI request failed</h3><p class="muted">${escapeHtml(String(e))}</p>`);
  }
}

// (2) autoFix -> corrected full source
async function autoFix(source, errorObj, apiKey) {
  const prompt =
`You are fixing a program written in "Lucid", a small statically-typed language.
Types are int, string, bool. Statements end with ';'. Blocks use { }.
Functions look like:  func name(int a, string b): int { ... return x; }
print(expr); shows output.  Comments start with //.

A C++ compiler reported this error (authoritative — fix exactly this, change as
little as possible, keep the program's intent):
  ${errorObj.phase} / ${errorObj.type} at line ${errorObj.line}: ${errorObj.message}

Here is the full program:
----
${source}
----
Return ONLY the corrected, complete Lucid program. No explanation, no markdown.`;
  const text = await geminiCall(prompt, apiKey);
  return stripFences(text);
}

// The agent: perceive (error) -> act (ask for fix) -> observe (recompile), x3.
async function runAutoFix(errorObj) {
  const apiKey = getKey();
  openModal("🔧 Auto-fix loop", "");
  if (!apiKey) {
    const prompt = "Fix the Lucid program for: " + errorObj.type + " — " + errorObj.message;
    setModalBody(offlinePayloadView("Auto-fix", prompt));
    return;
  }

  let source = editor.value;
  let currentError = errorObj;
  let doneHtml = "";   // finished attempts, kept as we go

  for (let attempt = 1; attempt <= 3; attempt++) {
    // Perceive/act: show a spinner while we ask Gemini for a fix.
    setModalBody(doneHtml + `<div class="attempt">
      <div class="attempt-head"><span class="spinner"></span> Attempt ${attempt}: asking Gemini to fix
      <em>${escapeHtml(currentError.type)}</em> …</div></div>`);

    let corrected;
    try {
      corrected = await autoFix(source, currentError, apiKey);
    } catch (e) {
      setModalBody(doneHtml + `<div class="attempt fail">
        <div class="attempt-head">⚠️ Attempt ${attempt}: AI request failed</div>
        <p class="muted">${escapeHtml(String(e))}</p></div>`);
      return;
    }

    // Observe: re-validate by re-compiling on the C++ server.
    let result;
    try {
      result = await compileRemote(corrected);
    } catch (e) {
      setModalBody(doneHtml + `<div class="attempt fail">
        <div class="attempt-head">⚠️ Attempt ${attempt}: re-compile failed</div>
        <p class="muted">${escapeHtml(String(e))}</p></div>`);
      return;
    }

    const ok = result.success;
    const next = result.errors[0] || {};
    doneHtml += `<div class="attempt ${ok ? "ok" : "fail"}">
      <div class="attempt-head">${ok ? "✅" : "⚠️"} Attempt ${attempt}: ${ok ? "compiles cleanly!" : "still has errors"}</div>
      <pre>${escapeHtml(corrected)}</pre>
      ${ok ? `<p class="muted">Applied to the editor.</p>`
           : `<p class="muted">Remaining: <strong>${escapeHtml(next.type || "error")}</strong> — ${escapeHtml(next.message || "")}</p>`}
    </div>`;
    setModalBody(doneHtml);

    if (ok) {
      editor.value = corrected;
      updateGutter();
      lastResult = result;
      renderAll(result);
      setStatus("ok", "fixed by AI ✓");
      return;
    }
    source = corrected;
    currentError = result.errors[0] || currentError;
  }
  setModalBody(doneHtml +
    `<p class="muted">Reached the 3-attempt limit. The remaining error may need a human — or a clearer program.</p>`);
}

// (3) explainProgram -> plain-English walkthrough
async function explainProgram() {
  const apiKey = getKey();
  const prompt =
`Explain, in plain English for a beginner, what this program written in the
"Lucid" language does step by step. Keep it concise (a short paragraph or a few
bullets). Do not rewrite the code.

Program:
----
${editor.value}
----`;
  openModal("📖 What this program does", `<p><span class="spinner"></span> Asking Gemini…</p>`);
  if (!apiKey) { setModalBody(offlinePayloadView("Explain program", prompt)); return; }
  try {
    const text = await geminiCall(prompt, apiKey);
    setModalBody(renderMarkdownish(text));
  } catch (e) {
    setModalBody(`<h3>AI request failed</h3><p class="muted">${escapeHtml(String(e))}</p>`);
  }
}

// ===========================================================================
// Modal
// ===========================================================================
function openModal(title, bodyHtml) {
  $("aiTitle").textContent = title;
  $("aiBody").innerHTML = bodyHtml;
  $("aiModal").hidden = false;
}
function setModalBody(html) { $("aiBody").innerHTML = html; }
$("aiClose").addEventListener("click", () => ($("aiModal").hidden = true));
$("aiModal").addEventListener("click", (e) => { if (e.target === $("aiModal")) $("aiModal").hidden = true; });

// ===========================================================================
// Helpers
// ===========================================================================
function escapeHtml(s) {
  return String(s).replace(/[&<>"']/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}

// Very small markdown-ish renderer: **bold**, `code`, headings "X:", newlines.
function renderMarkdownish(text) {
  let html = escapeHtml(text)
    .replace(/\*\*(.+?)\*\*/g, "<strong>$1</strong>")
    .replace(/`([^`]+)`/g, "<code>$1</code>")
    .replace(/^(Explanation|Fix|Summary):/gim, "<h3>$1</h3>")
    .replace(/\n/g, "<br>");
  return html;
}

// ===========================================================================
// Boot
// ===========================================================================
function loadExamples() {
  const sel = $("examples");
  window.LUCID_EXAMPLES.forEach((ex, i) => {
    const opt = document.createElement("option");
    opt.value = i; opt.textContent = ex.name;
    sel.appendChild(opt);
  });
  sel.addEventListener("change", () => {
    editor.value = window.LUCID_EXAMPLES[+sel.value].code;
    updateGutter();
    runCompile();
  });
}

$("runBtn").addEventListener("click", runCompile);
$("explainProgramBtn").addEventListener("click", explainProgram);

(function init() {
  loadExamples();
  const savedKey = localStorage.getItem("lucid_gemini_key");
  if (savedKey) $("apiKey").value = savedKey;
  const params = new URLSearchParams(location.search);
  const exParam = parseInt(params.get("ex"), 10);
  const exIndex = Number.isInteger(exParam) && window.LUCID_EXAMPLES[exParam] ? exParam : 0;
  $("examples").value = exIndex;
  // ?code=<base64> lets you share a program as a link; otherwise load an example.
  let initialCode = window.LUCID_EXAMPLES[exIndex].code;
  const codeParam = params.get("code");
  if (codeParam) { try { initialCode = decodeURIComponent(escape(atob(codeParam))); } catch (e) {} }
  editor.value = initialCode;
  updateGutter();
  const hash = location.hash.replace("#", "");
  if (hash) activateTab(hash);
  runCompile();
})();
