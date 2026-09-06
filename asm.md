# Assembly-only scrutiny candidates

This ledger records functions encountered during the 5% matching-floor campaign
whose retail implementation may be handwritten assembly or otherwise unsuitable
for ordinary C/C++ recovery. An entry is evidence for later scrutiny, **not**
permission to add an assembly sequence. `AGENTS.md` still requires explicit user
permission for each specific function before using that mechanism.

For donor-backed entries, evidence names the repository under
`~/projects/decomp/research` whose explicitly authored assembly has the exact
same instruction sequence as GQNE5D retail. This cross-title/source identity is
the provenance proof; mnemonic similarity alone is not treated as proof.

## Sequenced (approved and completed)

| Done | Unit | Symbol | Current fuzzy | Evidence |
| --- | --- | --- | ---: | --- |
| [x] | `main/mtx.a/vec` | `PSVECMag` | 100% | Research proof: `dolsdk2004/src/mtx/vec.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/vec` | `PSVECNormalize` | 100% | Research proof: `dolsdk2004/src/mtx/vec.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/vec` | `PSVECCrossProduct` | 100% | Research proof: `dolsdk2004/src/mtx/vec.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/mtxvec` | `PSMTXMultVec` | 100% | Research proof: `dolsdk2004/src/mtx/mtxvec.c` contains the exact retail `nofralloc` assembly sequence. |
| [x] | `main/mtx.a/mtx` | `PSMTXConcat` | 100% | Research proof: `dolsdk2004/src/mtx/mtx.c` explicitly authors the exact retail instruction and `Unit01` relocation sequence as assembly. |
| [x] | `main/mtx.a/mtx` | `PSMTXIdentity` | 100% | Research proof: both `dolsdk2004/src/mtx/mtx.c` and Gauntlet: Dark Legacy `src/dolphin/mtx/mtx.c` explicitly author the same exact `0x2C` retail sequence as assembly. |
| [x] | `main/mtx.a/mtx` | `PSMTXTrans` | 100% | Research proof: `dolsdk2004/src/mtx/mtx.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/mtx` | `PSMTXScale` | 100% | Research proof: `dolsdk2004/src/mtx/mtx.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/mtx` | `PSMTXQuat` | 100% | Research proof: `dolsdk2004/src/mtx/mtx.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/quat` | `PSQUATMultiply` | 100% | Research proof: `dolsdk2004/src/mtx/quat.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/mtx.a/quat` | `PSQUATNormalize` | 100% | Research proof: `dolsdk2004/src/mtx/quat.c` explicitly authors the exact retail instruction sequence as assembly. |
| [x] | `main/os.a/OSContext` | `__OSLoadFPUContext` | 100% | Research proof: `dolsdk2004/src/os/OSContext.c` contains the exact retail function-level assembly sequence. |
| [x] | `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Os/dolphin/dolphin_trk` | `InitMetroTRK` | 100% | Research proof: Gauntlet: Dark Legacy `src/TRK_MINNOW_DOLPHIN/Os/dolphin/dolphin_trk.c` explicitly authors the same MetroTRK bootstrap assembly sequence. |
| [x] | `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Os/dolphin/dolphin_trk` | `InitMetroTRK_BBA` | 100% | User-approved retail-derived sequence; the BBA-specific sibling differs only in MSR handling and communication-table selection. Symbol and branch relocations are retained. |
| [x] | `main/rpskin.a/skingcnasm` | `_rwDlSkinUpdate2WeightsP`, `_rwDlSkinUpdate2WeightsPN`, `_rwDlSkinUpdate3WeightsP`, `_rwDlSkinUpdate3WeightsPN`, `_rwDlSkinUpdate4WeightsP`, `_rwDlSkinUpdate4WeightsPN` | 100% | User-approved retail-derived sequences for the six paired-single skinning kernels; each wrapper retains a brief algorithm-intent comment. |

## Confirmed handwritten assembly

| Unit | Symbol | Current fuzzy | Evidence | Campaign action |
| --- | --- | ---: | --- | --- |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Os/dolphin/dolphin_trk_glue` | `TRKLoadContext` | 0% | Retail restores GPRs with `lmw`, writes CR/LR/CTR/XER/MSR and SPRG registers, then branches directly to `TRKInterruptHandler`; this is a privileged context-restore entry, not compiler-generated C. | Skip during unattended floor work; requires explicit per-function permission before any assembly sequence. |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Processor/ppc/Generic/targimpl` | `__TRK_get_MSR`, `__TRK_set_MSR`, `TRK_ppc_memcpy`, `TRKInterruptHandler`, `TRKExceptionHandler`, `TRKSwapAndGo`, `TRKInterruptHandlerEnableInterrupts`, `ReadFPSCR`, `WriteFPSCR` | 0% | Retail directly reads/writes MSR and FPSCR, switches MSR around byte accesses, saves/restores complete CPU contexts, manipulates SRR/SPRG/CR/LR/CTR/XER/DAR/DSISR, and returns with `rfi`; these routines implement privileged register contracts rather than compiler-generated C. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/gx.a/GXTransform` | `GXSetProjectionv` | 4.285714% | Retail copies the six projection floats and streams them to WGPIPE with paired-single `psq_l`/`psq_st`. The canonical SDK donor expresses this through register-qualified inline assembly, while the repository keeps an honest typed scalar implementation. | Keep the scalar C soft ceiling and skip during unattended floor work; requires explicit permission before any assembly sequence. |
| `main/Runtime.PPCEABI.H.a/Gecko_setjmp` | `__setjmp`, `longjmp` | 0% | Retail saves/restores fixed GPR and FPR sets with `stmw`/`lmw`, `stfd`/`lfd`, paired-single register halves, CR/LR, and ABI state. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/Runtime.PPCEABI.H.a/runtime` | whole unit | 0% | Retail contains compiler ABI helper bodies, including fixed-register GPR/FPR save/restore entry chains and hand-lowered 64-bit divide, shift, and conversion routines. | Skip during unattended floor work; scrutinize individual helpers before any explicitly permitted assembly sequence. |
| `main/base.a/PPCArch` | whole unit | 0% | Retail directly reads/writes MSR, HID, performance-counter, WPAR, and FPSCR state and supplies synchronization/halt primitives through privileged instructions. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Processor/ppc/Generic/flush_cache` | `TRK_flush_cache` | 0% | Retail directly issues `dcbst`, `dcbf`, `sync`, `icbi`, and `isync` over cache-line boundaries. | Skip during unattended floor work; requires explicit permission before any assembly sequence. |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Processor/ppc/Generic/mpc_7xx_603e` | `TRKSaveExtended1Block`, `TRKRestoreExtended1Block` | 0% | Retail saves/restores fixed GPR blocks and a large set of named and numeric SPRs, including GQR, HID, DMA, performance-monitor, thermal, and breakpoint registers. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Processor/ppc/Export/targsupp` | `pad_01_80201430_text` | 0% | The unit contains only a local padding-labelled 32-byte text body, not a callable C function. | Leave for later split/padding scrutiny; do not synthesize a C function or assembly sequence. |
| `main/vmbase.a/VMBase` | `__VMBASEInvalidateEntireTLB` | 4.5454545% | Retail issues the privileged `tlbia`, `sync`, and `isync` sequence; ordinary C cannot express the TLB invalidation contract. | Skip during unattended floor work; requires explicit permission before any assembly sequence. |
| `main/vmbase.a/VMBase` | `__VMBASESetupVMRegisters` | 4.7619047% | Retail reads and replaces MSR/segment-register/SDR1 state and transitions through an `rfi`-based helper. | Skip during unattended floor work; requires explicit permission before any assembly sequence. |
| `main/vmbase.a/VMBase` | `__VMBASESetupExceptionHandlers`, `__VMBASERestoreExceptionHandlers` | 1.0526316% / 3.8888888% | Retail installs/restores exception-vector instruction payloads and explicitly synchronizes and invalidates instruction-cache lines with `sync`, `icbi`, and `isync`. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/vmbase.a/VMBase` | `__VMBASEDSIExceptionHandler`, `__VMBASEISIExceptionHandler` | 2.1967213% each | These retail exception entries save fixed registers through SPRG, manipulate SRR/MSR state, and return via `rfi`; they are architectural entry stubs rather than compiler-generated C. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |
| `main/vmbase.a/VMBase` | `__VMBASEDSIServiceExceptionPrep`, `__VMBASEISIServiceExceptionPrep` | 4.0% / 4.2105265% | Retail constructs fixed exception frames with `stmw` and captures GQR and other architectural registers through `mfspr`; the register contract is not representable in ordinary C. | Skip during unattended floor work; each function requires explicit permission before any assembly sequence. |

## High-confidence candidates

These require retail instruction review before being promoted to the confirmed
section.

| Unit | Symbols / scope | Reason for scrutiny |
| --- | --- | --- |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Processor/ppc/Generic/__exception` | exception-vector payload | Retail symbol is a large init-section pad/vector body, not an ordinary C function. |
| `main/TRK_MINNOW_DOLPHIN.a/MetroTRK/Os/dolphin/dolphin_trk` | `TRK__write_aram`, `TRK__read_aram` | ARAM DMA routines contain explicit `dcbf`/`dcbi`/`sync` cache operations; inspect whether canonical SDK intrinsics can express them before classifying them as handwritten assembly. |

## Review rules

- Confirm from retail instructions, relocations, symbol boundaries, and known SDK
  source provenance; unit or symbol names alone are not proof.
- Keep ordinary compiler-generated functions in the C/C++ decompilation path.
- Do not add `SEQ_*`, embedded assembly, register forcing, or synthetic fallbacks
  while investigating a candidate.
- Record newly encountered candidates here even when the floor campaign skips
  them, so they can be reviewed explicitly later.
