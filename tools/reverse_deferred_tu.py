#!/usr/bin/env python3
"""reverse_deferred_tu.py IN.c OUT.c: reverse a unit's function definitions for `-inline deferred`.

MWCC `-inline deferred` parses the whole unit and emits functions in reverse
source order, so a unit whose retail `.text` shows that signature (see
tools/deferred_scan.py) needs its definitions in reverse `.text` order.
Top-level declarations and `static inline` definitions keep their relative
order; a prototype is left where each moved definition was (unless already
declared); a `#pragma X on|off` ... `#pragma X reset` pair wrapping a single
function travels with it; `#undef` lines move after the definitions. Output is
a candidate to measure with objdiff, not a guaranteed match.
"""
import re, sys

src = open(sys.argv[1], encoding='utf-8').read()
n = len(src)
chunks = []  # (kind, text)
i = 0
start = 0
depth = 0
body_open = None  # index of first depth-0 '{' in current chunk

def skip_ws_comments(j):
    while j < n:
        if src[j].isspace():
            j += 1
        elif src.startswith('/*', j):
            j = src.index('*/', j) + 2
        elif src.startswith('//', j):
            j = src.index('\n', j)
        else:
            break
    return j

def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', ' ', t, flags=re.S)
    return re.sub(r'//[^\n]*', ' ', t)

while i < n:
    c = src[i]
    if src.startswith('/*', i):
        i = src.index('*/', i) + 2; continue
    if src.startswith('//', i):
        i = src.index('\n', i); continue
    if c in '"\'':
        q = c; i += 1
        while src[i] != q:
            i += 2 if src[i] == '\\' else 1
        i += 1; continue
    if depth == 0 and c == '#' and (i == 0 or src[src.rfind('\n', 0, i) + 1:i].strip() == ''):
        # preprocessor line (with continuations) is its own chunk
        head = src[start:src.rfind('\n', 0, i) + 1]
        if head.strip():
            chunks.append(('decl', head))
        elif head:
            pass
        lead = head if not head.strip() else ''
        j = i
        while True:
            e = src.index('\n', j)
            if src[e - 1] == '\\':
                j = e + 1; continue
            break
        chunks.append(('pp', lead + src[i:e + 1]))
        i = start = e + 1
        continue
    if c == '{':
        if depth == 0:
            body_open = i
        depth += 1
    elif c == '}':
        depth -= 1
        if depth == 0:
            head = strip_comments(src[start:body_open]).strip()
            j = skip_ws_comments(i + 1)
            nxt = src[j] if j < n else ''
            if head.endswith(')') and '=' not in head.split('(')[0] and nxt != ';':
                e = src.find('\n', i)
                e = n if e < 0 else e + 1
                rest = src[i + 1:e].strip()
                if rest:
                    e = i + 1
                text = src[start:e] + ('' if src[e - 1] == '\n' else '\n')
                inline = re.search(r'\binline\b', head) is not None
                chunks.append(('inline' if inline else 'func', text))
                i = start = e; body_open = None
                continue
    elif c == ';' and depth == 0:
        e = src.find('\n', i)
        e = n if e < 0 else e + 1
        chunks.append(('decl', src[start:e]))
        i = start = e; body_open = None
        continue
    i += 1
tail = src[start:]

# attach single-function pragma pairs
units = []
k = 0
while k < len(chunks):
    kind, text = chunks[k]
    if kind == 'pp' and re.match(r'\s*#pragma\s+\w+\s+(on|off)\b', text) and k + 2 < len(chunks) \
            and chunks[k + 1][0] == 'func' and chunks[k + 2][0] == 'pp' \
            and re.match(r'\s*#pragma\s+\w+\s+reset\b', chunks[k + 2][1]):
        units.append(('func', text + chunks[k + 1][1] + chunks[k + 2][1], chunks[k + 1][1]))
        k += 3; continue
    units.append((kind, text, text))
    k += 1

def prototype(ftext):
    body = strip_comments(ftext)
    depth = 0
    for p, ch in enumerate(body):
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        elif ch == '{' and depth == 0:
            break
    return ' '.join(body[:p].split()) + ';\n'

def fname(ftext):
    proto = prototype(ftext)
    return re.search(r'(\w+)\s*\((?!\s*\*)', proto).group(1), proto

declared = set()
prefix = []
funcs = []
for kind, text, ftext in units:
    if kind == 'func':
        name, proto = fname(ftext)
        if name not in declared:
            prefix.append(proto)
            declared.add(name)
        funcs.append(text)
    else:
        prefix.append(text)
        for m in re.finditer(r'\b(\w+)\s*\(', strip_comments(text)):
            declared.add(m.group(1))
undefs = [x for x in prefix if re.match(r'\s*#undef\b', x)]
prefix = [x for x in prefix if x not in undefs]
NOTE = """
/*
 * Retail builds this unit with -inline noauto,deferred, which emits functions
 * in reverse source order. The definitions below are therefore in reverse of
 * the retail .text order; that order also fixes the pooled-string layout and
 * the anonymous .rodata initializer order.
 */
"""
out = ''.join(prefix) + NOTE + '\n' + ''.join(reversed(funcs)) + ''.join(undefs) + tail
open(sys.argv[2], 'w', encoding='utf-8').write(out)
print(f"{len(funcs)} functions reversed", file=sys.stderr)
