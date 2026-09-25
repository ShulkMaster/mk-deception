"""Call-argument staging transforms for decomp-permuter.

MWCC register choice around a call is often decided by where each argument
value is staged: in a local assigned by an earlier statement, folded into the
argument expression, or folded as an assignment expression that also keeps
the local (``f(x *= k)``).  Upstream ``perm_temp_for_expr`` and
``perm_expand_expr`` explore this one expression at a time and only fold plain
``=`` writes, so combinations across several arguments are rarely visited and
compound-assignment folds are never produced as honest source.

This module enumerates, per call argument:

* ``keep``        leave the argument unchanged;
* ``fold``        ``v op= e; ... f(v)``  ->  ``f(v op= e)`` (``v`` stays live);
* ``fold-value``  ``v = e; f(v)`` -> ``f(e)``, ``v op= e; f(v)`` -> ``f(v op e)``
                  when ``v`` has no later use;
* ``fold-deep``   ``v = e0; v op= e1; f(v)`` -> ``f((e0) op e1)``;
* ``hoist``       ``f(expr)`` -> ``tmp = expr; f(tmp)``.

Every option preserves evaluation order under the playbook rules: a moved
expression may not cross a call or memory store it could observe, no
address-taken local is folded, and a combination moves at most one
expression containing a call.  The module needs decomp-permuter's ``src``
package on ``sys.path``; see ``tools/permuter_mkd.py``.
"""

from __future__ import annotations

import copy
import itertools
from dataclasses import dataclass, field
from random import Random
from typing import Iterator, List, Optional, Sequence, Set, Tuple

from perm_pycparser import c_ast as ca

from src import ast_util
from src.ast_types import build_typemap, decayed_expr_type, is_local_var
from src.ast_util import Block, Expression, Statement

COMPOUND_OPS = ("+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^=")
KEEP = "keep"


@dataclass
class ArgOption:
    kind: str
    # Statements removed from the argument's block, by identity.
    removed: List[Statement] = field(default_factory=list)
    # Decl whose initializer is dropped (fold of a declaration initializer).
    drop_init: Optional[ca.Decl] = None
    # Decl removed entirely because the local has no remaining use.
    drop_decl: Optional[ca.Decl] = None
    # Replacement argument expression (built lazily on apply).
    new_arg: Optional[Expression] = None
    # For hoist: the hoisted expression and its type.
    hoist_type: Optional[object] = None
    has_call: bool = False
    touches_memory: bool = False

    def describe(self, index: int, arg: Expression) -> str:
        if self.kind == KEEP:
            return ""
        before = ast_util.to_c_raw(arg).strip()
        if self.kind == "hoist":
            return f"arg{index}: hoist {before}"
        after = ast_util.to_c_raw(self.new_arg).strip() if self.new_arg else "?"
        return f"arg{index}: {self.kind} {before} -> {after}"


@dataclass
class CallSite:
    block: Block
    stmt: Statement
    call: ca.FuncCall
    options: List[List[ArgOption]]  # per argument; index 0 is always keep

    @property
    def name(self) -> str:
        callee = self.call.name
        return ast_util.to_c_raw(callee).strip() if callee is not None else "?"

    def combos(self) -> Iterator[Tuple[int, ...]]:
        for combo in itertools.product(*(range(len(o)) for o in self.options)):
            if any(combo) and combo_is_valid(self, combo):
                yield combo


def _walk(node: ca.Node, cls: type) -> List[ca.Node]:
    found: List[ca.Node] = []

    class V(ca.NodeVisitor):
        def generic_visit(self, n: ca.Node) -> None:
            if isinstance(n, cls):
                found.append(n)
            for _, child in n.children():
                self.visit(child)

    V().visit(node)
    return found


def _names(node: Optional[ca.Node]) -> Set[str]:
    if node is None:
        return set()
    return {n.name for n in _walk(node, ca.ID)}


def _written_names(node: ca.Node) -> Set[str]:
    out: Set[str] = set()
    for a in _walk(node, ca.Assignment):
        if isinstance(a.lvalue, ca.ID):
            out.add(a.lvalue.name)
    for u in _walk(node, ca.UnaryOp):
        if u.op in ("p++", "p--", "++", "--") and isinstance(u.expr, ca.ID):
            out.add(u.expr.name)
    for d in _walk(node, ca.Decl):
        if d.name:
            out.add(d.name)
    return out


def _has_call(node: Optional[ca.Node]) -> bool:
    return node is not None and bool(_walk(node, ca.FuncCall))


def _stores_memory(node: ca.Node) -> bool:
    for a in _walk(node, ca.Assignment):
        if not isinstance(a.lvalue, ca.ID):
            return True
    for u in _walk(node, ca.UnaryOp):
        if u.op in ("p++", "p--", "++", "--") and not isinstance(u.expr, ca.ID):
            return True
    return False


def _touches_memory(expr: ca.Node, typemap: object) -> bool:
    if _walk(expr, ca.StructRef) or _walk(expr, ca.ArrayRef) or _has_call(expr):
        return True
    for u in _walk(expr, ca.UnaryOp):
        if u.op == "*":
            return True
    for ident in _walk(expr, ca.ID):
        if not is_local_var(ident.name, typemap):  # global or function
            return True
    return False


def _address_taken(fn: ca.FuncDef) -> Set[str]:
    return {
        u.expr.name
        for u in _walk(fn.body, ca.UnaryOp)
        if u.op == "&" and isinstance(u.expr, ca.ID)
    }


def _shallow_exprs(stmt: Statement) -> List[Expression]:
    """Expressions evaluated exactly once, before any nested block runs."""
    if isinstance(stmt, (ca.If, ca.Switch)):
        return [stmt.cond] if stmt.cond is not None else []
    if isinstance(stmt, ca.Return):
        return [stmt.expr] if stmt.expr is not None else []
    if isinstance(stmt, ca.Decl):
        init = stmt.init
        return [init] if init is not None and not isinstance(init, ca.InitList) else []
    if isinstance(
        stmt,
        (ca.Compound, ca.For, ca.While, ca.DoWhile, ca.Label, ca.Case, ca.Default,
         ca.Goto, ca.Break, ca.Continue, ca.EmptyStatement, ca.Pragma, ca.DeclList),
    ):
        return []
    return [stmt]


def _decl_of(fn: ca.FuncDef, name: str) -> Optional[ca.Decl]:
    for d in _walk(fn.body, ca.Decl):
        if d.name == name:
            return d
    return None


def _count_refs(node: ca.Node, name: str) -> int:
    return sum(1 for n in _walk(node, ca.ID) if n.name == name)


class _Analyzer:
    def __init__(self, fn: ca.FuncDef, ast: ca.FileAST) -> None:
        self.fn = fn
        self.typemap = build_typemap(ast, fn)
        self.addr_taken = _address_taken(fn)
        self.sites: List[CallSite] = []
        self.hoist_counter = 0

    def run(self) -> List[CallSite]:
        self._block(self.fn.body, in_loop=False)
        return self.sites

    def _block(self, block: Block, in_loop: bool) -> None:
        stmts = ast_util.get_block_stmts(block, False)
        for index, stmt in enumerate(stmts):
            for expr in _shallow_exprs(stmt):
                for call in _walk(expr, ca.FuncCall):
                    site = self._site(block, stmts, index, stmt, call, in_loop)
                    if site is not None:
                        self.sites.append(site)
            loop = in_loop or isinstance(stmt, (ca.For, ca.While, ca.DoWhile))
            ast_util.for_nested_blocks(stmt, lambda b: self._block(b, loop))

    def _site(self, block: Block, stmts: List[Statement], index: int,
              stmt: Statement, call: ca.FuncCall, in_loop: bool) -> Optional[CallSite]:
        if call.args is None or not call.args.exprs:
            return None
        options: List[List[ArgOption]] = []
        for k, arg in enumerate(call.args.exprs):
            opts = [ArgOption(KEEP)]
            opts += self._fold_options(stmts, index, stmt, call, k, arg, in_loop)
            opts += self._hoist_options(stmts, index, stmt, call, k, arg)
            options.append(opts)
        if all(len(o) == 1 for o in options):
            return None
        return CallSite(block, stmt, call, options)

    def _rest_of_stmt_has_call(self, stmt: Statement, call: ca.FuncCall, k: int) -> bool:
        """Does anything evaluated in stmt, other than call itself and arg k, call?"""
        total = sum(len(_walk(e, ca.FuncCall)) for e in _shallow_exprs(stmt))
        return total - 1 - len(_walk(call.args.exprs[k], ca.FuncCall)) > 0

    def _other_refs_in_stmt(self, stmt: Statement, call: ca.FuncCall, k: int, name: str) -> int:
        total = sum(_count_refs(e, name) for e in _shallow_exprs(stmt))
        return total - _count_refs(call.args.exprs[k], name)

    def _find_writer(self, stmts: List[Statement], index: int, name: str,
                     skip: Sequence[Statement] = ()) -> Optional[int]:
        """Nearest earlier statement mentioning name, if it is a pure write."""
        for j in range(index - 1, -1, -1):
            s = stmts[j]
            if s in skip:
                continue
            if name not in _names(s) and not (isinstance(s, ca.Decl) and s.name == name):
                continue
            if isinstance(s, ca.Assignment) and isinstance(s.lvalue, ca.ID) \
                    and s.lvalue.name == name and name not in _names(s.rvalue):
                return j
            if isinstance(s, ca.Assignment) and isinstance(s.lvalue, ca.ID) \
                    and s.lvalue.name == name and s.op in COMPOUND_OPS:
                return j
            if isinstance(s, ca.Decl) and s.name == name and s.init is not None \
                    and not isinstance(s.init, ca.InitList):
                return j
            return None
        return None

    def _can_move(self, stmts: List[Statement], lo: int, hi: int, expr: Expression,
                  skip: Sequence[Statement] = ()) -> bool:
        """Can expr, evaluated at stmts[lo], move to stmts[hi] unchanged?"""
        reads = _names(expr)
        memory = _touches_memory(expr, self.typemap)
        for s in stmts[lo + 1:hi]:
            if s in skip:
                continue
            if _written_names(s) & reads:
                return False
            if memory and (_has_call(s) or _stores_memory(s)):
                return False
            if _has_call(expr) and (_has_call(s) or _stores_memory(s)):
                return False
        return True

    def _fold_options(self, stmts: List[Statement], index: int, stmt: Statement,
                      call: ca.FuncCall, k: int, arg: Expression,
                      in_loop: bool) -> List[ArgOption]:
        if not isinstance(arg, ca.ID):
            return []
        name = arg.name
        if not is_local_var(name, self.typemap) or name in self.addr_taken:
            return []
        if self._other_refs_in_stmt(stmt, call, k, name):
            return []
        j = self._find_writer(stmts, index, name)
        if j is None:
            return []
        writer = stmts[j]
        value = writer.init if isinstance(writer, ca.Decl) else writer.rvalue
        if not self._can_move(stmts, j, index, value):
            return []
        memory = _touches_memory(value, self.typemap)
        if memory and self._rest_of_stmt_has_call(stmt, call, k):
            return []
        base = dict(has_call=_has_call(value), touches_memory=memory)
        out: List[ArgOption] = []
        op = "=" if isinstance(writer, ca.Decl) else writer.op

        # fold: keep the local, move its write into the argument.
        fold = ArgOption("fold", new_arg=ca.Assignment(op, ca.ID(name), copy.deepcopy(value)), **base)
        if isinstance(writer, ca.Decl):
            fold.drop_init = writer
        else:
            fold.removed = [writer]
        out.append(fold)

        # fold-value: the local's final value is only consumed here.
        later_refs = sum(_count_refs(s, name) for s in stmts[index + 1:])
        other_refs = _count_refs(self.fn.body, name) - _count_refs(writer, name) \
            - _count_refs(stmt, name)
        decl = _decl_of(self.fn, name)
        if later_refs or in_loop:
            return out
        if op == "=":
            if isinstance(writer, ca.Decl):
                if other_refs:
                    return out
                out.append(ArgOption("fold-value", drop_decl=writer,
                                     new_arg=copy.deepcopy(value), **base))
            else:
                if decl is None or decl.init is not None or other_refs:
                    return out
                out.append(ArgOption("fold-value", removed=[writer], drop_decl=decl,
                                     new_arg=copy.deepcopy(value), **base))
            return out

        binop = op[:-1]
        out.append(ArgOption("fold-value", removed=[writer],
                             new_arg=ca.BinaryOp(binop, ca.ID(name), copy.deepcopy(value)),
                             **base))
        # fold-deep: also fold the previous plain write of the same local.
        j0 = self._find_writer(stmts, j, name)
        if j0 is None:
            return out
        first = stmts[j0]
        first_value = first.init if isinstance(first, ca.Decl) else first.rvalue
        is_plain = isinstance(first, ca.Decl) or first.op == "="
        if not is_plain or not self._can_move(stmts, j0, index, first_value, skip=[writer]):
            return out
        if _touches_memory(first_value, self.typemap) and self._rest_of_stmt_has_call(stmt, call, k):
            return out
        remaining = other_refs - _count_refs(first, name)
        if isinstance(first, ca.Decl):
            if remaining:
                return out
            deep = ArgOption("fold-deep", removed=[writer], drop_decl=first)
        else:
            if decl is None or decl.init is not None or remaining:
                return out
            deep = ArgOption("fold-deep", removed=[first, writer], drop_decl=decl)
        deep.new_arg = ca.BinaryOp(binop, copy.deepcopy(first_value), copy.deepcopy(value))
        deep.has_call = base["has_call"] or _has_call(first_value)
        deep.touches_memory = memory or _touches_memory(first_value, self.typemap)
        out.append(deep)
        return out

    def _hoist_options(self, stmts: List[Statement], index: int, stmt: Statement,
                       call: ca.FuncCall, k: int, arg: Expression) -> List[ArgOption]:
        if isinstance(arg, (ca.ID, ca.Constant)):
            return []
        if isinstance(arg, ca.UnaryOp) and arg.op == "&" and isinstance(arg.expr, ca.ID):
            return []
        if isinstance(arg, ca.Assignment):
            return []
        if isinstance(stmt, ca.Decl) or any(isinstance(s, ca.Decl) for s in stmts[index + 1:]):
            return []  # keep C89 declarations before statements
        if (_has_call(arg) or _touches_memory(arg, self.typemap)) and \
                self._rest_of_stmt_has_call(stmt, call, k):
            return []
        try:
            type_ = decayed_expr_type(arg, self.typemap)
        except Exception:
            return []
        if isinstance(type_, ca.TypeDecl) and isinstance(type_.type, (ca.Struct, ca.Union)):
            return []
        return [ArgOption("hoist", hoist_type=type_, has_call=_has_call(arg),
                          touches_memory=_touches_memory(arg, self.typemap))]


def analyze(fn: ca.FuncDef, ast: ca.FileAST) -> List[CallSite]:
    return _Analyzer(fn, ast).run()


def combo_is_valid(site: CallSite, combo: Sequence[int]) -> bool:
    chosen = [site.options[k][c] for k, c in enumerate(combo) if c]
    calls = sum(1 for o in chosen if o.has_call)
    memory = sum(1 for o in chosen if o.touches_memory)
    if calls > 1 or (calls and memory > 1):
        return False
    removed = [id(s) for o in chosen for s in o.removed]
    return len(removed) == len(set(removed))


def _joint_is_valid(sites: Sequence[CallSite],
                    choice: Sequence[Optional[Tuple[int, ...]]]) -> bool:
    """Across sites, never fold one write twice or move two calls."""
    touched: List[int] = []
    calls = 0
    for site, combo in zip(sites, choice):
        if combo is None:
            continue
        for k, c in enumerate(combo):
            if not c:
                continue
            opt = site.options[k][c]
            touched += [id(x) for x in opt.removed]
            touched += [id(x) for x in (opt.drop_init, opt.drop_decl) if x is not None]
            calls += opt.has_call
    return calls <= 1 and len(touched) == len(set(touched))


def describe(site: CallSite, combo: Sequence[int]) -> str:
    parts = [
        site.options[k][c].describe(k, site.call.args.exprs[k])
        for k, c in enumerate(combo) if c
    ]
    return f"{site.name}(): " + "; ".join(parts)


def _unique_name(fn: ca.FuncDef, base: str) -> str:
    taken = _names(fn) | {d.name for d in _walk(fn, ca.Decl) if d.name}
    name, n = base, 1
    while name in taken:
        n += 1
        name = f"{base}{n}"
    return name


def apply(fn: ca.FuncDef, site: CallSite, combo: Sequence[int]) -> None:
    """Apply one combination to the (copied) function the site was built from."""
    stmts = ast_util.get_block_stmts(site.block, True)
    removed: List[Statement] = []
    drop_decls: List[ca.Decl] = []
    hoists: List[Statement] = []
    for k, c in enumerate(combo):
        if not c:
            continue
        opt = site.options[k][c]
        arg = site.call.args.exprs[k]
        if opt.kind == "hoist":
            var = _unique_name(fn, "call_arg")
            ast_util.insert_decl(fn, var, opt.hoist_type)
            hoists.append(ca.Assignment("=", ca.ID(var), arg))
            site.call.args.exprs[k] = ca.ID(var)
            continue
        site.call.args.exprs[k] = copy.deepcopy(opt.new_arg)
        removed += opt.removed
        if opt.drop_init is not None:
            opt.drop_init.init = None
        if opt.drop_decl is not None:
            drop_decls.append(opt.drop_decl)
    if hoists:
        at = next(i for i, s in enumerate(stmts) if s is site.stmt)
        stmts[at:at] = hoists
    for s in removed:
        stmts[:] = [x for x in stmts if x is not s]
    for d in drop_decls:
        for blk in [fn.body] + _walk(fn.body, ca.Compound):
            items = ast_util.get_block_stmts(blk, False)
            items[:] = [x for x in items if x is not d]


def perm_call_arg_staging(fn: ca.FuncDef, ast: ca.FileAST, indices: object,
                          region: object, random: Random) -> None:
    """Pick one call and restage any combination of its arguments: fold a
    preceding local write into the argument (as `v op= e` or as its value),
    fold two staged writes into one expression, or hoist an argument into a
    new local assigned just before the call. Moves never cross a call or
    memory store the expression could observe (MKD playbooks H14/H15)."""
    from src.randomizer import RandomizationFailure

    sites = [s for s in analyze(fn, ast) if region.contains_node(s.call)]
    if not sites:
        raise RandomizationFailure
    site = random.choice(sites)
    for _ in range(8):
        combo = tuple(
            0 if len(o) == 1 or random.random() < 0.5 else random.randrange(1, len(o))
            for o in site.options
        )
        if any(combo) and combo_is_valid(site, combo):
            apply(fn, site, combo)
            return
    raise RandomizationFailure


def variants(ast: ca.FileAST, fn_name: str, joint: bool = False,
             limit: int = 4096) -> Iterator[Tuple[str, ca.FileAST]]:
    """Yield (description, ast copy) for every valid combination.

    Per-site mode varies one call at a time; joint mode takes the product of
    every site's combinations (including leaving a site unchanged).
    """
    orig_fn, fn_index = ast_util.extract_fn(ast, fn_name)
    sites = analyze(orig_fn, ast)
    per_site: List[List[Optional[Tuple[int, ...]]]] = [list(s.combos()) for s in sites]
    if joint:
        choices = itertools.product(*([None] + c for c in per_site))
    else:
        choices = (
            tuple(combo if i == n else None for i in range(len(sites)))
            for n, combos in enumerate(per_site) for combo in combos
        )
    produced = 0
    for choice in choices:
        if all(c is None for c in choice):
            continue
        if produced >= limit:
            return
        fn_copy = copy.deepcopy(orig_fn)
        new_ast = copy.copy(ast)
        new_ast.ext = copy.copy(ast.ext)
        new_ast.ext[fn_index] = fn_copy
        copy_sites = analyze(fn_copy, new_ast)
        if not _joint_is_valid(copy_sites, choice):
            continue
        labels = []
        for site, combo in zip(copy_sites, choice):
            if combo is not None:
                labels.append(describe(site, combo))
        for site, combo in zip(copy_sites, choice):
            if combo is not None:
                apply(fn_copy, site, combo)
        produced += 1
        yield " | ".join(labels), new_ast


PASSES = [perm_call_arg_staging]
