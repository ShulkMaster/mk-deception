#!/usr/bin/env python3
"""Cross-check GameCube unit source against the PS2 release (SLUS_208.81).

The PS2 ELF has no symbols, so a unit's functions must be found before they
can be compared. For each candidate unit this tool:

  1. compiles the unit's GameCube source with mwccps2, keeping the unit's
     front-end flags (-O, -inline, -str, -opt, -lang, pragmas) and dropping
     PowerPC-only ones;
  2. finds the unit's functions in the PS2 text. Functions that match exactly
     once relocated fields are masked anchor the unit; an order-preserving
     alignment by instruction similarity places the functions between and
     around the anchors;
  3. harvests the PS2 addresses of called functions and referenced data from
     the relocations of placed functions, across all units, and repeats the
     search with those addresses as extra anchors;
  4. splits each placed range with tools/ps2_split.py, assembles it, and
     scores it with objdiff twice: structurally (relocation targets ignored)
     and with relocation names compared.

Units are ranked by the structural score. --write-config N writes the top N
units into config/SLUS_208.81/splits.txt and symbols.txt.

Usage:
    python3 tools/ps2_cross.py                    # all src/game/ units
    python3 tools/ps2_cross.py --filter src/mw/mwMem.c
    python3 tools/ps2_cross.py --top 10 --write-config 10

Requires the PS2 tools that `configure.py --version SLUS_208.81` downloads
(mwccps2, PS2 binutils) and tools/requirements-ps2.txt plus numpy.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from difflib import SequenceMatcher
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import ps2_split  # noqa: E402

VERSION = "SLUS_208.81"
ELF_SHA1 = "7657eff03bbfd816c541693812ab6bea076f0cf9"
COMPILER = "mwcps2-3.0.1b87-031208"

R_MIPS_32 = 2
R_MIPS_26 = 4
R_MIPS_HI16 = 5
R_MIPS_LO16 = 6
R_MIPS_GPREL16 = 7
R_MIPS_LITERAL = 8

JR_RA = 0x03E00008

# GameCube compiler options that take a separate value argument.
VALUE_FLAGS = {
    "-pragma", "-inline", "-str", "-opt", "-enum", "-fp", "-proc", "-align",
    "-Cpp_exceptions", "-RTTI", "-fp_contract", "-use_lmw_stmw", "-maxerrors",
    "-sdata", "-sdata2", "-char", "-common", "-sym", "-W", "-ipa",
    "-func_align", "-schedule", "-bool", "-wchar_t", "-warn", "-pooldata",
}
# PowerPC back-end options with no PS2 meaning.
DROP_FLAGS = {
    "-proc", "-align", "-fp", "-fp_contract", "-use_lmw_stmw", "-sdata",
    "-sdata2", "-multibyte", "-schedule", "-maxerrors", "-sym", "-func_align",
    "-pooldata",
}
# Flag sets tried on top of the translated GameCube flags. The PS2 build's own
# per-unit flags are unknown; mwMem matches best with "-O4,p -inline on".
VARIANTS: Dict[str, List[str]] = {
    "gc": [],
    "O4p": ["-O4,p"],
    "O4s": ["-O4,s"],
    "inline-on": ["-inline", "on"],
    "O4p+inline-on": ["-O4,p", "-inline", "on"],
    "inline-auto": ["-inline", "auto"],
}
DEFAULT_VARIANTS = "gc,O4p"

INCLUDE_DIRS = [
    f"config/{VERSION}/include",
    "include",
    "build/GQNE5D/include",
    "extern/gameware/rw3.6.0.3/include",
]


def ps2_cflags(gc_flags: str) -> List[str]:
    """Translate a unit's GameCube c_flags into mwccps2 flags."""
    out: List[str] = []
    for inc in INCLUDE_DIRS:
        out += ["-i", inc]
    toks = shlex.split(gc_flags)
    i = 0
    while i < len(toks):
        tok = toks[i]
        value = None
        if tok in VALUE_FLAGS and i + 1 < len(toks):
            value = toks[i + 1]
            i += 2
        else:
            i += 1
        if tok in DROP_FLAGS:
            continue
        if tok == "-DVERSION_GQNE5D":
            tok = "-DVERSION_SLUS_208_81"
        out.append(tok)
        if value is not None:
            out.append(value)
    return out


###
# Relocatable object reader (MIPS, 32-bit little-endian)
###


@dataclass
class ObjSym:
    name: str
    value: int
    size: int
    kind: int  # STT_*
    bind: int  # STB_*
    shndx: int


@dataclass
class ObjSection:
    index: int
    name: str
    sh_type: int
    data: bytes
    relocs: List[Tuple[int, int, int]] = field(default_factory=list)  # offset, type, sym index


def read_obj(path: Path) -> Tuple[List[ObjSection], List[ObjSym]]:
    data = path.read_bytes()
    e_shoff = struct.unpack_from("<I", data, 0x20)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    headers = [
        struct.unpack_from("<IIIIIIIIII", data, e_shoff + i * e_shentsize)
        for i in range(e_shnum)
    ]
    shstr = headers[e_shstrndx]
    strtab_data = data[shstr[4]:shstr[4] + shstr[5]]

    def cstr(buf: bytes, off: int) -> str:
        end = buf.index(b"\0", off)
        return buf[off:end].decode("latin-1")

    sections = []
    for i, h in enumerate(headers):
        body = data[h[4]:h[4] + h[5]] if h[1] != 8 else b""  # SHT_NOBITS
        sections.append(ObjSection(i, cstr(strtab_data, h[0]) if i else "", h[1], body))
    symbols: List[ObjSym] = []
    for i, h in enumerate(headers):
        if h[1] != 2:  # SHT_SYMTAB
            continue
        names = headers[h[6]]
        names_data = data[names[4]:names[4] + names[5]]
        for off in range(h[4], h[4] + h[5], 16):
            st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from(
                "<IIIBBH", data, off
            )
            symbols.append(
                ObjSym(cstr(names_data, st_name), st_value, st_size,
                       st_info & 0xF, st_info >> 4, st_shndx)
            )
    for h in headers:
        if h[1] != 9:  # SHT_REL
            continue
        target = sections[h[7]]
        for off in range(h[4], h[4] + h[5], 8):
            r_offset, r_info = struct.unpack_from("<II", data, off)
            target.relocs.append((r_offset, r_info & 0xFF, r_info >> 8))
    return sections, symbols


###
# Instruction helpers
###


def tokens(words: np.ndarray) -> np.ndarray:
    """Opcode-and-register view of instructions, immediates and targets masked."""
    op = words >> 26
    rtype = (op == 0) | (op == 0x1C) | (op == 0x10) | (op == 0x12)
    cop1 = op == 0x11
    bc1 = cop1 & (((words >> 21) & 0x1F) == 8)
    jtype = (op == 2) | (op == 3)
    keep_all = rtype | (cop1 & ~bc1)
    return np.where(
        keep_all, words, np.where(jtype, words & 0xFC000000, words & 0xFFFF0000)
    ).astype(np.uint32)


def optokens(words: np.ndarray) -> np.ndarray:
    """Register-free opcode view: major opcode plus function/format fields."""
    op = words >> 26
    funct = words & 0x3F
    fmt = (words >> 21) & 0x1F
    regimm = (words >> 16) & 0x1F
    sub = np.where(
        (op == 0) | (op == 0x1C), funct,
        np.where(op == 1, regimm, np.where((op >= 0x10) & (op <= 0x12), fmt * 64 + funct, 0)),
    )
    return (op * 4096 + sub).astype(np.uint32)


def sext16(v: int) -> int:
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


@dataclass
class Reloc:
    index: int  # word index in the function
    rtype: int
    sym: ObjSym


@dataclass
class Func:
    name: str
    order: int
    words: np.ndarray
    mask: np.ndarray  # bits compared in an exact match
    relocs: List[Reloc]
    local: bool
    tok: Tuple[int, ...] = ()
    otok: Tuple[int, ...] = ()

    @property
    def size(self) -> int:
        return len(self.words) * 4


def unit_functions(sections: List[ObjSection], symbols: List[ObjSym]) -> List[Func]:
    funcs = []
    by_section: Dict[int, List[ObjSym]] = defaultdict(list)
    for sym in symbols:
        if sym.kind == 2 and sym.shndx not in (0, 0xFFF1, 0xFFF2) and sym.size > 0:
            by_section[sym.shndx].append(sym)
    order = 0
    for shndx in sorted(by_section):
        sec = sections[shndx]
        for sym in sorted(by_section[shndx], key=lambda s: s.value):
            raw = sec.data[sym.value:sym.value + sym.size]
            words = np.frombuffer(raw[: len(raw) // 4 * 4], dtype="<u4").astype(np.uint32)
            mask = np.full(len(words), 0xFFFFFFFF, dtype=np.uint32)
            relocs = []
            for r_offset, rtype, symidx in sec.relocs:
                if not sym.value <= r_offset < sym.value + sym.size:
                    continue
                idx = (r_offset - sym.value) // 4
                if rtype == R_MIPS_26:
                    mask[idx] &= 0xFC000000
                elif rtype in (R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16, R_MIPS_LITERAL):
                    mask[idx] &= 0xFFFF0000
                else:
                    continue
                relocs.append(Reloc(idx, rtype, symbols[symidx]))
            relocs.sort(key=lambda r: r.index)
            func = Func(sym.name, order, words, mask, relocs, sym.bind == 0)
            func.tok = tuple(tokens(words).tolist())
            func.otok = tuple(optokens(words).tolist())
            funcs.append(func)
            order += 1
    return funcs


###
# PS2 image
###


class Ps2Image:
    def __init__(self, elf_path: Path):
        raw = elf_path.read_bytes()
        digest = hashlib.sha1(raw).hexdigest()
        if digest != ELF_SHA1:
            sys.exit(f"{elf_path}: SHA-1 {digest} does not match {ELF_SHA1}")
        self.elf = ps2_split.read_elf(elf_path)
        if self.elf.gp is None:
            sys.exit(f"{elf_path}: no .reginfo gp value")
        self.gp = self.elf.gp
        seg = self.elf.segments[0]
        self.base = seg.vaddr
        allw = np.frombuffer(raw[seg.offset:seg.offset + seg.filesz // 4 * 4], dtype="<u4")
        self.all_words = allw.astype(np.uint32)
        self.file_end = seg.vaddr + seg.filesz

        # Text ends after the last return of the contiguous code run.
        rets = np.nonzero(self.all_words == JR_RA)[0]
        gaps = np.nonzero(np.diff(rets) > 0x4000)[0]
        last = rets[gaps[0]] if len(gaps) else rets[-1]
        self.text_words = int((last + 2 + 3) // 4 * 4)
        self.text_end = self.base + self.text_words * 4
        self.words = self.all_words[: self.text_words]
        self.tok = tokens(self.words)
        self.otok = optokens(self.words)
        self.starts = self._function_starts()
        self.start_index = {int(s): i for i, s in enumerate(self.starts)}
        self.ends = self._function_ends()
        self._tok_cache: Dict[int, Tuple[int, ...]] = {}
        self._otok_cache: Dict[int, Tuple[int, ...]] = {}
        # Exact-search index on the first word (raw and with the low half masked).
        first = self.words[(self.starts - self.base) // 4]
        self.by_first: Dict[int, List[int]] = defaultdict(list)
        self.by_first_hi: Dict[int, List[int]] = defaultdict(list)
        for s, w in zip(self.starts.tolist(), first.tolist()):
            self.by_first[w].append(s)
            self.by_first_hi[w & 0xFFFF0000].append(s)

    def _function_starts(self) -> np.ndarray:
        w = self.words
        n = len(w)
        starts = {self.base}
        jal = np.nonzero((w >> 26) == 3)[0]
        targets = (w[jal] & 0x3FFFFFF) << 2
        for t in targets.tolist():
            if self.base <= t < self.text_end:
                starts.add(t)
        # A function follows a return (or jump) and its delay slot, after zero padding.
        ends = np.nonzero((w == JR_RA) | ((w >> 26) == 2))[0]
        nonzero = np.nonzero(w)[0]
        for r in ends.tolist():
            p = r + 2
            k = np.searchsorted(nonzero, p)
            if k >= len(nonzero):
                continue
            q = int(nonzero[k])
            if q % 2 == 0 and q < n:
                starts.add(self.base + q * 4)
        return np.array(sorted(starts), dtype=np.int64)

    def _function_ends(self) -> np.ndarray:
        """End of each candidate function, trailing zero padding trimmed."""
        ends = np.empty(len(self.starts), dtype=np.int64)
        nxt = np.append(self.starts[1:], self.text_end)
        for i, (s, e) in enumerate(zip(self.starts.tolist(), nxt.tolist())):
            a, b = (s - self.base) // 4, (e - self.base) // 4
            while b > a + 1 and self.words[b - 1] == 0:
                b -= 1
            ends[i] = self.base + b * 4
        return ends

    def end_of(self, start: int) -> int:
        i = self.start_index.get(start)
        if i is not None:
            return int(self.ends[i])
        k = int(np.searchsorted(self.starts, start, side="right"))
        return int(self.ends[k - 1]) if k else start

    def words_at(self, addr: int, count: int) -> np.ndarray:
        a = (addr - self.base) // 4
        return self.words[a:a + count]

    def tok_of(self, start: int) -> Tuple[int, ...]:
        cached = self._tok_cache.get(start)
        if cached is None:
            a, b = (start - self.base) // 4, (self.end_of(start) - self.base) // 4
            cached = tuple(self.tok[a:b].tolist())
            self._tok_cache[start] = cached
        return cached

    def otok_of(self, start: int) -> Tuple[int, ...]:
        cached = self._otok_cache.get(start)
        if cached is None:
            a, b = (start - self.base) // 4, (self.end_of(start) - self.base) // 4
            cached = tuple(self.otok[a:b].tolist())
            self._otok_cache[start] = cached
        return cached

    def exact_hits(self, func: Func, limit: int = 64) -> List[int]:
        n = len(func.words)
        if n == 0:
            return []
        if func.mask[0] == 0xFFFFFFFF:
            cands = self.by_first.get(int(func.words[0]), [])
        else:
            cands = self.by_first_hi.get(int(func.words[0]) & 0xFFFF0000, [])
        want = func.words & func.mask
        hits = []
        for s in cands:
            a = (s - self.base) // 4
            if a + n > len(self.words):
                continue
            if np.array_equal(self.words[a:a + n] & func.mask, want):
                hits.append(s)
                if len(hits) > limit:
                    break
        return hits


def similarity(a: Sequence[int], b: Sequence[int]) -> float:
    la, lb = len(a), len(b)
    if not la or not lb or min(la, lb) / max(la, lb) < 0.4:
        return 0.0
    sm = SequenceMatcher(None, a, b, autojunk=False)
    if sm.real_quick_ratio() < 0.5 or sm.quick_ratio() < 0.5:
        return 0.0
    return sm.ratio()


###
# Unit placement
###


@dataclass
class Placement:
    func: Func
    start: int
    end: int
    sim: float
    how: str  # exact | named | aligned


@dataclass
class Unit:
    name: str  # objdiff unit name without "main/"
    source: str
    gc_flags: str
    obj: Path
    variant: str = "gc"
    extra: List[str] = field(default_factory=list)
    funcs: List[Func] = field(default_factory=list)
    syms: List[ObjSym] = field(default_factory=list)
    sections: List[ObjSection] = field(default_factory=list)
    error: Optional[str] = None
    placed: List[Placement] = field(default_factory=list)
    text: Optional[Tuple[int, int]] = None
    gaps: List[Tuple[int, int]] = field(default_factory=list)  # unplaced PS2 functions in range
    score: Dict[str, float] = field(default_factory=dict)


def aligned_size(func: Func) -> int:
    return (func.size + 15) // 16 * 16


def best_chain(unit: Unit, cands: Dict[int, List[Tuple[int, float, str]]]) -> List[Tuple[int, int, str]]:
    """Order-preserving anchor chain with plausible spacing, maximizing weight."""
    prefix = [0]
    for f in unit.funcs:
        prefix.append(prefix[-1] + aligned_size(f))
    items = sorted(
        (i, addr, w, how) for i, lst in cands.items() for addr, w, how in lst
    )
    best = [0.0] * len(items)
    back = [-1] * len(items)
    for k, (i, addr, w, _) in enumerate(items):
        best[k] = w
        for j in range(k):
            ij, aj, _, _ = items[j]
            if ij >= i or aj >= addr:
                continue
            expect = prefix[i] - prefix[ij]
            dist = addr - aj
            if dist < expect * 0.4 - 0x100 or dist > expect * 2.5 + 0x800:
                continue
            if best[j] + w > best[k]:
                best[k] = best[j] + w
                back[k] = j
    if not items:
        return []
    k = max(range(len(items)), key=lambda x: best[x])
    chain = []
    while k >= 0:
        i, addr, _, how = items[k]
        chain.append((i, addr, how))
        k = back[k]
    return chain[::-1]


def align_segment(
    image: Ps2Image, funcs: List[Func], starts: List[int], min_sim: float
) -> List[Tuple[Func, int, float]]:
    """Needleman-Wunsch over compiled functions and PS2 functions, free gaps."""
    n, m = len(funcs), len(starts)
    if not n or not m:
        return []
    def score(f: Func, s: int) -> float:
        b = image.otok_of(s)
        size = min(len(f.otok), len(b)) / max(len(f.otok), len(b), 1)
        return similarity(f.otok, b) * (0.5 + 0.5 * size)

    sims = [[score(f, s) for s in starts] for f in funcs]
    score = np.zeros((n + 1, m + 1))
    move = np.zeros((n + 1, m + 1), dtype=np.int8)  # 0 diag, 1 skip func, 2 skip start
    for i in range(1, n + 1):
        move[i][0] = 1
    for j in range(1, m + 1):
        move[0][j] = 2
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            s = sims[i - 1][j - 1]
            diag = score[i - 1][j - 1] + s if s >= min_sim else -1.0
            up = score[i - 1][j]
            left = score[i][j - 1]
            if diag >= up and diag >= left:
                score[i][j], move[i][j] = diag, 0
            elif up >= left:
                score[i][j], move[i][j] = up, 1
            else:
                score[i][j], move[i][j] = left, 2
    out = []
    i, j = n, m
    while i > 0 and j > 0:
        mv = move[i][j]
        if mv == 0:
            out.append((funcs[i - 1], starts[j - 1], sims[i - 1][j - 1]))
            i, j = i - 1, j - 1
        elif mv == 1:
            i -= 1
        else:
            j -= 1
    return out[::-1]


def starts_between(image: Ps2Image, lo: int, hi: int) -> List[int]:
    a = int(np.searchsorted(image.starts, lo, side="left"))
    b = int(np.searchsorted(image.starts, hi, side="left"))
    return image.starts[a:b].tolist()


def fuzzy_anchor(image: Ps2Image, func: Func, min_sim: float = 0.75) -> Optional[Tuple[int, float]]:
    """Best PS2 function by similarity, for units without exact anchors."""
    sizes = image.ends - image.starts
    lo, hi = func.size * 0.7, func.size * 1.4
    best = None
    for s in image.starts[(sizes >= lo) & (sizes <= hi)].tolist():
        sim = similarity(func.otok, image.otok_of(s))
        if sim >= min_sim and (best is None or sim > best[1]):
            best = (s, sim)
    return best


def place_unit(unit: Unit, image: Ps2Image, known: Dict[str, int], min_sim: float) -> None:
    funcs = unit.funcs
    cands: Dict[int, List[Tuple[int, float, str]]] = defaultdict(list)
    for f in funcs:
        hits = image.exact_hits(f)
        if 0 < len(hits) <= 16:
            weight = f.size / len(hits)
            for h in hits:
                cands[f.order].append((h, weight, "exact"))
        addr = known.get(key_for(unit, f.name, f.local))
        if addr is not None and addr in image.start_index:
            if not any(a == addr for a, _, _ in cands[f.order]):
                cands[f.order].append((addr, f.size * 0.75, "named"))
    if not any(cands.values()):
        for f in sorted(funcs, key=lambda f: -f.size)[:4]:
            if f.size < 0x40:
                break
            found = fuzzy_anchor(image, f)
            if found:
                cands[f.order].append((found[0], f.size * found[1] * 0.5, "aligned"))
    chain = best_chain(unit, cands)
    if not chain:
        unit.placed = []
        return

    placed: Dict[int, Placement] = {}
    for i, addr, how in chain:
        f = funcs[i]
        sim = 1.0 if how == "exact" else similarity(f.otok, image.otok_of(addr))
        placed[i] = Placement(f, addr, image.end_of(addr), sim, how)

    # Fill the segments before, between and after the anchors. A segment with
    # anchors on both sides is constrained by position, so it accepts a lower
    # similarity than an open edge segment.
    anchors = [i for i, _, _ in chain]
    segments = []  # (functions, lo, hi, edge) with edge "before" | "after" | None
    first, last = anchors[0], anchors[-1]
    before = sum(aligned_size(f) for f in funcs[:first])
    if first > 0:
        lo = placed[first].start - int(before * 2.0) - 0x200
        segments.append((funcs[:first], lo, placed[first].start, "before"))
    for a, b in zip(anchors, anchors[1:]):
        if b > a + 1:
            segments.append((funcs[a + 1:b], placed[a].end, placed[b].start, None))
    after = sum(aligned_size(f) for f in funcs[last + 1:])
    if last + 1 < len(funcs):
        hi = placed[last].end + int(after * 2.0) + 0x200
        segments.append((funcs[last + 1:], placed[last].end, hi, "after"))
    for seg_funcs, lo, hi, edge in segments:
        starts = [s for s in starts_between(image, lo, hi) if s >= lo]
        limit = 4 * len(seg_funcs) + 16
        if edge == "before":
            starts = starts[-limit:]
        elif edge == "after":
            starts = starts[:limit]
        threshold = min_sim if edge else min_sim * 0.6
        for f, s, sim in align_segment(image, seg_funcs, starts, threshold):
            placed[f.order] = Placement(f, s, image.end_of(s), sim, "aligned")

    unit.placed = [placed[i] for i in sorted(placed)]
    # Drop placements out of address order (keeps the longest consistent run).
    ordered: List[Placement] = []
    for p in unit.placed:
        if ordered and p.start <= ordered[-1].start:
            if p.sim > ordered[-1].sim:
                ordered[-1] = p
            continue
        ordered.append(p)
    unit.placed = ordered
    lo = min(p.start for p in ordered)
    hi = max(p.end for p in ordered)
    unit.text = (lo, hi)
    mapped = {p.start for p in ordered}
    unit.gaps = [
        (s, image.end_of(s)) for s in starts_between(image, lo, hi) if s not in mapped
    ]


###
# Symbol harvest
###


def key_for(unit: Unit, name: str, local: bool) -> str:
    return f"{unit.name}:{name}" if local else name


@dataclass
class Vote:
    addrs: Counter = field(default_factory=Counter)
    kind: str = "object"
    size: Optional[int] = None
    local: bool = False
    unit: str = ""
    name: str = ""


def block_map(func: Func, rtok: Sequence[int], exact: bool) -> Dict[int, int]:
    if exact:
        return {i: i for i in range(len(func.words))}
    out = {}
    sm = SequenceMatcher(None, func.tok, rtok, autojunk=False)
    for a, b, size in sm.get_matching_blocks():
        if size < 3:  # short runs pair unrelated instructions
            continue
        for k in range(size):
            out[a + k] = b + k
    return out


def harvest(unit: Unit, image: Ps2Image, votes: Dict[str, Vote], defined: Dict[str, ObjSym]) -> None:
    for p in unit.placed:
        f = p.func
        exact = p.how == "exact"
        n_retail = (p.end - p.start) // 4
        rwords = image.words_at(p.start, max(n_retail, len(f.words)))
        mapping = block_map(f, image.tok_of(p.start), exact)
        for k, rel in enumerate(f.relocs):
            sym = rel.sym
            if sym.kind == 3 or not sym.name:  # section symbols
                continue
            j = mapping.get(rel.index)
            if j is None or j >= len(rwords):
                continue
            cw, rw = int(f.words[rel.index]), int(rwords[j])
            if cw >> 26 != rw >> 26:
                continue
            addr = None
            kind = "object"
            if rel.rtype == R_MIPS_26:
                pc = p.start + 4 * j
                target = ((pc + 4) & 0xF0000000) | ((rw & 0x3FFFFFF) << 2)
                addr = target - ((cw & 0x3FFFFFF) << 2)
                kind = "function"
            elif rel.rtype == R_MIPS_HI16:
                lo = next(
                    (r for r in f.relocs[k + 1:]
                     if r.rtype == R_MIPS_LO16 and r.sym is sym), None
                )
                if lo is None:
                    continue
                j2 = mapping.get(lo.index)
                if j2 is None or j2 >= len(rwords):
                    continue
                cl, rl = int(f.words[lo.index]), int(rwords[j2])
                if cl >> 26 != rl >> 26:
                    continue
                retail = ((rw & 0xFFFF) << 16) + sext16(rl)
                compiled = ((cw & 0xFFFF) << 16) + sext16(cl)
                addr = (retail - compiled) & 0xFFFFFFFF
            elif rel.rtype in (R_MIPS_GPREL16, R_MIPS_LITERAL):
                addr = (image.gp + sext16(rw) - sext16(cw)) & 0xFFFFFFFF
            if addr is None:
                continue
            local = sym.bind == 0
            key = key_for(unit, sym.name, local)
            vote = votes.setdefault(key, Vote(name=sym.name, local=local, unit=unit.name))
            vote.addrs[addr] += 3 if exact else 1
            if sym.kind == 2 or kind == "function":
                vote.kind = "function"
            d = defined.get(key)
            if d is not None and d.size:
                vote.size = d.size


def resolve_votes(votes: Dict[str, Vote]) -> Dict[str, int]:
    """Majority address per symbol; an address keeps only its best-supported name."""
    best: Dict[int, Tuple[int, str]] = {}
    for key, vote in votes.items():
        (addr, top), = vote.addrs.most_common(1)
        if top / sum(vote.addrs.values()) < 0.6:
            continue
        if addr not in best or top > best[addr][0]:
            best[addr] = (top, key)
    return {key: addr for addr, (_, key) in best.items()}


def data_check(unit: Unit, image: Ps2Image, resolved: Dict[str, int]) -> Tuple[int, int]:
    """Compare initialized data the unit defines with PS2 bytes at harvested addresses.

    Independent of the code match: an address harvested from code relocations
    is right if the bytes there equal the compiled initializer (pointer words
    masked). Returns (matching symbols, checked symbols).
    """
    ok = checked = 0
    for sym in unit.syms:
        if sym.kind != 1 or sym.size < 4 or sym.shndx in (0, 0xFFF1, 0xFFF2):
            continue
        sec = unit.sections[sym.shndx]
        if sec.sh_type != 1 or not sec.data:  # PROGBITS only
            continue
        addr = resolved.get(key_for(unit, sym.name, sym.bind == 0))
        if addr is None:
            continue
        off = image.elf.file_offset(addr)
        if off is None:
            continue
        mine = bytearray(sec.data[sym.value:sym.value + sym.size])
        theirs = bytearray(image.elf.data[off:off + sym.size])
        if len(theirs) != len(mine):
            continue
        for r_offset, rtype, _ in sec.relocs:
            if rtype == R_MIPS_32 and sym.value <= r_offset < sym.value + sym.size:
                k = r_offset - sym.value
                mine[k:k + 4] = theirs[k:k + 4] = b"\0\0\0\0"
        checked += 1
        ok += mine == theirs
    return ok, checked


###
# Build, split, score
###


@dataclass
class Tools:
    mwcc: Path
    wibo: Path
    gnu_as: Path
    objdiff: Path

    @classmethod
    def find(cls, compiler: str = COMPILER) -> "Tools":
        t = cls(
            ROOT / "build/ps2_compilers/PS2" / compiler / "mwccps2.exe",
            ROOT / "build/tools/wibo",
            ROOT / "build/ps2_binutils/mips-ps2-decompals-as",
            ROOT / "build/tools/objdiff-cli",
        )
        missing = [p for p in (t.mwcc, t.wibo, t.gnu_as, t.objdiff) if not p.exists()]
        if missing:
            sys.exit(
                "missing PS2 tools: " + ", ".join(str(p) for p in missing)
                + "\n    run: python3 configure.py --version SLUS_208.81 && ninja"
                + " build/SLUS_208.81/report.json"
            )
        return t


def compile_unit(unit: Unit, tools: Tools) -> None:
    unit.obj.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(tools.wibo), str(tools.mwcc), *ps2_cflags(unit.gc_flags), *unit.extra,
        "-c", unit.source, "-o", str(unit.obj),
    ]
    res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if res.returncode != 0 or not unit.obj.exists():
        text = (res.stdout + res.stderr).strip().splitlines()
        err = next((l for l in text if "rror" in l), text[0] if text else "compile failed")
        # Include the offending line that follows MW's "Error" header.
        for k, line in enumerate(text):
            if line.strip().startswith("#") and "rror" in line and k + 1 < len(text):
                err = text[k + 1].strip() + " | " + " ".join(text[k + 2:k + 4]).strip()
                break
        unit.error = err[:300]
        return
    sections, syms = read_obj(unit.obj)
    unit.syms = syms
    unit.sections = sections
    unit.funcs = unit_functions(sections, syms)


def referenced_keys(units: List[Unit]) -> set:
    """Symbol keys the units' code references or the units define."""
    keys = set()
    for u in units:
        for f in u.funcs:
            for r in f.relocs:
                keys.add(key_for(u, r.sym.name, r.sym.bind == 0))
        for s in u.syms:
            if s.kind in (1, 2) and s.shndx not in (0, 0xFFF1, 0xFFF2) and s.name:
                keys.add(key_for(u, s.name, s.bind == 0))
    return keys


def write_split_inputs(
    units: List[Unit], image: Ps2Image, resolved: Dict[str, int],
    votes: Dict[str, Vote], splits_path: Path, symbols_path: Path,
    only: Optional[set] = None,
) -> None:
    """Write splits.txt/symbols.txt for the units (symbols limited to `only` keys)."""
    lines = ["Sections:", "\t.text       type:code align:16", ""]
    syms: Dict[int, str] = {}
    taken: set = set()

    # Locals are keyed by unit: units may each have a static of the same name
    # (MW's @stringBase0, @123 literals); each unit's asm only names its own.
    def add(key: str, addr: int, line: str) -> bool:
        if addr in syms or key in taken:
            return False
        syms[addr] = line
        taken.add(key)
        return True

    for unit in sorted(units, key=lambda u: u.text[0]):
        lo, hi = unit.text
        lines += [f"{unit.name}.o:", f"\t.text       start:0x{lo:08X} end:0x{hi:08X}", ""]
        for p in unit.placed:
            scope = "local" if p.func.local else "global"
            add(key_for(unit, p.func.name, p.func.local), p.start,
                f"{p.func.name} = .text:0x{p.start:08X}; // type:function "
                f"size:0x{p.end - p.start:X} scope:{scope} align:8")
        for s, e in unit.gaps:
            add(f"func_{s:08X}", s,
                f"func_{s:08X} = .text:0x{s:08X}; // type:function size:0x{e - s:X} scope:global align:8")
    names_in_units = {u.name for u in units}
    layout, _ = ps2_split.read_splits(ROOT / "config" / VERSION / "splits.txt")
    for key, addr in resolved.items():
        vote = votes[key]
        if vote.local and vote.unit not in names_in_units:
            continue
        if only is not None and key not in only:
            continue
        if not image.base <= addr < image.base + 0x1000000:
            continue
        section = ps2_split.section_at(layout, addr) or (
            ".text" if addr < image.text_end else ".data"
        )
        size = f" size:0x{vote.size:X}" if vote.size else ""
        scope = "local" if vote.local else "global"
        add(key, addr,
            f"{vote.name} = {section}:0x{addr:08X}; // type:{vote.kind}{size} scope:{scope}")
    splits_path.write_text("\n".join(lines) + "\n")
    symbols_path.write_text("\n".join(syms[a] for a in sorted(syms)) + "\n")


def split_and_score(units: List[Unit], image: Ps2Image, resolved: Dict[str, int],
                    votes: Dict[str, Vote], tools: Tools, work: Path) -> None:
    splits = work / "splits.txt"
    symbols = work / "symbols.txt"
    write_split_inputs(units, image, resolved, votes, splits, symbols)
    asm_dir = work / "asm"
    res = subprocess.run(
        [sys.executable, str(ROOT / "tools/ps2_split.py"),
         "--elf", str(ROOT / "orig" / VERSION / VERSION),
         "--splits", str(splits), "--symbols", str(symbols), "--out", str(asm_dir)],
        cwd=ROOT, capture_output=True, text=True,
    )
    if res.returncode != 0:
        sys.exit("ps2_split.py failed:\n" + res.stdout + res.stderr)
    objdiff_units = []
    for unit in units:
        asm_path = asm_dir / f"{unit.name}.s"
        target = work / "target" / f"{unit.name}.o"
        target.parent.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(
            [str(tools.gnu_as), "-EL", "-march=r5900", "-mabi=eabi", "-G0",
             "-no-pad-sections", "-I", str(asm_dir), "-o", str(target), str(asm_path)],
            capture_output=True, text=True,
        )
        if r.returncode != 0:
            unit.error = "assemble: " + (r.stderr.strip().splitlines() or ["?"])[0][:200]
            continue
        objdiff_units.append({
            "name": f"main/{unit.name}",
            "target_path": str(target.relative_to(work)),
            "base_path": str(unit.obj.resolve()),
            "metadata": {"source_path": unit.source, "auto_generated": False},
        })
    (work / "objdiff.json").write_text(json.dumps({
        "min_version": "2.0.0-beta.5", "build_target": False, "units": objdiff_units,
    }, indent=1))
    for label, extra in (("struct", ["-c", "functionRelocDiffs=none"]),
                         ("named", ["-c", "functionRelocDiffs=name_address"])):
        out = work / f"report_{label}.json"
        r = subprocess.run(
            [str(tools.objdiff), "report", "generate", "-p", str(work), "-o", str(out), *extra],
            capture_output=True, text=True,
        )
        if r.returncode != 0:
            sys.exit("objdiff report failed:\n" + r.stdout + r.stderr)
        report = json.loads(out.read_text())
        by_name = {u["name"]: u for u in report.get("units", [])}
        for unit in units:
            m = by_name.get(f"main/{unit.name}", {}).get("measures", {})
            unit.score[f"{label}_fuzzy"] = float(m.get("fuzzy_match_percent", 0) or 0)
            unit.score[f"{label}_matched_code"] = float(m.get("matched_code_percent", 0) or 0)
            unit.score[f"{label}_matched_functions"] = int(m.get("matched_functions", 0) or 0)
            unit.score["total_code"] = int(m.get("total_code", 0) or 0)
            unit.score["total_functions"] = int(m.get("total_functions", 0) or 0)


###
# Driver
###


def load_units(filters: List[str], work: Path) -> List[Unit]:
    objdiff = json.loads((ROOT / "objdiff.json").read_text())
    units = []
    for u in objdiff["units"]:
        meta = u.get("metadata", {})
        src = meta.get("source_path", "")
        scratch = u.get("scratch", {})
        if not src or "c_flags" not in scratch:
            continue
        if not any(src.startswith(f) for f in filters):
            continue
        name = u["name"].split("/", 1)[1]
        units.append(Unit(name, src, scratch["c_flags"], work / "base" / f"{name}.o"))
    if not units:
        sys.exit("no units matched; is objdiff.json the GQNE5D one? (python3 configure.py)")
    return units


# Flags the PS2 project's base cflags already carry (configure.py ps2_config).
PS2_BASE_FLAGS = {"-nodefaults", "-nosyspath", "-DNDEBUG=1", "-DVERSION_SLUS_208_81"}


def paired_flags(flags: List[str]) -> List[str]:
    """Join option/value pairs the way configure.py lists them."""
    out = []
    i = 0
    while i < len(flags):
        tok = flags[i]
        if (tok in VALUE_FLAGS or tok == "-i") and i + 1 < len(flags):
            value = flags[i + 1]
            out.append(f'{tok} "{value}"' if " " in value else f"{tok} {value}")
            i += 2
        else:
            out.append(tok)
            i += 1
    return out


def object_flags(unit: Unit) -> List[str]:
    flags = paired_flags(ps2_cflags(unit.gc_flags)[2 * len(INCLUDE_DIRS):] + unit.extra)
    return [f for f in flags if f not in PS2_BASE_FLAGS]


def config_snippet(top: List[Unit], all_units: List[Unit]) -> str:
    """configure.py lines for the PS2 objects: shared game prefix plus extras."""
    lists = [object_flags(u) for u in all_units if u.error is None]
    prefix: List[str] = []
    for column in zip(*lists):
        if any(c != column[0] for c in column):
            break
        prefix.append(column[0])
    out = ["cflags_ps2_game = ["]
    out += [f"    {json.dumps(f)}," for f in
            [f"-i {d}" for d in INCLUDE_DIRS[2:]] + prefix]
    out.append("]")
    base_opt = [f for f in prefix if f.startswith("-O")]
    for u in sorted(top, key=lambda u: u.text[0]):
        extras = object_flags(u)[len(prefix):]
        # The last -O level wins; list only the effective one.
        opts = [f for f in extras if f.startswith("-O")]
        extras = [f for f in extras if not f.startswith("-O")]
        if opts and (not base_opt or opts[-1] != base_opt[-1]):
            extras.insert(0, opts[-1])
        src = u.source[len("src/"):]
        out.append(f'    ("{u.name}.o", "{src}", [{", ".join(json.dumps(f) for f in extras)}]),')
    return "\n".join(out[:len(prefix) + 4] + ["ps2_game_units = ["] + out[len(prefix) + 4:] + ["]"])


def write_config(top: List[Unit], image: Ps2Image, resolved: Dict[str, int],
                 votes: Dict[str, Vote]) -> None:
    cfg = ROOT / "config" / VERSION
    splits = cfg / "splits.txt"
    symbols = cfg / "symbols.txt"
    old_splits = splits.read_text()
    old_syms = symbols.read_text().splitlines()
    tmp_splits = Path(str(splits) + ".new")
    tmp_syms = Path(str(symbols) + ".new")
    write_split_inputs(top, image, resolved, votes, tmp_splits, tmp_syms,
                       only=referenced_keys(top))
    # Append unit blocks after the existing ones; skip units already listed.
    new_blocks = tmp_splits.read_text().split("\n\n", 1)[1]
    existing_units = set(re.findall(r"^(\S+\.o):", old_splits, re.M))
    keep = []
    for block in new_blocks.strip().split("\n\n"):
        head = block.splitlines()[0].rstrip(":")
        if head not in existing_units:
            keep.append(block)
    splits.write_text(old_splits.rstrip("\n") + "\n\n" + "\n\n".join(keep) + "\n")
    known_names = {l.split(" = ")[0] for l in old_syms if " = " in l}
    known_addrs = {re.search(r":0x([0-9A-F]+);", l).group(1) for l in old_syms if " = " in l}
    lines = list(old_syms)
    for line in tmp_syms.read_text().splitlines():
        name = line.split(" = ")[0]
        addr = re.search(r":0x([0-9A-F]+);", line).group(1)
        if name in known_names or addr in known_addrs:
            continue
        lines.append(line)
    lines.sort(key=lambda l: (l.split(" = ")[1].split(":")[0] != ".text",
                              int(re.search(r":0x([0-9A-F]+);", l).group(1), 16)))
    symbols.write_text("\n".join(lines) + "\n")
    tmp_splits.unlink()
    tmp_syms.unlink()


@dataclass
class PipelineResult:
    units: List[Unit]
    known: Dict[str, int]
    votes: Dict[str, "Vote"]


def run_pipeline(units: List[Unit], image: Ps2Image, tools: Tools, work: Path,
                 min_sim: float, tag: str) -> PipelineResult:
    """Place, harvest (two rounds) and score units that compiled."""
    work.mkdir(parents=True, exist_ok=True)
    ok = [u for u in units if u.error is None and u.funcs]
    for u in ok:
        u.placed, u.text, u.gaps, u.score = [], None, [], {}
    defined: Dict[str, ObjSym] = {}
    for u in ok:
        for s in u.syms:
            if s.shndx not in (0, 0xFFF1, 0xFFF2) and s.kind in (1, 2) and s.name:
                defined[key_for(u, s.name, s.bind == 0)] = s

    known: Dict[str, int] = {}
    votes: Dict[str, Vote] = {}
    for rnd in range(2):
        votes = {}
        for u in ok:
            place_unit(u, image, known, min_sim)
        # Own placements are votes too: they name the placed functions.
        for u in ok:
            for p in u.placed:
                key = key_for(u, p.func.name, p.func.local)
                v = votes.setdefault(key, Vote(name=p.func.name, local=p.func.local,
                                               unit=u.name, kind="function"))
                v.addrs[p.start] += 6 if p.how == "exact" else 2
            harvest(u, image, votes, defined)
        known = resolve_votes(votes)
    print(f"{tag}compiled {len(ok)}/{len(units)}, placed "
          f"{sum(len(u.placed) for u in ok)} functions, {len(known)} PS2 symbols")

    located = [u for u in ok if u.placed and u.text]
    # Overlapping placements: keep the unit with more exact functions.
    located.sort(key=lambda u: u.text[0])
    final: List[Unit] = []
    for u in located:
        if final and u.text[0] < final[-1].text[1]:
            prev = final[-1]
            mine = sum(p.how == "exact" for p in u.placed)
            theirs = sum(p.how == "exact" for p in prev.placed)
            if mine > theirs:
                prev.error = f"overlaps {u.name}"
                final[-1] = u
            else:
                u.error = f"overlaps {prev.name}"
            continue
        final.append(u)
    if final:
        split_and_score(final, image, known, votes, tools, work)
    for u in final:
        good, checked = data_check(u, image, known)
        u.score["data_ok"], u.score["data_checked"] = good, checked
    return PipelineResult(final, known, votes)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--filter", action="append",
                    help="source path prefix to include (default: src/game/)")
    ap.add_argument("--work", type=Path, default=ROOT / "build" / VERSION / "cross",
                    help="work directory (default: build/SLUS_208.81/cross)")
    ap.add_argument("--top", type=int, default=10, help="units to list as the top")
    ap.add_argument("--min-sim", type=float, default=0.55,
                    help="minimum instruction similarity to place a non-exact function")
    ap.add_argument("--min-code", type=int, default=0x400,
                    help="minimum located PS2 code bytes to rank a unit")
    ap.add_argument("--write-config", type=int, default=0, metavar="N",
                    help="add the top N units to config/SLUS_208.81 splits/symbols")
    ap.add_argument("--variants", default=DEFAULT_VARIANTS,
                    help=f"comma-separated flag variants to try (default: {DEFAULT_VARIANTS}; "
                         f"known: {', '.join(VARIANTS)})")
    ap.add_argument("--compiler", default=COMPILER,
                    help=f"decomp.me mwcps2 compiler id under build/ps2_compilers/PS2 (default: {COMPILER})")
    ap.add_argument("-j", "--jobs", type=int, default=8)
    args = ap.parse_args()

    filters = args.filter or ["src/game/"]
    work: Path = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    tools = Tools.find(args.compiler)
    image = Ps2Image(ROOT / "orig" / VERSION / VERSION)
    print(f"PS2 text 0x{image.base:08X}-0x{image.text_end:08X}, "
          f"{len(image.starts)} function starts, gp 0x{image.gp:08X}")

    units = load_units(filters, work)
    variants = [v for v in args.variants.split(",") if v]
    for v in variants:
        if v not in VARIANTS:
            sys.exit(f"unknown variant {v}; known: {', '.join(VARIANTS)}")
    best: Dict[str, Unit] = {}
    for v in variants:
        vunits = [
            Unit(u.name, u.source, u.gc_flags, work / "base" / v / f"{u.name}.o",
                 variant=v, extra=VARIANTS[v])
            for u in units
        ]
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            list(pool.map(lambda u: compile_unit(u, tools), vunits))
        run_pipeline(vunits, image, tools, work / v, args.min_sim, f"[{v}] ")
        for u in vunits:
            cur = best.get(u.name)
            if cur is None or (u.error is None and (
                    cur.error is not None
                    or u.score.get("struct_fuzzy", 0) > cur.score.get("struct_fuzzy", 0))):
                best[u.name] = u
    units = [best[u.name] for u in units]
    for u in units:
        if u.error is None or u.error.startswith("overlaps"):
            u.error = None
    print("final: " + ", ".join(f"{v} x{sum(u.variant == v for u in units)}" for v in variants))
    final = run_pipeline(units, image, tools, work / "final", args.min_sim, "[final] ")
    known, votes = final.known, final.votes
    final = final.units

    ranked = sorted(
        (u for u in final if u.error is None and u.score.get("total_code", 0) >= args.min_code),
        key=lambda u: (-u.score.get("struct_fuzzy", 0), -u.score.get("total_code", 0)),
    )
    rows = []
    for u in units:
        rows.append({
            "unit": u.name, "source": u.source, "error": u.error,
            "functions": len(u.funcs), "placed": len(u.placed),
            "exact": sum(p.how == "exact" for p in u.placed),
            "text": [f"0x{u.text[0]:08X}", f"0x{u.text[1]:08X}"] if u.text else None,
            "unplaced_ps2_functions": len(u.gaps),
            "variant": u.variant,
            "flags": " ".join(ps2_cflags(u.gc_flags)[2 * len(INCLUDE_DIRS):] + u.extra),
            **u.score,
        })
    (work / "cross.json").write_text(json.dumps(rows, indent=1))

    hdr = (f"{'unit':<22} {'struct%':>8} {'named%':>7} {'exact fn':>9} "
           f"{'placed':>8} {'PS2 code':>9} {'data ok':>8}  {'variant':<8} range")
    print("\n" + hdr)
    print("-" * len(hdr))
    for u in ranked[: args.top]:
        s = u.score
        print(f"{u.name:<22} {s['struct_fuzzy']:8.2f} {s['named_fuzzy']:7.2f} "
              f"{s['struct_matched_functions']:>4}/{s['total_functions']:<4} "
              f"{len(u.placed):>3}/{len(u.funcs):<4} {s['total_code']:>9} "
              f"{s['data_ok']:>3}/{s['data_checked']:<4}  "
              f"{u.variant:<8} 0x{u.text[0]:08X}-0x{u.text[1]:08X}")
    failed = [u for u in units if u.error]
    if failed:
        print(f"\n{len(failed)} units not scored:")
        for u in failed:
            print(f"  {u.name}: {u.error}")
    unplaced = [u for u in units if u.error is None and u.funcs and not u.placed]
    if unplaced:
        print(f"\n{len(unplaced)} units without a PS2 location: "
              + ", ".join(u.name for u in unplaced))
    print(f"\nwrote {work / 'cross.json'}")

    if args.write_config:
        top = ranked[: args.write_config]
        write_config(top, image, known, votes)
        print(f"added {len(top)} units to config/{VERSION}/splits.txt and symbols.txt")
        snippet = config_snippet(top, units)
        (work / "configure_snippet.py").write_text(snippet + "\n")
        print(f"configure.py objects for them: {work / 'configure_snippet.py'}")


if __name__ == "__main__":
    main()
