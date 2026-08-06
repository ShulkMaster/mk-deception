# Contributing to MK Deception (GC)

## Prerequisites

- Own a copy of *Mortal Kombat: Deception* (GameCube, USA, DOL-GQNE-USA)
- Python 3.10+, ninja, Wine or [wibo](https://github.com/decompals/wibo)
- [objdiff](https://github.com/encounter/objdiff) for local diffing

## Project status

This project is still at an early stage, but we accept focused PRs that follow the contribution process below.

Our immediate goal is to pin down the project details and establish a solid initial foundation that will make future contributions easier and more consistent.

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

## Thanks

Thank you for your interest and for helping move the decompilation forward in small, reviewable steps.
