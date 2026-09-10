#!/usr/bin/env python3
"""
build_deck.py — generates Lucid_Review_Deck.pptx

A 12-slide review deck for the Lucid compiler project.
Brief: few words, large type, feature-led. No names, no marks.
Fonts are Arial + Courier New — real system fonts on both macOS and Windows,
so every app (PowerPoint, Keynote, Preview, Slides) renders them identically.

Run:  python3 build_deck.py
"""

from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR

# ---------------------------------------------------------------- design system

SW, SH = 13.333, 7.5
M = 0.70
CW = SW - 2 * M

INK    = RGBColor(0x11, 0x18, 0x27)
BODY   = RGBColor(0x3A, 0x43, 0x57)
MUTED  = RGBColor(0x6B, 0x74, 0x8B)
FAINT  = RGBColor(0xA0, 0xA8, 0xBA)
ACCENT = RGBColor(0x4F, 0x46, 0xE5)
ACC_L  = RGBColor(0xEE, 0xEC, 0xFD)
PAGE   = RGBColor(0xFF, 0xFF, 0xFF)
SURF   = RGBColor(0xF5, 0xF7, 0xFA)
LINE   = RGBColor(0xE4, 0xE8, 0xF0)
DARK   = RGBColor(0x11, 0x18, 0x27)

P1 = RGBColor(0xD9, 0x77, 0x06)
P2 = RGBColor(0x25, 0x63, 0xEB)
P3 = RGBColor(0x7C, 0x3A, 0xED)
P4 = RGBColor(0x0D, 0x94, 0x88)
P5 = RGBColor(0xE1, 0x1D, 0x48)

P1L = RGBColor(0xFD, 0xF1, 0xE0)
P2L = RGBColor(0xE7, 0xEF, 0xFE)
P3L = RGBColor(0xF2, 0xEB, 0xFE)
P4L = RGBColor(0xE1, 0xF5, 0xF2)
P5L = RGBColor(0xFD, 0xE8, 0xED)
OK   = RGBColor(0x05, 0x96, 0x69)
OK_L = RGBColor(0xE4, 0xF6, 0xED)

F  = "Arial"
FM = "Courier New"

PHASES = [
    ("1", "LEXER",       "text to tokens", P1, P1L),
    ("2", "PARSER",      "tokens to tree", P2, P2L),
    ("3", "SEMANTIC",    "types & scopes", P3, P3L),
    ("4", "OPTIMIZER",   "shrink the tree", P4, P4L),
    ("5", "INTERPRETER", "run it",         P5, P5L),
]

# ------------------------------------------------------------------- primitives


def new_deck():
    prs = Presentation()
    prs.slide_width = Inches(SW)
    prs.slide_height = Inches(SH)
    return prs


def blank(prs):
    s = prs.slides.add_slide(prs.slide_layouts[6])
    f = s.background.fill
    f.solid()
    f.fore_color.rgb = PAGE
    return s


def box(slide, l, t, w, h, fill=None, line=None, lw=1.0,
        shape=MSO_SHAPE.ROUNDED_RECTANGLE, radius=0.05):
    sh = slide.shapes.add_shape(shape, Inches(l), Inches(t), Inches(w), Inches(h))
    sh.shadow.inherit = False
    if shape == MSO_SHAPE.ROUNDED_RECTANGLE:
        try:
            sh.adjustments[0] = radius
        except Exception:
            pass
    if fill is None:
        sh.fill.background()
    else:
        sh.fill.solid()
        sh.fill.fore_color.rgb = fill
    if line is None:
        sh.line.fill.background()
    else:
        sh.line.color.rgb = line
        sh.line.width = Pt(lw)
    tf = sh.text_frame
    tf.word_wrap = True
    tf.margin_left = tf.margin_right = Inches(0.16)
    tf.margin_top = tf.margin_bottom = Inches(0.09)
    return sh


def circle(slide, l, t, d, fill, label, color, size=16):
    c = box(slide, l, t, d, d, fill=fill, shape=MSO_SHAPE.OVAL)
    tf = c.text_frame
    tf.word_wrap = False
    tf.margin_left = tf.margin_right = Inches(0.02)
    tf.vertical_anchor = MSO_ANCHOR.MIDDLE
    para(tf, label, size=size, bold=True, color=color, first=True,
         align=PP_ALIGN.CENTER)
    return c


def tb(slide, l, t, w, h, anchor=MSO_ANCHOR.TOP):
    t_ = slide.shapes.add_textbox(Inches(l), Inches(t), Inches(w), Inches(h))
    tf = t_.text_frame
    tf.word_wrap = True
    tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
    tf.vertical_anchor = anchor
    return tf


def para(tf, text="", size=12, bold=False, color=BODY, font=F, first=False,
         align=PP_ALIGN.LEFT, space_before=0, space_after=0, line_spacing=1.0,
         italic=False):
    p = tf.paragraphs[0] if first else tf.add_paragraph()
    p.alignment = align
    p.space_before = Pt(space_before)
    p.space_after = Pt(space_after)
    p.line_spacing = line_spacing
    runs = text if isinstance(text, list) else [(text, {})]
    for rtext, ov in runs:
        r = p.add_run()
        r.text = rtext
        r.font.size = Pt(ov.get("size", size))
        r.font.bold = ov.get("bold", bold)
        r.font.italic = ov.get("italic", italic)
        r.font.name = ov.get("font", font)
        r.font.color.rgb = ov.get("color", color)
    return p


def header(slide, eyebrow, title):
    tf = tb(slide, M, 0.48, CW, 0.30)
    para(tf, eyebrow.upper(), size=11.5, bold=True, color=ACCENT, first=True)
    tf2 = tb(slide, M, 0.80, CW, 0.74)
    para(tf2, title, size=30, bold=True, color=INK, first=True)
    return 1.76


def footer(slide, n):
    tf = tb(slide, M, 6.94, CW, 0.26)
    para(tf, [("LUCID", {"bold": True}), ("     " + str(n), {"bold": True})],
         size=9.5, color=FAINT, first=True)


def arrow(slide, l, t, w=0.28, h=0.18, color=FAINT):
    a = slide.shapes.add_shape(MSO_SHAPE.RIGHT_ARROW, Inches(l), Inches(t),
                               Inches(w), Inches(h))
    a.shadow.inherit = False
    a.fill.solid()
    a.fill.fore_color.rgb = color
    a.line.fill.background()
    return a


def down_arrow(slide, l, t, w=0.22, h=0.32, color=FAINT):
    a = slide.shapes.add_shape(MSO_SHAPE.DOWN_ARROW, Inches(l), Inches(t),
                               Inches(w), Inches(h))
    a.shadow.inherit = False
    a.fill.solid()
    a.fill.fore_color.rgb = color
    a.line.fill.background()
    return a


def stat(slide, l, t, w, value, label, color=INK, vsize=28, lsize=9.5):
    tf = tb(slide, l, t, w, 0.52)
    para(tf, value, size=vsize, bold=True, color=color, first=True)
    tf2 = tb(slide, l, t + 0.48, w, 0.28)
    para(tf2, label.upper(), size=lsize, bold=True, color=MUTED, first=True)


def phase_strip(slide, l, t, w, h=0.72, label_size=11, show_sub=True, gap=0.24):
    n = len(PHASES)
    bw = (w - gap * (n - 1)) / n
    x = l
    for i, (num, name, sub, col, coll) in enumerate(PHASES):
        box(slide, x, t, bw, h, fill=coll)
        box(slide, x, t, bw, 0.055, fill=col, shape=MSO_SHAPE.RECTANGLE)
        tf = tb(slide, x + 0.08, t + 0.13, bw - 0.16, h - 0.20,
                anchor=MSO_ANCHOR.MIDDLE)
        para(tf, name, size=label_size, bold=True, color=col, first=True,
             align=PP_ALIGN.CENTER)
        if show_sub:
            para(tf, sub, size=label_size - 2.5, color=MUTED,
                 align=PP_ALIGN.CENTER, space_before=2)
        if i < n - 1:
            arrow(slide, x + bw + 0.03, t + h / 2 - 0.08, gap - 0.06, 0.16)
        x += bw + gap
    return bw


def bullets(slide, l, t, w, items, size=14, dot=ACCENT, gap=0.40, dsize=0.085):
    y = t
    for it in items:
        box(slide, l, y + 0.085, dsize, dsize, fill=dot, shape=MSO_SHAPE.OVAL)
        tf = tb(slide, l + 0.28, y - 0.02, w - 0.28, 0.34)
        para(tf, it, size=size, color=BODY, first=True)
        y += gap
    return y


def callout(slide, l, t, w, h, lines, color=ACCENT, fill=ACC_L, size=15):
    box(slide, l, t, w, h, fill=fill)
    box(slide, l, t, 0.06, h, fill=color, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(slide, l + 0.32, t + 0.10, w - 0.60, h - 0.20,
            anchor=MSO_ANCHOR.MIDDLE)
    if isinstance(lines, str):
        lines = [lines]
    for i, ln in enumerate(lines):
        para(tf, ln, size=size, color=INK, bold=True, first=(i == 0),
             line_spacing=1.2, space_after=2)


# ==================================================================== 1 · TITLE

def s01(prs):
    s = blank(prs)
    box(s, 0, 0, SW, 0.10, fill=ACCENT, shape=MSO_SHAPE.RECTANGLE)

    tf = tb(s, M, 1.44, CW, 0.32)
    para(tf, "COMPILER DESIGN  ·  PROJECT REVIEW", size=12.5, bold=True,
         color=ACCENT, first=True)

    tf = tb(s, M, 1.88, CW, 1.50)
    para(tf, "Lucid", size=76, bold=True, color=INK, first=True)

    tf = tb(s, M, 3.32, 11.4, 0.70)
    para(tf, [("A compiler you can ", {}),
              ("see through", {"bold": True, "color": ACCENT}), (".", {})],
         size=30, color=INK, first=True)

    tf = tb(s, M, 4.12, 10.5, 0.50)
    para(tf, "Built from scratch in C++17  ·  live playground  ·  AI assistant",
         size=15, color=MUTED, first=True)

    phase_strip(s, M, 4.96, CW, h=0.80, label_size=11.5)

    for i, (v, k) in enumerate([("5", "phases"), ("3", "types"),
                                ("2", "AI actions"), ("0", "frameworks")]):
        stat(s, M + i * 2.35, 6.18, 2.1, v, k, vsize=25, lsize=9)


# ================================================================== 2 · PROBLEM

def s02(prs):
    s = blank(prs)
    y = header(s, "The problem", "A compiler is a black box")

    lw = 6.10
    bullets(s, M, y + 0.22, lw, [
        "You never see the tokens.",
        "You never see the syntax tree.",
        "You never see the symbol table.",
    ], size=17, dot=P5, gap=0.54, dsize=0.10)

    tf = tb(s, M, y + 2.08, lw, 0.9)
    para(tf, "And when it fails, you get a line number.", size=15, color=MUTED,
         first=True, line_spacing=1.3)
    para(tf, "Never which phase rejected your code, or why.", size=15,
         color=MUTED, line_spacing=1.3)

    callout(s, M, y + 3.16, lw, 1.00,
            ["Internals are invisible.", "Errors are unexplained."], size=16)

    rl = M + lw + 0.60
    rw = CW - lw - 0.60
    cx = rl + rw / 2

    b1 = box(s, cx - 1.95, y + 0.26, 3.90, 0.64, fill=SURF)
    t1 = b1.text_frame
    t1.vertical_anchor = MSO_ANCHOR.MIDDLE
    para(t1, 'int x = "hello";', size=15, font=FM, color=BODY, first=True,
         align=PP_ALIGN.CENTER)

    down_arrow(s, cx - 0.11, y + 1.02, 0.22, 0.34)

    bb = box(s, cx - 1.95, y + 1.50, 3.90, 1.34, fill=DARK)
    t2 = bb.text_frame
    t2.vertical_anchor = MSO_ANCHOR.TOP
    t2.margin_top = Inches(0.24)
    para(t2, "?", size=40, bold=True, color=PAGE, first=True,
         align=PP_ALIGN.CENTER)
    tf = tb(s, cx - 1.95, y + 2.38, 3.90, 0.32)
    para(tf, "5 phases  ·  none of them visible", size=11.5, bold=True,
         color=FAINT, first=True, align=PP_ALIGN.CENTER)

    down_arrow(s, cx - 0.11, y + 2.96, 0.22, 0.34)

    b3 = box(s, cx - 1.95, y + 3.44, 3.90, 0.62, fill=P5L)
    t3 = b3.text_frame
    t3.vertical_anchor = MSO_ANCHOR.MIDDLE
    para(t3, "error: line 1", size=15, font=FM, bold=True, color=P5, first=True,
         align=PP_ALIGN.CENTER)

    footer(s, 2)


# ================================================================= 3 · SOLUTION

def s03(prs):
    s = blank(prs)
    y = header(s, "The solution", "Lucid opens the box")

    tf = tb(s, M, y, 11.6, 0.8)
    para(tf, "A real compiler that shows its own internals — live, as you type.",
         size=21, color=INK, first=True, line_spacing=1.2)

    tf = tb(s, M, y + 1.06, CW, 0.30)
    para(tf, "FIVE PHASES", size=10.5, bold=True, color=MUTED, first=True)
    phase_strip(s, M, y + 1.38, CW, h=0.88, label_size=12)

    tf = tb(s, M, y + 2.54, CW, 0.30)
    para(tf, "EVERY ONE OF THEM VISIBLE", size=10.5, bold=True, color=MUTED,
         first=True)

    tabs = [("Tokens", P1), ("Syntax tree", P2), ("Symbol table", P3),
            ("Optimizer", P4), ("Output", P5)]
    gap = 0.22
    bw = (CW - gap * 4) / 5
    x = M
    for name, col in tabs:
        b = box(s, x, y + 2.88, bw, 0.58, fill=PAGE, line=col, lw=1.25)
        tfb = b.text_frame
        tfb.vertical_anchor = MSO_ANCHOR.MIDDLE
        para(tfb, name, size=12.5, bold=True, color=col, first=True,
             align=PP_ALIGN.CENTER)
        x += bw + gap

    callout(s, M, y + 3.80, CW, 0.86,
            "Type a character — every panel refreshes in 300 milliseconds.",
            size=16)
    footer(s, 3)


# =============================================================== 4 · OBJECTIVES

def s04(prs):
    s = blank(prs)
    y = header(s, "Objectives", "What we set out to build")

    objs = [
        ("Build it by hand", "No Lex. No Yacc. No frameworks.", P1),
        ("Show every stage", "Each phase renders its own output.", P2),
        ("Name the phase that failed", "Lexical, syntax, semantic or runtime.", P3),
        ("Optimise and prove it", "A measured before and after count.", P4),
        ("Report all errors at once", "Never stop at the first mistake.", P5),
        ("AI explains, never decides", "The compiler has the final say.", ACCENT),
    ]
    cw = (CW - 2 * 0.30) / 3
    ch = 1.74
    for i, (t_, d, col) in enumerate(objs):
        r, c = divmod(i, 3)
        l = M + c * (cw + 0.30)
        t = y + 0.06 + r * (ch + 0.34)
        box(s, l, t, cw, ch, fill=SURF)
        box(s, l, t, 0.06, ch, fill=col, shape=MSO_SHAPE.RECTANGLE)
        tf = tb(s, l + 0.32, t + 0.34, cw - 0.60, 0.46)
        para(tf, t_, size=15.5, bold=True, color=INK, first=True,
             line_spacing=1.1)
        tf2 = tb(s, l + 0.32, t + 0.94, cw - 0.60, 0.62)
        para(tf2, d, size=12, color=MUTED, first=True, line_spacing=1.2)
    footer(s, 4)


# ==================================================================== 5 · SCOPE

def s05(prs):
    s = blank(prs)
    y = header(s, "Scope", "Where we drew the line")

    gw = (CW - 0.44) / 2

    box(s, M, y, gw, 3.46, fill=OK_L)
    box(s, M, y, gw, 0.07, fill=OK, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(s, M + 0.34, y + 0.32, gw - 0.68, 0.36)
    para(tf, "WE BUILT", size=13.5, bold=True, color=OK, first=True)
    bullets(s, M + 0.34, y + 0.88, gw - 0.68, [
        "A statically-typed language",
        "All five phases, hand-written",
        "Type checking and a symbol table",
        "Constant folding and dead-code removal",
        "Four kinds of error, clearly named",
        "A playground and a command-line tool",
    ], size=13.5, dot=OK, gap=0.40)

    l2 = M + gw + 0.44
    box(s, l2, y, gw, 3.46, fill=SURF)
    box(s, l2, y, gw, 0.07, fill=MUTED, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(s, l2 + 0.34, y + 0.32, gw - 0.68, 0.36)
    para(tf, "WE DID NOT", size=13.5, bold=True, color=MUTED, first=True)
    bullets(s, l2 + 0.34, y + 0.88, gw - 0.68, [
        "Generate machine code",
        "Emit three-address code",
        "Support floats, arrays or classes",
        "Add modules or imports",
        "Deploy it to the web",
    ], size=13.5, dot=FAINT, gap=0.40)

    callout(s, M, y + 3.70, CW, 0.88,
            "Four phases done thoroughly beats six phases done shallowly.",
            size=16)
    footer(s, 5)


# ============================================================= 6 · ARCHITECTURE

def s06(prs):
    s = blank(prs)
    y = header(s, "Architecture", "Three layers, one direction")

    dw = 8.20

    t1 = y + 0.14
    box(s, M, t1, dw, 1.08, fill=P2L)
    box(s, M, t1, 0.06, 1.08, fill=P2, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(s, M + 0.34, t1 + 0.20, dw - 0.66, 0.74)
    para(tf, "PLAYGROUND", size=11, bold=True, color=P2, first=True)
    para(tf, "Browser · five live panels · AI assistant", size=14.5, color=INK,
         space_before=4)

    ay = t1 + 1.28
    down_arrow(s, M + 0.55, ay, 0.24, 0.44)
    tf = tb(s, M + 1.10, ay + 0.06, dw - 1.4, 0.36)
    para(tf, "one request  ·  one JSON reply", size=12, font=FM, color=MUTED,
         first=True)

    t2 = ay + 0.64
    box(s, M, t2, dw, 0.94, fill=P4L)
    box(s, M, t2, 0.06, 0.94, fill=P4, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(s, M + 0.34, t2 + 0.16, dw - 0.66, 0.68)
    para(tf, "SERVER", size=11, bold=True, color=P4, first=True)
    para(tf, "A thin C++ shell — no compiler logic", size=14.5, color=INK,
         space_before=4)

    t3 = t2 + 1.16
    box(s, M, t3, dw, 1.54, fill=P3L)
    box(s, M, t3, 0.06, 1.54, fill=P3, shape=MSO_SHAPE.RECTANGLE)
    tf = tb(s, M + 0.34, t3 + 0.16, dw - 0.66, 0.40)
    para(tf, "THE COMPILER", size=11, bold=True, color=P3, first=True)
    phase_strip(s, M + 0.34, t3 + 0.60, dw - 0.68, h=0.70, label_size=9,
                show_sub=False, gap=0.14)

    rl = M + dw + 0.50
    rw = CW - dw - 0.50
    notes = [
        ("Pure C++17", "No network. No framework."),
        ("Runs two ways", "In a browser, or in a terminal."),
        ("AI sits outside", "Remove it — nothing breaks."),
    ]
    yy = y + 0.14
    for t_, d in notes:
        box(s, rl, yy, rw, 1.30, fill=PAGE, line=LINE, lw=1.0)
        tf = tb(s, rl + 0.28, yy + 0.30, rw - 0.56, 0.38)
        para(tf, t_, size=14.5, bold=True, color=INK, first=True)
        tf2 = tb(s, rl + 0.28, yy + 0.76, rw - 0.56, 0.44)
        para(tf2, d, size=11.5, color=MUTED, first=True, line_spacing=1.15)
        yy += 1.46
    footer(s, 6)


# ================================================================= 7 · PIPELINE

def s07(prs):
    s = blank(prs)
    y = header(s, "How it works", "Source in, five stages, output out")

    n, gap = 5, 0.24
    bw = (CW - gap * (n - 1)) / n
    detail = [
        ("Reads the text", "Splits it into tokens."),
        ("Builds the tree", "Checks the grammar."),
        ("Checks meaning", "Types, names and scopes."),
        ("Shrinks the tree", "Removes dead code."),
        ("Runs the program", "Prints the output."),
    ]
    x = M
    for i, (num, name, sub, col, coll) in enumerate(PHASES):
        a, b = detail[i]
        h = 2.50
        box(s, x, y + 0.18, bw, h, fill=PAGE, line=LINE, lw=1.0)
        box(s, x, y + 0.18, bw, 0.08, fill=col, shape=MSO_SHAPE.RECTANGLE)
        circle(s, x + 0.26, y + 0.50, 0.52, coll, num, col, size=17)
        tf = tb(s, x + 0.26, y + 1.18, bw - 0.52, 0.42)
        para(tf, name, size=14, bold=True, color=INK, first=True)
        tf2 = tb(s, x + 0.26, y + 1.64, bw - 0.52, 0.92)
        para(tf2, a, size=12, color=BODY, first=True, line_spacing=1.18,
             space_after=5)
        para(tf2, b, size=12, color=MUTED, line_spacing=1.18)
        if i < n - 1:
            arrow(s, x + bw + 0.03, y + 1.38, gap - 0.06, 0.16)
        x += bw + gap

    callout(s, M, y + 2.98, CW, 0.90,
            "Break your code and the pipeline stops at the phase that rejected "
            "it — you see exactly where.", size=15)
    footer(s, 7)


# ================================================================= 8 · CONCEPTS

def s08(prs):
    s = blank(prs)
    y = header(s, "Compiler concepts", "The theory, running")

    rows = [
        ("Tokens and maximal munch", "Lexer", P1),
        ("Grammar and recursive descent", "Parser", P2),
        ("Operator precedence", "Parser", P2),
        ("Panic-mode error recovery", "Parser", P2),
        ("Symbol table and scopes", "Semantic", P3),
        ("Static type checking", "Semantic", P3),
        ("Constant folding", "Optimizer", P4),
        ("Dead-code elimination", "Optimizer", P4),
        ("Activation records", "Interpreter", P5),
    ]
    gw = (CW - 0.36) / 2
    rh = 0.54
    for i, (concept, where, col) in enumerate(rows):
        c = 0 if i < 5 else 1
        r = i if i < 5 else i - 5
        l = M + c * (gw + 0.36)
        t = y + 0.14 + r * (rh + 0.16)
        box(s, l, t, gw, rh, fill=SURF)
        box(s, l, t, 0.055, rh, fill=col, shape=MSO_SHAPE.RECTANGLE)
        tf = tb(s, l + 0.30, t, gw - 2.35, rh, anchor=MSO_ANCHOR.MIDDLE)
        para(tf, concept, size=13.5, bold=True, color=INK, first=True)
        tf2 = tb(s, l + gw - 1.95, t, 1.68, rh, anchor=MSO_ANCHOR.MIDDLE)
        para(tf2, where, size=11.5, bold=True, color=col, first=True,
             align=PP_ALIGN.RIGHT)

    callout(s, M + gw + 0.36, y + 3.64, gw, 1.10,
            "Implemented — and visible while it runs.", size=15)
    footer(s, 8)


# ================================================================= 9 · FEATURES

def s09(prs):
    s = blank(prs)
    y = header(s, "Features", "What makes it different")

    feats = [
        ("AI", "AI explains errors", "Plain English, on demand.", ACCENT, ACC_L),
        ("AI", "AI fixes your code", "Then the compiler re-checks it.", ACCENT, ACC_L),
        ("< >", "Live playground", "Five panels, updating as you type.", P2, P2L),
        ("AST", "Visual syntax tree", "Watch the tree take shape.", P3, P3L),
        ("55%", "Real optimisation", "38 nodes become 17.", P4, P4L),
        ("?", "Did you mean...?", "Spelling help on unknown names.", P5, P5L),
    ]
    cw = (CW - 2 * 0.30) / 3
    ch = 2.14
    for i, (icon, title, line, col, coll) in enumerate(feats):
        r, c = divmod(i, 3)
        l = M + c * (cw + 0.30)
        t = y + 0.06 + r * (ch + 0.30)
        box(s, l, t, cw, ch, fill=PAGE, line=LINE, lw=1.0)
        box(s, l, t, cw, 0.075, fill=col, shape=MSO_SHAPE.RECTANGLE)
        b = box(s, l + 0.30, t + 0.32, 0.72, 0.46, fill=coll)
        btf = b.text_frame
        btf.word_wrap = False
        btf.margin_left = btf.margin_right = Inches(0.02)
        btf.vertical_anchor = MSO_ANCHOR.MIDDLE
        para(btf, icon, size=13, bold=True, color=col, first=True,
             align=PP_ALIGN.CENTER)
        tf = tb(s, l + 0.30, t + 0.94, cw - 0.60, 0.42)
        para(tf, title, size=15.5, bold=True, color=INK, first=True)
        tf2 = tb(s, l + 0.30, t + 1.42, cw - 0.60, 0.60)
        para(tf2, line, size=12, color=MUTED, first=True, line_spacing=1.2)
    footer(s, 9)


# ======================================================================= 10 · AI

def s10(prs):
    s = blank(prs)
    y = header(s, "The AI assistant", "It explains. It never decides.")

    steps = [
        ("1", "SEND", ["Your broken code", "and our error"], P2, P2L),
        ("2", "SUGGEST", ["The AI proposes", "a correction"], P3, P3L),
        ("3", "VERIFY", ["Our compiler", "re-checks it"], P4, P4L),
    ]
    gap = 0.44
    bw = (CW - gap * 2) / 3
    x = M
    for i, (num, name, desc, col, coll) in enumerate(steps):
        box(s, x, y + 0.24, bw, 1.94, fill=PAGE, line=col, lw=1.5)
        circle(s, x + 0.34, y + 0.54, 0.56, coll, num, col, size=18)
        tf = tb(s, x + 0.34, y + 1.24, bw - 0.68, 0.38)
        para(tf, name, size=15, bold=True, color=INK, first=True)
        tf2 = tb(s, x + 0.34, y + 1.64, bw - 0.68, 0.52)
        for j, ln in enumerate(desc):
            para(tf2, ln, size=12, color=MUTED, first=(j == 0), line_spacing=1.18)
        if i < 2:
            arrow(s, x + bw + 0.08, y + 1.12, gap - 0.16, 0.20)
        x += bw + gap

    tf = tb(s, M, y + 2.40, CW, 0.34)
    para(tf, "Repeats up to three times — until it compiles.", size=13,
         color=MUTED, first=True, align=PP_ALIGN.CENTER)

    callout(s, M, y + 3.00, CW, 1.06,
            ["The AI never decides whether your code is correct.",
             "That verdict always comes from the compiler."], size=16)

    tf = tb(s, M, y + 4.24, CW, 0.34)
    para(tf, "Optional — the whole project works without it.", size=13,
         italic=True, color=MUTED, first=True)
    footer(s, 10)


# ===================================================================== 11 · DEMO

def s11(prs):
    s = blank(prs)
    y = header(s, "Demo", "What you will see")

    steps = [
        ("Write a program", "Tokens and the tree appear instantly.", P1),
        ("Delete a semicolon", "It stops at the syntax stage.", P2),
        ("Break the types", "Three errors, all reported at once.", P3),
        ("Misspell a name", "It asks: did you mean 'count'?", P3),
        ("Run the optimizer", "38 nodes become 17, side by side.", P4),
        ("Press Fix it", "The AI repairs it, the compiler confirms.", ACCENT),
    ]
    rh = 0.64
    for i, (t_, d, col) in enumerate(steps):
        t = y + 0.16 + i * (rh + 0.12)
        box(s, M, t, CW, rh, fill=SURF)
        circle(s, M + 0.30, t + 0.12, 0.40, col, str(i + 1), PAGE, size=13)
        tf = tb(s, M + 0.94, t, 3.90, rh, anchor=MSO_ANCHOR.MIDDLE)
        para(tf, t_, size=15, bold=True, color=INK, first=True)
        tf2 = tb(s, M + 4.94, t, CW - 5.24, rh, anchor=MSO_ANCHOR.MIDDLE)
        para(tf2, d, size=14, color=MUTED, first=True)
    footer(s, 11)


# ================================================================== 12 · CLOSING

def s12(prs):
    s = blank(prs)
    box(s, 0, 0, SW, 0.10, fill=ACCENT, shape=MSO_SHAPE.RECTANGLE)

    tf = tb(s, M, 2.38, CW, 0.36)
    para(tf, "THANK YOU", size=13, bold=True, color=ACCENT, first=True)

    tf = tb(s, M, 2.86, 11.8, 1.5)
    para(tf, [("Lucid", {"bold": True, "color": INK}),
              (" makes a compiler visible while it runs.", {})],
         size=34, color=INK, first=True, line_spacing=1.2)

    phase_strip(s, M, 4.76, CW, h=0.80, label_size=11.5)

    tf = tb(s, M, 5.90, CW, 0.40)
    para(tf, "Questions", size=15, bold=True, color=MUTED, first=True)


# ======================================================================= build

def main():
    prs = new_deck()
    for fn in (s01, s02, s03, s04, s05, s06, s07, s08, s09, s10, s11, s12):
        fn(prs)
    out = "Lucid_Review_Deck.pptx"
    prs.save(out)
    print("wrote %s  (%d slides)" % (out, len(prs.slides._sldIdLst)))


if __name__ == "__main__":
    main()
