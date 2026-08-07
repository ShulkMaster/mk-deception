# Mortal Kombat: Deception

[![Build Status]][actions] [![Code Progress]][progress] [![Data Progress]][progress]

[Build Status]: https://github.com/ShulkMaster/mk-deception/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/ShulkMaster/mk-deception/actions/workflows/build.yml
[Code Progress]: https://decomp.dev/ShulkMaster/mk-deception.svg?mode=shield&measure=code&label=Code
[Data Progress]: https://decomp.dev/ShulkMaster/mk-deception.svg?mode=shield&measure=data&label=Data
[progress]: https://decomp.dev/ShulkMaster/mk-deception

A work-in-progress decompilation of the Nintendo GameCube version of *Mortal Kombat: Deception*.

Released by Midway in February 2005, *Mortal Kombat: Deception* is the sixth entry in the Mortal Kombat series. The game is also the second entry in the Mortal Kombat 3D era and features improvements over previous installments, such as an expanded Konquest mode.

This repository does **not** contain any game assets or assembly. An existing copy of the game is required.

Supported versions:

- `GQNE5D`: USA

## Local setup

Python is the only hard prerequisite for the bootstrap script. From the
repository root, run:

```sh
python3 tools/init.py
```

The script performs the complete setup in one pass. It validates the retail
input, initializes Git submodules when present, installs or updates m2c under
`build/m2c`, downloads the configured CodeWarrior compilers and matching tools
under `build/`, generates the build files, and runs the full Ninja build.
Missing host programs such as Git or Ninja are reported in the final checklist.
There are no dry-run, offline, or skip-build modes.

Place a raw `GQNE5D` ISO/GCM or an extracted disc tree under `orig/GQNE5D`.
The expected raw image SHA-1 is
`489c6b57b70390933dff7d8d9d12424f58a8f821`; extracted trees are checked by
the configured retail `main.dol` SHA-1 instead.
An ISO elsewhere can be validated explicitly:

```sh
python3 tools/init.py --iso /path/to/game.iso
```

The final output provides an explicit readiness checklist:

```text
[x] Python
[x] Ninja
[x] Git
[x] ISO SHA-1
[x] ISO size
[x] ISO game ID
[x] Retail main.dol SHA-1
[x] DTK retail input
[x] m2c update
[x] m2c smoke test
[x] Tool: compilers
[x] Tool: dtk
[x] Tool: objdiff-cli
[x] Tool: sjiswrap
[x] Tool: binutils
[x] Tool: wibo
[x] Generate build files
[x] Initial matching build

READY: retail input, matching tools, configuration, and build checks passed.
```

A successful build also prints `build/GQNE5D/main.dol: OK`. Any required
validation, download, configuration, or build failure produces `NOT READY` and
a nonzero exit status.
