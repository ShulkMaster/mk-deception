# Naming conventions

Naming follows canonical source families, not a blanket C-versus-C++ rule. The
retail symbol table contains several conventions because the executable combines
Midway game code, platform code, RenderWare, Dolphin SDK, Sofdec, and other
middleware.

## Canonical names always win

Preserve exact spelling and capitalization recovered from:

1. retail ELF symbols and verified address ranges;
2. DWARF or surviving debug information;
3. relocations, linker maps, debug strings, and source-path evidence;
4. authenticated upstream SDK or library source for the same version.

Do not rename a canonical symbol to make it conform to this guide. Add a comment
when its purpose needs clarification. A nearby naming pattern is evidence for a
new placeholder, but it cannot override a recovered name.

For a proposed rename, confirm the function address and range in
`config/GQNE5D/symbols.txt` or the retail ELF. Repository spelling by itself is
not proof: some current names were introduced during reconstruction.

## C naming by source family

### Midway game and runtime C

Gameplay, process, menu, Konquest, object, and runtime functions are
predominantly `snake_case` in the canonical symbol table:

```c
reset_game_speed();
get_inverse_game_speed();
gc_setup_feedback_buffer_for_konquest();
get_num_controllers();
scan_switches();
turn_rumble_off();
```

Preserve established semantic prefixes such as `p_`, `r_`, `x_`, `get_`,
`set_`, `init_`, `gc_`, `mk_`, and `plyr_`. These prefixes often encode process
roles or subsystem ownership and should not be converted to cosmetic CamelCase.

For a genuinely introduced game-C helper with no recoverable name, use a short
descriptive `snake_case` name consistent with adjacent canonical functions.
File-static helpers follow the same style; storage duration does not require a
special name prefix.

### Midway platform and rendering C

Platform and rendering units are mixed. Some use game-style snake case, while
others use CamelCase or lowerCamelCase:

```c
gc_native_display_render();
CheckFor480PMode();
displayContinueMessage();
ProcessSpecularity();
GCNSetupNonRenderwarePipeline();
inplaceSkinGeometryNativeRead();
```

Use the convention of the canonical functions in that translation unit and
subsystem. Do not normalize a mixed retail unit globally.

### Dolphin SDK C

Public Dolphin APIs conventionally use an uppercase subsystem prefix followed
by CamelCase:

```c
OSInitAlarm();
OSSetAlarm();
GXSetTevColor();
CARDRead();
SPGetSoundEntry();
```

Private SDK symbols may use leading underscores, all-caps subsystem prefixes,
or lowercase internal names. Recover them from the matching SDK version; do not
apply the game-code snake-case default to SDK functions.

### RenderWare C

RenderWare names encode type and subsystem ownership in their prefixes:

```c
RwStreamRead();
RpGeometryStreamRead();
_rwDolphinHeapAlloc();
_rpNativeRead();
```

Keep `Rw`, `Rp`, `Rt`, `_rw`, and `_rp` capitalization exactly. Use the matching
RenderWare source or retail symbols for missing names. Do not translate these
APIs into either generic snake_case or unprefixed CamelCase.

### Midway middleware and bundled C libraries

The `mwMem`, `mwFile`, screen-engine, and related Midway libraries commonly use
lowerCamelCase with a lowercase library prefix:

```c
mwMemHeapGetInfo();
mwMemSystemSetParams();
fixedBlockHeapAlloc();
privGetUsedHdrFromBlock();
mwFileOpen();
```

Private helpers often begin with `priv`, while some platform glue remains
snake_case. Sofdec and other third-party libraries must follow their own
authenticated upstream convention. Do not infer a shared style merely because
both libraries are written in C.

## C++ naming

C++ does not imply that every free function becomes CamelCase. Preserve the
source family's free-function convention. For example, game glue can remain
snake_case while an `mw*` class uses middleware-style names.

- Classes, structs, and enums normally preserve canonical type names such as
  `MwMemHeapInfo`, `RwStream`, or `CARDFileInfo`.
- Member functions preserve the class/library convention. Do not rename a
  recovered lowerCamelCase member to PascalCase.
- Constructors, destructors, conversion operators, `operator new`, and
  `operator delete` use their required C++ spelling.
- Namespaces, templates, overloads, and mangled symbols must agree with retail
  ABI evidence; style preference cannot justify an ABI change.

For an introduced local class or helper type, mirror the surrounding canonical
C++ unit. Use PascalCase only when that unit's types consistently do so.

## Variables, members, and constants

Local-variable names rarely survive in this retail binary. Choose concise names
that state confirmed behavior and match the containing source family:

- use `snake_case` by default in Midway game C;
- follow authenticated upstream style in SDK and library reconstructions;
- retain conventional short names such as `i`, `count`, `obj`, or `player` when
  they accurately describe the value;
- use `arg1`, `arg2`, and `var1`, `var2` only while meaning is unknown.

Never encode guessed semantics in a name. Rename a placeholder once callers,
accesses, or layout evidence establishes its role.

Keep unknown structure members and padding named by verified offset:

```c
field_1C
pad_20
```

Replace an offset name only after the member meaning and type are supported.
Preserve canonical global prefixes and capitalization when known. For introduced
internal names, prefer the local unit's established pattern over universal
`g_`/`s_` prefixes.

Macros and enum constants should follow the owning API. New project-local macros
may use `MK_SCREAMING_SNAKE_CASE`; Dolphin, RenderWare, and middleware constants
retain their canonical prefixes. Do not add playful `K` substitutions unless
the spelling is present in canonical evidence.

## Introduced-name checklist

Before adding a name that will appear in source:

1. Search `config/GQNE5D/symbols.txt` and the retail ELF by address/range.
2. Check callers, callees, relocations, strings, headers, and neighboring
   functions in the same retail translation unit.
3. For SDK or library code, check the authenticated matching upstream version.
4. Select the source family: game/runtime, platform/rendering, Dolphin,
   RenderWare, Midway middleware, Sofdec, or another bundled library.
5. Follow that family's local convention and keep uncertain names neutral.
6. Document why a non-canonical public name was introduced so it can be replaced
   when stronger evidence appears.
