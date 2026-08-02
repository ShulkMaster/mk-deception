# Naming conventions

- Prefer established **K** spellings over C spellings: `kombat`, `konquest`,
  `koin`, and similar terms. The original developers carried the game's naming
  style into source code; follow retail terminology instead of correcting it.
- Preserve retail symbol names whenever they are known, including capitalization
  and unusual spelling.
- Use `snake_case` for C functions and variables and `PascalCase` for types,
  unless retail evidence says otherwise.
- Prefix globals with `g_` and file-static variables with `s_` when no retail
  name is known.
- Use `kName` for constants and `skName` for file-static constants (`sk` means
  **static konstant**).
- Name unknown members `field_0xNN` and padding `pad_0xNN`; do not guess meaning.
- Use descriptive local names based on confirmed behavior. Keep address-based or
  unknown names until evidence supports a semantic rename.
