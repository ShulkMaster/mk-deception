# Contributing to MK Deception (GC)

## Prerequisites

- Own a copy of *Mortal Kombat: Deception* (GameCube, USA, DOL-GQNE-USA)
- Python 3.10+, ninja, Wine or [wibo](https://github.com/decompals/wibo)
- [objdiff](https://github.com/encounter/objdiff) for local diffing

## Project status

Project structure and naming are now in a stable phase for contributions. All file and function names have been extracted from canonical sources (ELF, DWAFT, and surviving debug metadata) and applied to the existing code base.

A large chunk of gameplay modules has been recovered and is close to complete; the main priority is finishing these near-100% areas to close out the full match workflow.

All files and functions are already present in the repository, and the file structure is expected to remain stable.

An AI workflow is currently running broad decompilation passes to automate as much recovery as possible and establish a stronger base for contributors. Because of that, avoid editing untouched `TODO` functions; broad passes may still overwrite them.

The immediate goal is to prioritize high-confidence recovered gameplay work and keep contributions reviewable.

### Naming conventions

- Follow the project naming guidelines in [docs/naming_conventions.md](docs/naming_conventions.md) when updating names.

### PRs

- Use a separate branch for each PR (do not develop directly on `main`).
- Prefer work in this order:
  - functions near 100%
  - struct layout reconstructions
  - new functions
- Keep PRs small and isolated when possible:
  - one function or one file per PR when feasible
  - avoid giant PRs with thousands of changed lines
- For existing files and functions, avoid creating new files; headers are the only exception.
- Document the reason for any regression or struct / struct member rename in the PR description.
- Include a brief summary and any assumptions in the PR description.

## Thanks

Thank you for your interest and for helping move the decompilation forward in small, reviewable steps.
