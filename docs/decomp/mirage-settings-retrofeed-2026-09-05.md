# Mirage settings persistence / quality-pass report

Scope: `mcardmsg` save pause helpers, `plyrprofile` controller defaults,
`menu:p_controller_config` player walk, and `gcmcicon` overlapping icon copy.
The sibling Mirage E2E (`.cache/settings-mirage-seams-01`) passes global settings
and controller binding save/restart/restore through real menus and pristine
Aurora, with compatibility seams owned entirely by Mirage.
No Konquest or unlock behavior was exercised; those bytes were preserved.

## Retained corrections

`mcardmsg` had a private `MkProcMessageFlags` bitfield overlay for the scheduling
bit. Native GCC interpreted it as bit 0x10, while the native canonical MkProc
field/scheduler used 0x08. The save message paused its own process permanently.
Use `proc->flags_bits.skip_if_paused` in `recover_from_message`,
`prepare_for_haulting_message`, and `prepare_for_sleeping_message`. Retail
assembly and m2c confirm the same 0x08 access. All three remain exactly 100%;
the complete objdiff symbol arrays and section results are unchanged.

`plyrprofile:copy_profile_switch_defaults` used an `int[]` declaration and
literal 12-byte stride for `default_switch_map`, despite its canonical
`SwitchMapEntry` type containing two pointers. On PC this copied intervening
pointer halves as masks. Use `SwitchMapEntry[]` and `.mask`, preserving the
same word accesses and retail 12-byte stride without encoding it manually.
`set_profile_to_default` stays 96.59574%; all whole-unit symbols and sections
are unchanged. The helper is TU-local; no ABI or caller changes are needed.

## Tested native-only differences

The menu overlay begins its player at +0xA4 in retail. Native GameInfo has larger
preceding pointers. An offsetof trial could not compile because this retail
include configuration has no stddef.h. A direct canonical PlyrInfo walk built,
but moved the base and changed codegen: `p_controller_config` 98.554214% ->
96.96385%. The original GameInfo-base-plus-player-stride walk agrees with retail
and is not a fabricated algorithm; retained it at 98.554214%. Mirage derives
the overlay offset from its native GameInfo. An upstream portable overlay-size
expression remains a follow-up; no PC branches were introduced here.

Native ASan also exposed an overlapping memcpy in
`create_memorycard_write_buffer` (retail call at 0x80166CA8). Retail
`src/runtime/__mem.c:memcpy` explicitly supports overlap. A memmove trial changed
the call relocation and reduced 97.77778% -> 97.72222%; restored the valid retail
memcpy at 97.77778%, identical whole-unit symbol results. Mirage uses native
libc memmove for that call and initializes its unused write-sector padding.

## Quality gate

- Baseline/final: objdiff whole-symbol arrays unchanged for all four final units;
  reports `/tmp/settings-{mcardmsg,plyrprofile,menu,gcmcicon}-*.json`.
- Anti-pattern audit: removed duplicate scheduling bitfield and raw switch-map
  stride. No asm, register forcing, volatile coaxing, dead sinks or goto added.
- Structs/fields: canonical MkProc scheduling flag and SwitchMapEntry.mask.
  The retail player walk overlay is a documented low-level layout view; broader
  existing menu/profile reconstruction debt is outside this pass.
- Arguments: no function ABI changes. Default table declaration now agrees with
  controller's canonical type. No function-pointer or external call-site change.
- Cast/index cleanup: eliminated pointer arithmetic in the default-mask helper;
  tested, measured and classified the player-walk alternative instead of making
  an unsupported wholesale GameInfo rewrite.
- Retail identity/ownership: existing symbols, split tables, retail mcardmsg,
  menu, plyrprofile and gcmcicon assembly; no new names, canonical moves or
  Matching/NonMatching metadata changes.
- Playbooks: shared-type recovery applies; no remaining register-coloring
  manipulation attempted. Existing soft ceilings retained and status comments
  updated according to AGENTS.md.
- Verification: full ninja OK; `dtk shasum -c config/GQNE5D/build.sha1` main.dol OK;
  final objdiff symbols unchanged; scoped diff check OK.
- Deferred: native source integration (Aurora CARD, byte-order serialization,
  pointer sizes and libc overlap contract) remains in Mirage/Aurora. This source checkpoint is ready for Mirage to pin through mirage-sync; the
  portable corrections must be absorbed from its local patches. Unrelated
  worktree edits were preserved.

## Checkpoint verification

Revalidated all four whole-unit symbol arrays against the pre-fix retail
baselines: unchanged. Full ninja, DOL SHA-1 and progress gates pass. Current
reports are `/tmp/checkpoint-{mcardmsg,plyrprofile,menu,gcmcicon}.json`;
Mirage retains compact evidence under `docs/evidence/settings-retail-2026-09-05.json`.
Only the two portable corrections and their scoped status/evidence updates belong
in this source commit; unrelated game, image and decoder experiments are excluded.
