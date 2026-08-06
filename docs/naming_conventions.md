# Naming conventions

These conventions distinguish canonical names recovered from the original game
from names introduced during decompilation. Above all else, preserve the
original developers' names when evidence for them exists.

## Sources of truth

Names recovered from ELF symbols, DWARF data, surviving debug information,
debug strings, or references in code strings are canonical. Preserve their
exact spelling and capitalization, even when they do not follow the conventions
below.

For Dolphin SDK, Sofdec, RenderWare, and other external library code, research
the original upstream name when it is absent from the available debug
information. Existing names supported by debug information remain the absolute
source of truth.

Do not rename a canonical symbol merely to make it more descriptive or
consistent, for that add a documentation comment instead.

## General style

Use `snake_case` for C functions and variables and `PascalCase` for types when
introducing a name that is not canonical. Function arguments and local
variables should use concise, descriptive names based on confirmed behavior.
When a descriptive name cannot be assigned because the code is not yet
understood, name arguments with the `arg$` prefix and local variables with the
`var$` prefix. Replace these fallback names once evidence supports a descriptive
name.

Prefer established **K** spellings in game-related code when they fit the
original programmers' Mortal Kombat style. Examples include `combat` ->
`kombat`, `constant` -> `konstant`, and `container` -> `kontainer` where the compiler
and context allow it. This is a small personality touch left to contributor
discretion, not a requirement. Avoid it in Dolphin SDK, RenderWare, Sofdec, and
other external library code unless the original library uses that spelling.

## Introduced symbols

Symbols introduced during decompilation must follow the convention for their
category so that reviewers can distinguish them from canonical names. These
prefixes apply only when no canonical name can be recovered.

| Symbol category | Convention | Example |
| --- | --- | --- |
| Inline helper function | `fn_snake_case` | `fn_decode_flags` |
| Macro | `MK_SCREAMING_SNAKE_CASE` | `MK_ALIGN_SIZE` |
| Enum constant | `EK_SCREAMING_SNAKE_CASE` | `EK_STATE_ACTIVE` |
| Global constant | `GK_SCREAMING_SNAKE_CASE` | `GK_MAX_PLAYERS` |
| File-static constant | `SK_SCREAMING_SNAKE_CASE` | `SK_BUFFER_SIZE` |
| Global variable | `g_snake_case` | `g_active_player` |
| File-static variable | `s_snake_case` | `s_current_state` |
| Unknown function argument | `argN` | `arg1` |
| Unknown local variable | `varN` | `var1` |
| Midway game struct | `MKPascalCase` | `MKPlayerState` |
| Unknown struct member | `field_OFFSET` | `field_1C` |
| Padding member | `pad_OFFSET` | `pad_20` |

New macros must use the `MK_` prefix. If a macro name can be derived from debug
information or a surviving source reference, use that canonical name instead.

All Midway game structs introduced without a canonical name use the `MK`
prefix. Do not apply it to Dolphin SDK, RenderWare, Sofdec, or other external
library types; recover those names from their original libraries where
possible.

Keep unknown struct members named by offset, such as `field_1C`, until evidence
supports a semantic name. Use the same offset style for padding, such as
`pad_20`. Do not guess member meaning merely to make a struct look complete.

For another symbol that does not survive compilation and has no debug-derived
name, follow the closest category above and choose a name that clearly reads as
introduced rather than canonical. Explain unusual cases in the pull request.
