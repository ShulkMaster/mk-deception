#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GQNE5D",  # 0 -- USA (DOL-GQNE-USA)
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
# Request preset on decomp.me Discord; see CONTRIBUTING.md
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

config.linker_version = "GC/2.7"

# Game code compiler: GC/2.7 (mwcceppc 2.4.7 build 108). Evidence summary:
# - Matching TUs .text-match retail under GC/2.7; GC/3.0a3 ruled out by codegen.
# - GC/2.0-2.7 emit identical .text on tested TUs; only GC/2.7 writes .comment v=0x0B
#   (config mw_comment_version: 11). MAP .dtors$10 => original linker was GC 2.7+.
# Re-probe: python3 tools/mw_version_probe.py [--bakeoff]
# SDK / RW libs use per-lib mw_version overrides (GCN_Nightly_Build / BFBB-style).
game_mw_version = "GC/2.7"
# SoftDec exposes source-level symbols and carries the same retail v=0x0B
# CodeWarrior marker as game objects, so use the game compiler explicitly.
softdec_mw_version = game_mw_version


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_base,
        "progress_category": "sdk",
        "objects": objects,
    }


# RenderWare Graphics (Criterion) -- BFBB uses GC/1.3.2 + these flags for rwsdk.
# Linker stays GC/2.7. Portable TUs only first; GC driver (*Gcn*, _rwDl*) later.
cflags_renderware = [
    *cflags_base,
    "-lang=c",
    "-fp fmadd",
    "-fp_contract off",
    "-char signed",
    "-str reuse",
    "-common off",
    "-O4,p",
]


def RenderWareLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_renderware,
        "progress_category": "renderware",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = False
config.warn_missing_source = False
config.libs = [
    {
        "lib": "game",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "game",
        "objects": [
            Object(Matching, "debug_file.o", source="runtime/debug_file.c"),
            Object(NonMatching, "mk_cmdscript.o", source="runtime/mk_cmdscript.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "script_functions.o", source="runtime/script_functions.c"),
            Object(NonMatching, "mk_hwfile.o", source="runtime/mk_hwfile.c",
                   extra_cflags=["-inline off", "-use_lmw_stmw on", "-O4,s"]),
            # Huge .rodata/.data (file tables + path pool); NonMatching = ASM still linked
            Object(NonMatching, "mk_fileinfo.o", source="runtime/mk_fileinfo.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(Matching, "pakfile.o", source="runtime/pakfile.c"),
            Object(Matching, "mk_vtbl.o", source="runtime/mk_vtbl.c"),
            Object(
                NonMatching,
                "mk_plugins.o",
                source="runtime/mk_plugins.c",
                extra_cflags=["-use_lmw_stmw on", "-O4,s"],
            ),
            # gxVectAngleZX not fully matched yet; keep split until fixed
            Object(NonMatching, "gxVect.o", source="math/gxVect.c"),
            Object(NonMatching, "gxQuat.o", source="math/gxQuat.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(
                NonMatching,
                "gxMat.o",
                source="math/gxMat.c",
                extra_cflags=["-use_lmw_stmw on", "-O4,s"],
            ),
            # Trig polynomials + GXMathSqrtTable .data; NonMatching = ASM linked
            Object(NonMatching, "gxMath.o", source="math/gxMath.c"),
            # Midway V3/XZ/quat/MKMATRIX core; Wave C scaffold
            Object(NonMatching, "mk_math.o", source="math/mk_math.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(Matching, "MovieSubtitle_GC.o", source="movie/MovieSubtitle_GC.cpp",
                   extra_cflags=["-lang=c"]),
            Object(Matching, "fog.o", source="platform/fog.c"),
            Object(Matching, "fast_rw.o", source="platform/fast_rw.c"),
            Object(NonMatching, "gcspecskin.o", source="game/gcspecskin.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "gcinstance.o", source="platform/gcinstance.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "gcpipemanager.o", source="platform/gcpipemanager.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "joy.o", source="platform/joy.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(Matching, "MovieConfig.o", source="movie/MovieConfig.cpp"),
            Object(NonMatching, "MovieManager.o", source="movie/MovieManager.cpp",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "MovieManagerGC_Disp.o", source="movie/MovieManagerGC_Disp.cpp", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "MkMovies.o", source="movie/MkMovies.cpp",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(Matching, "gprofile_gcn.o", source="platform/gprofile_gcn.c"),
            Object(Matching, "mwMemNewDelete.o", source="mw/mwMemNewDelete.cpp"),
            Object(Matching, "mwFileGlue.o", source="mw/mwFileGlue.cpp"),
            # ProcessFrame register scheduling not matched yet
            Object(
                Matching,
                "MovieManagerGC_RW_Disp.o",
                source="movie/MovieManagerGC_RW_Disp.cpp",
                extra_cflags=["-use_lmw_stmw", "on", "-O4,s"],
            ),
            Object(NonMatching, "pwrbar.o", source="game/pwrbar.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "fx.o", source="game/fx.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "jab.o", source="game/jab.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "blood.o", source="game/blood.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "pfxscript.o", source="game/pfxscript.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "reactions.o", source="game/reactions.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "fatality.o", source="game/fatality.c",
                   extra_cflags=["-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "ncs.o", source="game/ncs.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "collision.o", source="game/collision.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline noauto"]),
            Object(NonMatching, "ejb.o", source="game/ejb.c",
                   extra_cflags=["-inline noauto", "-use_lmw_stmw on"]),
            Object(NonMatching, "moves.o", source="game/moves.c",
                   extra_cflags=["-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(Matching, "mwMem_MultiThread.o", source="mw/mwMem_MultiThread.c"),
            Object(NonMatching, "mwMemPlatform.o", source="mw/mwMemPlatform.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "mwMemPriv.o", source="mw/mwMemPriv.c",
                   extra_cflags=["-opt", "off", "-O4,s"]),
            Object(NonMatching, "mwMemHdrless.o", source="mw/mwMemHdrless.c",
                   extra_cflags=["-opt", "off", "-O4,s", "-use_lmw_stmw on"]),
            Object(Matching, "mwMemDebug.o", source="mw/mwMemDebug.c"),
            Object(NonMatching, "mwMemFixed.o", source="mw/mwMemFixed.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "mwMemNormal.o", source="mw/mwMemNormal.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"]),
            Object(NonMatching, "mwMem.o", source="mw/mwMem.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(Matching, "sound_assets.o", source="game/sound_assets.c"),
            Object(Matching, "sound_settings.o", source="game/sound_settings.c"),
            Object(Matching, "sound_groups.o", source="game/sound_groups.c"),
            Object(Matching, "anims.o", source="runtime/anims.c"),
            Object(NonMatching, "mk_anim.o", source="runtime/mk_anim.c"),
            Object(NonMatching, "controller.o", source="game/controller.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "switch.o", source="game/switch.c"),
            Object(NonMatching, "game.o", source="game/game.c"),
            Object(NonMatching, "plyr.o", source="game/plyr.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "io.o", source="platform/io.c",
                   extra_cflags=["-O4,s"]),
            Object(NonMatching, "shadow.o", source="runtime/shadow.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "hashtable.o", source="runtime/hashtable.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "section_slot_file.o", source="runtime/section_slot_file.c", extra_cflags=["-use_lmw_stmw on", "-str reuse,pool,readonly", "-O4,s"]),
            Object(NonMatching, "section.o", source="runtime/section.c", extra_cflags=["-use_lmw_stmw on", "-str reuse,pool,readonly", "-O4,s"]),
            # SEC decode (process_art_section_data / load_tga); NonMatching = ASM linked
            Object(NonMatching, "asset.o", source="runtime/asset.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "bgnd_nbc.o", source="game/bgnd_nbc.c"),
            Object(NonMatching, "specular.o", source="game/specular.c", extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "minigames.o", source="game/minigames.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "pz_moves.o", source="game/pz_moves.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on", "-str reuse,pool,readonly"]),
            Object(NonMatching, "pz_fatality.o", source="game/pz_fatality.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "pz_fighters.o", source="game/pz_fighters.c",
                   extra_cflags=["-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "mk_chess.o", source="game/mk_chess.c"),
            Object(NonMatching, "bgnd_jmt.o", source="game/bgnd_jmt.c", extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "bgnd_jtb.o", source="game/bgnd_jtb.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "mcardmsg.o", source="game/mcardmsg.c"),
            # B20 Wave B: game memcard (profile callees); CARD I/O is gcmcard
            Object(NonMatching, "memcard.o", source="game/memcard.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "sound.o", source="game/sound.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s",
                                 "-str reuse,pool,readonly"]),
            Object(Matching, "nbc.o", source="game/nbc.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "acb.o", source="game/acb.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "image.o", source="runtime/image.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "fonts.o", source="runtime/fonts.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            # Readable TGA writer; codegen not matched (removed retail-asm dump)
            Object(NonMatching, "tga.o", source="runtime/tga.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s", "-str", "reuse,pool,readonly"]),
            Object(NonMatching, "instance.o", source="runtime/instance.c", extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "light.o", source="runtime/light.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "gcInit.o", source="platform/gcInit.c"),
            # Boot -> loading screen (gcdisplay owns loading_image / tile / native display)
            Object(NonMatching, "gcdisplay.o", source="platform/gcdisplay.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "main.o", source="platform/main.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "gcARam.o", source="platform/gcARam.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s", "-opt", "nocse"]),
            Object(NonMatching, "mtRand2.o", source="runtime/mtRand2.c", extra_cflags=["-O4,s"]),
            Object(NonMatching, "utils.o", source="runtime/utils.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "mk_mem.o", source="runtime/mk_mem.c",
                   extra_cflags=["-opt", "off", "-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "mk_struct.o", source="runtime/mk_struct.c",
                   extra_cflags=["-use_lmw_stmw on", "-str reuse,pool,readonly", "-O4,s",
                                 "-inline off"]),
            Object(Matching, "mk_pdata.o", source="runtime/mk_pdata.c",
                   extra_cflags=["-use_lmw_stmw on", "-opt", "off", "-O4,s"]),
            Object(NonMatching, "mk_proc.o", source="runtime/mk_proc.c",
                   extra_cflags=["-opt", "off", "-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "mk_pebble.o", source="runtime/mk_pebble.c",
                   # TU-wide CSE disable replaces create_pebble_userdata pragmas
                   # and also improves pebble_render_callback.
                   extra_cflags=[
                       "-opt", "off", "-O4,s", "-opt", "nocse",
                       "-use_lmw_stmw on",
                   ]),
            Object(NonMatching, "mk_obj.o", source="runtime/mk_obj.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "mk_particle.o", source="runtime/mk_particle.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "bgnd.o", source="game/bgnd.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "bgnd_mab.o", source="game/bgnd_mab.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "constrain.o", source="game/constrain.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "ai.o", source="game/ai.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "jmt.o", source="game/jmt.c"),
            Object(NonMatching, "projectile.o", source="game/projectile.c"),
            Object(NonMatching, "konquest.o", source="game/konquest.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "konquest_missions.o",
                   source="game/konquest_missions.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "konquest_npc.o", source="game/konquest_npc.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "konquest_interior.o",
                   source="game/konquest_interior.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "konquest_time.o",
                   source="game/konquest_time.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "cam.o", source="runtime/cam.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "krypt.o", source="game/krypt.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "mab.o", source="game/mab.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "gcmcicon.o", source="platform/gcmcicon.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s",
                                 "-str reuse,pool,readonly"]),
            # B20 Wave C: Midway GC memcard facade (CARD*); card.a out
            # -str pool,readonly: retail pools "" / "MKD" into @stringBase0 in
            # .rodata (far lis/addi); plain -str reuse emits @sda21 loads.
            Object(NonMatching, "gcmcard.o", source="platform/gcmcard.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "gcmcardmsg.o", source="platform/gcmcardmsg.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "gcio.o", source="platform/gcio.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "disc_error.o", source="platform/disc_error.c",
                   extra_cflags=["-use_lmw_stmw on", "-O4,s", "-str", "reuse,pool,readonly"]),
            Object(NonMatching, "gcutils.o", source="platform/gcutils.c", extra_cflags=["-use_lmw_stmw on", "-O4,s", "-common off"]),
            Object(NonMatching, "settings.o", source="game/settings.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "nis.o", source="game/nis.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "attract.o", source="game/attract.c", extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "menu.o", source="game/menu.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            # B19 Wave A: character select entry (p_pselect*); rest stays ASM
            Object(NonMatching, "pselect.o", source="game/pselect.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "cloth.o", source="game/cloth.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "weapon.o", source="game/weapon.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on",
                                 "-str reuse,pool,readonly"]),
            Object(NonMatching, "ladder.o", source="game/ladder.c",
                   extra_cflags=["-O4,s"]),
            Object(NonMatching, "ending.o", source="game/ending.c"),
            # B15 P0: MAIN_MENU C APIs; rest of ~61KB Glue stays ASM (NonMatching)
            Object(NonMatching, "mwScreenEngineGlue.o", source="mw/mwScreenEngineGlue.cpp",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on", "-bool off"]),
            Object(NonMatching, "plyrprofile.o", source="game/plyrprofile.c",
                   extra_cflags=["-O4,s", "-use_lmw_stmw on"]),
            Object(NonMatching, "konquest_items.o", source="game/konquest_items.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "mwMemHeap.o", source="mw/mwMemHeap.c",
                   extra_cflags=["-use_lmw_stmw on", "-str", "reuse,pool,readonly"]),
            Object(NonMatching, "konquest_nav.o", source="game/konquest_nav.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "display.o", source="platform/display.c", extra_cflags=["-use_lmw_stmw on", "-O4,s"]),
            Object(NonMatching, "jdn.o", source="game/jdn.c", extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "mk_render.o", source="runtime/mk_render.c",
                   extra_cflags=["-use_lmw_stmw on"]),
            Object(NonMatching, "konquest_save.o", source="game/konquest_save.c",
                   extra_cflags=["-use_lmw_stmw on"]),
        ],
    },
    {
        "lib": "libmwsfdg",
        "mw_version": softdec_mw_version,
        "cflags": cflags_base,
        "progress_category": "sofdec",
        "objects": [
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/mwply/mwsfx_ARGB8888PLN.o",
                source="libmwsfdg/crimw/dev/sofdec/src/mwply/mwsfx_ARGB8888PLN.c",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/mwply/mwsfx_Y84C44.o",
                source="libmwsfdg/crimw/dev/sofdec/src/mwply/mwsfx_Y84C44.c",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/mwply/mwsfdrsc.o",
                source="libmwsfdg/crimw/dev/sofdec/src/mwply/mwsfdrsc.c",
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/mwply/mwsfdsbt.o",
                source="libmwsfdg/crimw/dev/sofdec/src/mwply/mwsfdsbt.c",
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfx/sfx_inf.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfx/sfx_inf.c",
            ),
            Object(
                NonMatching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfx/sfx_set.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfx/sfx_set.c",
            ),
            Object(
                NonMatching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sud/sud_lib.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sud/sud_lib.c",
                extra_cflags=["-sdata", "0", "-use_lmw_stmw", "on"],
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/mwply/mwsfdsee.o",
                source="libmwsfdg/crimw/dev/sofdec/src/mwply/mwsfdsee.c",
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfdcore/dct/dct_ver.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfdcore/dct/dct_ver.c",
                extra_cflags=["-sdata 0"],
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfdcore/memcpy/mcp_not.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfdcore/memcpy/mcp_not.c",
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfdcore/mpv/mpv_m2v.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfdcore/mpv/mpv_m2v.c",
            ),
            Object(
                NonMatching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfdcore/uty/muldiv.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfdcore/uty/muldiv.c",
            ),
            Object(
                Matching,
                "libmwsfdg.a//crimw/dev/sofdec/src/sfdcore/uty/memsetd.o",
                source="libmwsfdg/crimw/dev/sofdec/src/sfdcore/uty/memsetd.c",
                extra_cflags=["-O2,p"],
            ),
        ],
    },
    {
        "lib": "mwMovie",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "sdk",
        "objects": [
            Object(
                NonMatching,
                "mwMovie_release.a/mk6/mwMovie/build/gc/mwMovie_Data/release/mwMovie.o",
                source="movie/mwMovie.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
        ],
    },
    {
        "lib": "libmwfile",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "sdk",
        "objects": [
            Object(
                NonMatching,
                "libmwfile.a/mk6/mwFile/build/gcn/mwfile_gcn_Data/GAMECUBE_HW2_Rel/mwFile.o",
                source="mw/mwFile.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
        ],
    },
    {
        "lib": "libmsl",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "msl",
        "objects": [
            Object(
                NonMatching,
                "libmsl.a/listpool.o",
                source="libmsl/listpool.c",
            ),
            Object(
                Matching,
                "libmsl.a/mslstub.o",
                source="libmsl/mslstub.c",
            ),
            Object(
                NonMatching,
                "libmsl.a/mslsupport.o",
                source="libmsl/mslsupport.cpp",
            ),
            Object(
                Matching,
                "libmsl.a/mslmem.o",
                source="libmsl/mslmem.cpp",
            ),
            Object(
                NonMatching,
                "libmsl.a/mslcore.o",
                source="libmsl/mslcore.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                Matching,
                "libmsl.a/mslqueue.o",
                source="libmsl/mslqueue.c",
            ),
            Object(
                NonMatching,
                "libmsl.a/mslBank.o",
                source="libmsl/mslBank.cpp",
                extra_cflags=[
                    "-use_lmw_stmw on", "-str", "reuse,pool,readonly"
                ],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslBankLoadAsyncQueue.o",
                source="libmsl/mslBankLoadAsyncQueue.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslSound.o",
                source="libmsl/mslSound.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslWave.o",
                source="libmsl/mslWave.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslgcn.o",
                source="libmsl/mslgcn.cpp",
                extra_cflags=["-use_lmw_stmw on",
                              "-str reuse,pool,readonly"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslSoundBuffer.o",
                source="libmsl/mslSoundBuffer.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslStreamFile.o",
                source="libmsl/mslStreamFile.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslGCN_ARamBlock.o",
                source="libmsl/mslGCN_ARamBlock.cpp",
                extra_cflags=[
                    "-use_lmw_stmw on", "-inline", "deferred,level=4",
                    "-str", "reuse,pool,readonly"
                ],
            ),
            Object(
                NonMatching,
                "libmsl.a/mslStreamCache.o",
                source="libmsl/mslStreamCache.cpp",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/CriticalSection.o",
                source="libmsl/CriticalSection.c",
                extra_cflags=["-use_lmw_stmw on", "-str", "reuse,pool,readonly"],
            ),
            Object(
                NonMatching,
                "libmsl.a/RedBlackTree.o",
                source="libmsl/RedBlackTree.c",
                extra_cflags=["-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "libmsl.a/ExtHeapMgr.o",
                source="libmsl/ExtHeapMgr.c",
                extra_cflags=[
                    "-use_lmw_stmw on", "-opt", "nocse", "-inline", "all"
                ],
            ),
        ],
    },
    # Midway mwScreenEngine (NOT Criterion RW). B15: smallest Screen* C++ TUs.
    # Split names must match config/GQNE5D/splits.txt exactly. -lang=c++ auto for .cpp.
    {
        "lib": "mwScreenEngine",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "game",
        "objects": [
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenEvent.o",
                source="mwScreenEngine/ScreenEvent.cpp",
                extra_cflags=["-O4,s"],
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAnimKey.o",
                source="mwScreenEngine/ScreenAnimKey.cpp",
                extra_cflags=["-O4,s"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAnimControl.o",
                source="mwScreenEngine/ScreenAnimControl.cpp",
                # -inline off: keep bl GetTime/GetEase*/GetValue on keys.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAnimEffect.o",
                source="mwScreenEngine/ScreenAnimEffect.cpp",
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAnimScene.o",
                source="mwScreenEngine/ScreenAnimScene.cpp",
                # -inline off: keep bl GetDirection/Process/GetMaxTime.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAnimAction.o",
                source="mwScreenEngine/ScreenAnimAction.cpp",
                # -inline off: keep bl _GetAnimAction.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenMatrixStack.o",
                source="mwScreenEngine/ScreenMatrixStack.cpp",
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenClient.o",
                source="mwScreenEngine/ScreenClient.cpp",
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenParams.o",
                source="mwScreenEngine/ScreenParams.cpp",
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenResourceLibrary.o",
                source="mwScreenEngine/ScreenResourceLibrary.cpp",
            ),
            # B16 P1: ScreenMgr wave for boot / MAIN_MENU.
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenUtil.o",
                source="mwScreenEngine/ScreenUtil.cpp",
                # -O4,s: prefer mtctr on ReadHexInt digit loop (still soft-ceiling).
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/Screen.o",
                source="mwScreenEngine/Screen.cpp",
                # -inline off: keep GetRoot as bl in FireEvent (else inlined -> ~37%).
                # -O4,s: prefer mtctr dword-pair copy of RenderAll @120 init.
                extra_cflags=[
                    "-O4,s",
                    "-use_lmw_stmw on",
                    "-inline off",
                    "-sdata 0",
                    "-str pool,readonly",
                ],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenSet.o",
                source="mwScreenEngine/ScreenSet.cpp",
                # -inline off: GetChild(char*) must bl GetChild(int); GetScreen stmw.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenMgr.o",
                source="mwScreenEngine/ScreenMgr.cpp",
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
            ),
            # Wave A: action stack (CreateAction via ScreenUtil).
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenActionStack.o",
                source="mwScreenEngine/ScreenActionStack.cpp",
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
            ),
            # B19 / mode-select Wave B chrome deps: base node + action.
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenNode.o",
                source="mwScreenEngine/ScreenNode.cpp",
                extra_cflags=["-sdata 0", "-sdata2 0", "-str reuse,pool,readonly"],
            ),
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenObject.o",
                source="mwScreenEngine/ScreenObject.cpp",
                # -inline off: keep GetFocus/FireEvent/ProcessEvent as bl (MWCC otherwise
                # inlines them into FireEvent/BroadcastEvent/SetComponent -> 0% / 3-5x size).
                # -O4,s: mtctr/bdnz + stmw for ctor/HasEvent/SetLast/SetComponent loops.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenAction.o",
                source="mwScreenEngine/ScreenAction.cpp",
                extra_cflags=[
                    "-O4,s",
                    "-use_lmw_stmw on",
                    "-sdata 0",
                    "-sdata2 0",
                    "-str reuse,pool,readonly",
                ],
            ),
            # B18d Wave C / B19: Open/Close/Insert/Replace/Exit/Transition.
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenScreenAction.o",
                source="mwScreenEngine/ScreenScreenAction.cpp",
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
                # Link ceiling: duplicate ScreenBaseScreenAction weak dtor/vtable
                # changes 42 DOL bytes despite aggregate 100% objdiff.
            ),
            # B18d Wave C / B19: SetFocus, UserConfirm, Visible, Enable, Else/Question.
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenMiscAction.o",
                source="mwScreenEngine/ScreenMiscAction.cpp",
                # -inline off: keep bl ScreenIntegerCompare from Question Update.
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            # Wave A: RegisterGameVariables (+ dispatcher Register).
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/GameVariables.o",
                source="mwScreenEngine/GameVariables.cpp",
                # -O4,s: prefer stmw/lmw on walker NVs (GetInt/GetIntArray/HandleAction).
                extra_cflags=["-O4,s", "-use_lmw_stmw on"],
            ),
            Object(
                Matching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenControl.o",
                source="mwScreenEngine/ScreenControl.cpp",
                # -inline off: keep recursive _RefreshData as bl (else MWCC inlines).
                extra_cflags=["-O4,s", "-use_lmw_stmw on", "-inline off"],
            ),
            # Wave A: CloseScreen / CloseObject for wait_for_screen_close.
            Object(
                NonMatching,
                "mwScreenEngineGCrelease.a/mk6/mwScreenEngine/mwScreenEngineGC_Data/release/ScreenInstancer.o",
                source="mwScreenEngine/ScreenInstancer.cpp",
                extra_cflags=[
                    "-O4,s",
                    "-use_lmw_stmw on",
                    "-inline off",
                    "-str reuse,pool,readonly",
                ],
            ),
        ],
    },
    # Midway libmkparticle (NOT Criterion RW). B12: 2D/font path for boot->PRESS START.
    # Split names must match config/GQNE5D/splits.txt exactly. Defer VM/emitter/gc_render.
    {
        "lib": "libmkparticle",
        "mw_version": game_mw_version,
        "cflags": cflags_base,
        "progress_category": "game",
        "objects": [
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/color.o",
                source="libmkparticle/color.c",
                extra_cflags=["-schedule off"],
            ),
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/supported.o",
                source="libmkparticle/supported.c",
                extra_cflags=["-opt nopeephole"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/pfxmath.o",
                source="libmkparticle/pfxmath.c",
                extra_cflags=["-O4,s"],
            ),
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/table.o",
                source="libmkparticle/table.c",
            ),
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/shader.o",
                source="libmkparticle/shader.c",
                extra_cflags=["-schedule off"],
            ),
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/init.o",
                source="libmkparticle/init.c",
                extra_cflags=["-O4,s", "-inline off", "-schedule off"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/texture_anim.o",
                source="libmkparticle/texture_anim.c",
                extra_cflags=["-O4,s", "-schedule off"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/geometry.o",
                source="libmkparticle/geometry.c",
                extra_cflags=["-O4,s", "-schedule off", "-fp_contract off", "-opt nopeephole"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/CompileFields.o",
                source="libmkparticle/CompileFields.c",
                extra_cflags=["-O4,s", "-schedule off", "-opt nopeephole"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/streams.o",
                source="libmkparticle/streams.c",
                extra_cflags=["-O4,s", "-inline off", "-schedule off"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/texture_bridge.o",
                source="libmkparticle/texture_bridge.c",
                extra_cflags=["-O4,s", "-inline off", "-schedule off", "-opt nopeephole"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/metrics.o",
                source="libmkparticle/metrics.c",
                # Retail pfxmetrics_estimate_size requires peephole optimization off.
                extra_cflags=["-O4,s", "-inline off", "-schedule off", "-opt nopeephole"],
            ),
            Object(
                Matching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/gc_state.o",
                source="libmkparticle/gc_state.c",
                # -inline off: retail bl apply_single_texture from alphamap (no inline).
                # -use_lmw_stmw + scheduling off: xoris i2f / thin GX wrappers.
                extra_cflags=["-use_lmw_stmw on", "-inline off"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/gc_2d.o",
                source="libmkparticle/gc_2d.c",
                # -schedule off: keep source-order i2f/fctiwz closer to retail.
                # -fp_contract off: retail uses fmuls+fadds (not fmadds) in geometry.
                extra_cflags=["-use_lmw_stmw on", "-schedule off", "-fp_contract off"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/gc_font.o",
                source="libmkparticle/gc_font.c",
                # No -use_lmw_stmw: retail uses _savegpr_29 in nativefont_string_render.
                # -schedule off: Y-then-X fctiwz / UV load order closer to retail.
                # Retail nativefont_instance_unlock requires peephole optimization off.
                extra_cflags=["-O4,s", "-inline off", "-schedule off", "-opt nopeephole"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/pfx2d.o",
                source="libmkparticle/pfx2d.c",
                # No -use_lmw_stmw: retail uses _savegpr_25/_restgpr_25 in end_render.
                extra_cflags=["-O4,s"],
            ),
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/pfxfont.o",
                source="libmkparticle/pfxfont.c",
                # No -use_lmw_stmw: retail uses _savegpr_19/_restgpr_19 in string_set.
                # Scheduling and peephole settings are uniform; no-inline remains local.
                extra_cflags=["-O4,s", "-schedule off", "-opt nopeephole"],
            ),
            # Thin pfxsystem_* frame helpers; VM/emitter stubs (whole-TU NonMatching).
            Object(
                NonMatching,
                "libmkparticle_release.a/mk6/particles/build/gc/mkparticle_gc_Data/release/particle.o",
                source="libmkparticle/particle.c",
                extra_cflags=["-use_lmw_stmw on", "-O4,s", "-schedule off"],
            ),
        ],
    },
    DolphinLib(
        "os",
        [
            Object(NonMatching, "os.a/__start.o", source="dolphin/__start.c"),
            Object(
                NonMatching,
                "os.a/__ppc_eabi_init.o",
                source="dolphin/os/__ppc_eabi_init.cpp",
                extra_cflags=["-lang=c"],
            ),
        ],
    ),
    DolphinLib(
        "sp",
        [
            Object(Matching, "sp.a/sp.o", source="dolphin/sp.c"),
        ],
    ),
    RenderWareLib(
        "rwcore",
        [
            Object(Matching, "rwcore.a/bacolor.obj", source="rw/bacolor.c"),
            Object(NonMatching, "rwcore.a/babinfrm.obj", source="rw/babinfrm.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/babintex.obj", source="rw/babintex.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/bacamera.obj", source="rw/bacamera.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off",
                                 "-fp_contract", "on"]),
            Object(NonMatching, "rwcore.a/dl2drend.obj", source="rw/dl2drend.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dlsprite.obj", source="rw/dlsprite.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/nodeDolphinSubmitNoLight.obj",
                   source="rw/nodeDolphinSubmitNoLight.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dltoken.obj", source="rw/dltoken.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dlbrkpt.obj", source="rw/dlbrkpt.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dldevice.obj", source="rw/dldevice.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dlraster.obj", source="rw/dlraster.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dlconvrt.obj", source="rw/dlconvrt.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dlrendst.obj", source="rw/dlrendst.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dltextur.obj", source="rw/dltextur.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/dltexdic.obj", source="rw/dltexdic.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/baim3d.obj", source="rw/baim3d.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2altmdl.obj", source="rw/p2altmdl.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2renderstate.obj",
                   source="rw/p2renderstate.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2resort.obj", source="rw/p2resort.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/im3dpipe.obj", source="rw/im3dpipe.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/bapipe.obj", source="rw/bapipe.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/batypehf.obj", source="rw/batypehf.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/bafsys.obj", source="rw/bafsys.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/bamatrix.obj", source="rw/bamatrix.c",
                   extra_cflags=["-inline", "off", "-schedule", "off"]),
            Object(NonMatching, "rwcore.a/basync.obj", source="rw/basync.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/bavector.obj", source="rw/bavector.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/baimras.obj", source="rw/baimras.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/baimage.obj", source="rw/baimage.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/palquant.obj", source="rw/palquant.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(Matching, "rwcore.a/baerr.obj", source="rw/baerr.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(Matching, "rwcore.a/rwgrp.obj", source="rw/rwgrp.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/resmem.obj", source="rw/resmem.c",
                   extra_cflags=["-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/baresour.obj", source="rw/baresour.c",
                   extra_cflags=["-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/baresamp.obj", source="rw/baresamp.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(
                NonMatching,
                "rwcore.a/baraster.obj",
                source="rw/baraster.c",
                extra_cflags=["-opt", "off", "-O0"],
            ),
            Object(
                NonMatching,
                "rwcore.a/batextur.obj",
                source="rw/batextur.c",
                extra_cflags=["-opt", "off", "-O0"],
            ),
            # -opt off clears inherited -O4,p from cflags_base before per-TU level
            Object(Matching, "rwcore.a/osintf.obj", source="rw/osintf.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(Matching, "rwcore.a/babbox.obj", source="rw/babbox.c",
                   extra_cflags=["-opt", "off", "-O1,p"]),
            Object(NonMatching, "rwcore.a/badevice.obj", source="rw/badevice.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/baframe.obj", source="rw/baframe.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/batkbin.obj", source="rw/batkbin.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/batkreg.obj", source="rw/batkreg.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rwcore.a/rwstring.obj", source="rw/rwstring.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/babinary.obj", source="rw/babinary.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/bamemory.obj", source="rw/bamemory.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/bastream.obj", source="rw/bastream.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2dep.obj", source="rw/p2dep.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2define.obj", source="rw/p2define.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2heap.obj", source="rw/p2heap.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rwcore.a/p2core.obj", source="rw/p2core.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rpmatfx",
        [
            Object(NonMatching, "rpmatfx.a/rpmatfx.obj", source="rw/rpmatfx.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpmatfx.a/effectPipesGcn.obj",
                   source="rw/effectPipesGcn.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpmatfx.a/multiTex.obj", source="rw/multiTex.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpmatfx.a/multiTexEffect.obj",
                   source="rw/multiTexEffect.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpmatfx.a/multiTexGcnData.obj",
                   source="rw/multiTexGcnData.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpmatfx.a/multiTexGcnPipe.obj",
                   source="rw/multiTexGcnPipe.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpmatfx.a/multiTexGcn.obj",
                   source="rw/multiTexGcn.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpmatfx.a/nbtGen.obj", source="rw/nbtGen.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rpworld",
        [
            Object(NonMatching, "rpworld.a/babinwor.obj", source="rw/babinwor.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpworld.a/baclump.obj", source="rw/baclump.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/bageomet.obj", source="rw/bageomet.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/bamesh.obj", source="rw/bamesh.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/bameshop.obj", source="rw/bameshop.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/basector.obj", source="rw/basector.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpworld.a/bapipew.obj", source="rw/bapipew.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/alphapass.obj", source="rw/alphapass.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpworld.a/baworld.obj", source="rw/baworld.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/baworobj.obj", source="rw/baworobj.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/wrldpipe.obj", source="rw/wrldpipe.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpworld.a/nodeGameCubeWorldSectorAllInOne.obj",
                   source="rw/nodeGameCubeWorldSectorAllInOne.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/nodeGameCubeAtomicAllInOne.obj",
                   source="rw/nodeGameCubeAtomicAllInOne.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/gcpipe.obj", source="rw/gcpipe.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/setup.obj", source="rw/setup.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/instanceworld.obj",
                   source="rw/instanceworld.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/gclights.obj", source="rw/gclights.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/gcmorph.obj", source="rw/gcmorph.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/ibuffer.obj", source="rw/ibuffer.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/vtxfmt.obj", source="rw/vtxfmt.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/geomcond.obj", source="rw/geomcond.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/instancegeom.obj", source="rw/instancegeom.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/geominst.obj", source="rw/geominst.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/itools.obj", source="rw/itools.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/vtools.obj", source="rw/vtools.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/native.obj", source="rw/native.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/vtxdesc.obj", source="rw/vtxdesc.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/vbuffer.obj", source="rw/vbuffer.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/bamatlst.obj", source="rw/bamatlst.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/bamateri.obj", source="rw/bamateri.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpworld.a/balight.obj", source="rw/balight.c",
                   extra_cflags=["-opt", "off", "-O0"]),
        ],
    ),
    RenderWareLib(
        "rpskin",
        [
            Object(NonMatching, "rpskin.a/skingcng.obj", source="rw/skingcng.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/instanceskin.obj",
                   source="rw/instanceskin.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/rpskin.obj", source="rw/rpskin.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/bsplit.obj", source="rw/bsplit.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/skinplatform.obj", source="rw/skinplatform.c",
                   extra_cflags=["-opt", "off", "-O0"]),
            Object(NonMatching, "rpskin.a/skinstream.obj", source="rw/skinstream.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/skinmatrixblend.obj", source="rw/skinmatrixblend.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/skingcnasm.obj", source="rw/skingcnasm.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rpskin.a/skingcn.obj", source="rw/skingcn.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rpspecular",
        [
            Object(NonMatching, "rpspecular.a/rpspecular.obj",
                   source="rw/rpspecular.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rphanim",
        [
            Object(NonMatching, "rphanim.a/stdkey.obj", source="rw/stdkey.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
            Object(NonMatching, "rphanim.a/rphanim.obj", source="rw/rphanim.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rtanim",
        [
            Object(NonMatching, "rtanim.a/rtanim.obj", source="rw/rtanim.c",
                   extra_cflags=["-O4,s", "-opt", "off", "-inline", "off"]),
        ],
    ),
    RenderWareLib(
        "rtquat",
        [
            Object(NonMatching, "rtquat.a/rtquat.obj", source="rw/rtquat.c",
                   extra_cflags=["-opt", "off", "-O0", "-inline", "off"]),
        ],
    ),
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
    ProgressCategory("renderware", "RenderWare"),
    ProgressCategory("sofdec", "Sofdec"),
    ProgressCategory("msl", "Midway Sound Library"),
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
