###
# PS2 (SLUS_208.81) build generator.
#
# The PS2 ELF has one merged PROGBITS section and no symbols, so the DTK
# pipeline in project.py cannot split or link it. This generator writes a
# build.ninja and objdiff.json that:
#   - split the units listed in config/<version>/splits.txt into assembly with
#     tools/ps2_split.py and assemble them into target objects;
#   - compile the shared source of the same units with mwccps2;
#   - generate an objdiff report for progress.
# Nothing is linked, so there is no SHA-1 check of a rebuilt ELF; the retail
# ELF itself is verified before splitting.
###

import importlib.util
import json
import os
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional

from . import ninja_syntax
from .project import CHAIN, EXE, is_windows, make_flags_str


@dataclass
class Ps2Object:
    name: str  # Unit object name in splits.txt, e.g. "mwMem.o"
    source: str  # Path under src/, e.g. "mw/mwMem.c"
    extra_cflags: List[str] = field(default_factory=list)
    progress_category: Optional[str] = None


@dataclass
class Ps2ProjectConfig:
    version: str = ""
    build_dir: Path = Path("build")
    tools_dir: Path = Path("tools")
    src_dir: Path = Path("src")
    elf_sha1: str = ""
    compiler: str = ""  # decomp.me compiler id, e.g. "mwcps2-3.0.1-020123"
    cflags: List[str] = field(default_factory=list)
    asflags: List[str] = field(default_factory=list)
    objects: List[Ps2Object] = field(default_factory=list)
    progress_categories: List[Any] = field(default_factory=list)
    # Tool paths; None means download the pinned tag into build/.
    binutils_path: Optional[Path] = None
    binutils_tag: str = ""
    compilers_path: Optional[Path] = None
    objdiff_path: Optional[Path] = None
    objdiff_tag: str = ""
    wrapper: Optional[Path] = None
    wibo_tag: str = ""

    def out_path(self) -> Path:
        return self.build_dir / self.version

    def config_dir(self) -> Path:
        return Path("config") / self.version

    def elf_path(self) -> Path:
        return Path("orig") / self.version / self.version

    def target_obj_path(self, obj: Ps2Object) -> Path:
        return self.out_path() / "obj" / obj.name

    def base_obj_path(self, obj: Ps2Object) -> Path:
        return self.out_path() / "src" / obj.name

    def asm_path(self, obj: Ps2Object) -> Path:
        return (self.out_path() / "asm" / obj.name).with_suffix(".s")

    def report_path(self) -> Path:
        return self.out_path() / "report.json"


def _download(n: ninja_syntax.Writer, output: Path, tool: str, tag: str, script: Path) -> None:
    n.build(
        outputs=output,
        rule="download_tool",
        implicit=script,
        variables={"tool": tool, "tag": tag},
    )


def generate_build(config: Ps2ProjectConfig) -> None:
    if importlib.util.find_spec("spimdisasm") is None:
        print(
            "Warning: spimdisasm is not installed for this Python; the PS2 split "
            "step will fail.\n"
            "    python3 -m pip install -r tools/requirements-ps2.txt"
        )
    for obj in config.objects:
        if Path(obj.name).stem != Path(obj.source).stem:
            sys.exit(f"{obj.name}: object name must match the stem of {obj.source}")
    generate_build_ninja(config)
    generate_objdiff_config(config)


def generate_build_ninja(config: Ps2ProjectConfig) -> None:
    out = open("build.ninja", "w", encoding="utf-8")
    n = ninja_syntax.Writer(out)
    n.variable("ninja_required_version", "1.3")
    n.newline()

    configure_script = Path(os.path.relpath(os.path.abspath(sys.argv[0])))
    python_lib = Path(os.path.relpath(__file__))
    n.comment("The arguments passed to configure.py, for rerunning it.")
    n.variable("configure_args", sys.argv[1:])
    n.variable("python", f'"{sys.executable}"')
    n.newline()

    ###
    # Tooling
    ###
    n.comment("Tooling")
    build_tools = config.build_dir / "tools"
    download_tool = config.tools_dir / "download_tool.py"
    n.rule(
        name="download_tool",
        command=f"$python {download_tool} $tool $out --tag $tag",
        description="TOOL $out",
    )

    if config.objdiff_path is not None:
        objdiff = config.objdiff_path
    else:
        objdiff = build_tools / f"objdiff-cli{EXE}"
        _download(n, objdiff, "objdiff-cli", config.objdiff_tag, download_tool)

    if config.binutils_path is not None:
        binutils = config.binutils_path
        binutils_implicit = None
    else:
        binutils = config.build_dir / "ps2_binutils"
        binutils_implicit = binutils
        _download(n, binutils, "ps2_binutils", config.binutils_tag, download_tool)
    gnu_as = binutils / f"mips-ps2-decompals-as{EXE}"

    if config.compilers_path is not None:
        compiler_dir = config.compilers_path / "PS2" / config.compiler
        compiler_implicit = None
    else:
        compiler_dir = config.build_dir / "ps2_compilers" / "PS2" / config.compiler
        compiler_implicit = compiler_dir
        _download(n, compiler_dir, "ps2_compiler", config.compiler, download_tool)
    mwcc = compiler_dir / "mwccps2.exe"

    wrapper_cmd = ""
    wrapper_implicit = None
    if not is_windows():
        wrapper = config.wrapper
        if wrapper is None:
            wrapper = build_tools / "wibo"
            wrapper_implicit = wrapper
            _download(n, wrapper, "wibo", config.wibo_tag, download_tool)
        wrapper_cmd = f"{wrapper} "
    n.newline()

    ###
    # Split the retail ELF
    ###
    n.comment("Split retail units into assembly")
    asm_dir = config.out_path() / "asm"
    split_script = config.tools_dir / "ps2_split.py"
    elf = config.elf_path()
    splits = config.config_dir() / "splits.txt"
    symbols = config.config_dir() / "symbols.txt"
    n.rule(
        name="ps2_split",
        command=(
            f"$python {split_script} --elf {elf} --sha1 {config.elf_sha1} "
            f"--splits {splits} --symbols {symbols} --out {asm_dir}"
        ),
        description="SPLIT $elf_name",
    )
    macro_inc = asm_dir / "macro.inc"
    n.build(
        outputs=[macro_inc, *(config.asm_path(obj) for obj in config.objects)],
        rule="ps2_split",
        implicit=[elf, splits, symbols, split_script],
        variables={"elf_name": elf.name},
    )
    n.newline()

    n.comment("Assemble target objects")
    n.rule(
        name="ps2_as",
        command=f"{CHAIN}{gnu_as} $asflags -I {asm_dir} -o $out $in",
        description="AS $out",
    )
    for obj in config.objects:
        n.build(
            outputs=config.target_obj_path(obj),
            rule="ps2_as",
            inputs=config.asm_path(obj),
            implicit=[binutils_implicit or gnu_as, macro_inc],
            variables={"asflags": make_flags_str(config.asflags)},
        )
    n.build(
        outputs="all_target",
        rule="phony",
        inputs=[config.target_obj_path(obj) for obj in config.objects],
    )
    n.newline()

    ###
    # Compile shared source
    ###
    n.comment("Compile source with mwccps2")
    mwcc_cmd = f"{wrapper_cmd}{mwcc} $cflags -MMD -c $in -o $basedir"
    mwcc_implicit: List[Any] = [compiler_implicit or mwcc]
    if wrapper_implicit is not None:
        mwcc_implicit.append(wrapper_implicit)
    if os.name != "nt":
        transform_dep = config.tools_dir / "transform_dep.py"
        mwcc_cmd += f" && $python {transform_dep} $basefile.d $basefile.d"
        mwcc_implicit.append(transform_dep)
    n.rule(
        name="mwcc",
        command=mwcc_cmd,
        description="MWCC $out",
        depfile="$basefile.d",
        deps="gcc",
    )
    for obj in config.objects:
        out_path = config.base_obj_path(obj)
        cflags = [*config.cflags, *obj.extra_cflags]
        n.build(
            outputs=out_path,
            rule="mwcc",
            inputs=config.src_dir / obj.source,
            implicit=mwcc_implicit,
            variables={
                "cflags": make_flags_str(cflags),
                "basedir": os.path.dirname(out_path),
                "basefile": out_path.with_suffix(""),
            },
        )
    n.build(
        outputs="all_source",
        rule="phony",
        inputs=[config.base_obj_path(obj) for obj in config.objects],
    )
    n.newline()

    ###
    # Progress
    ###
    n.comment("Generate progress report")
    report_path = config.report_path()
    n.rule(
        name="report",
        command=f"{objdiff} report generate -o $out",
        description="REPORT",
    )
    n.build(
        outputs=report_path,
        rule="report",
        implicit=[objdiff, "objdiff.json", "all_source", "all_target"],
    )
    n.rule(
        name="progress",
        command=f"$python {configure_script} $configure_args progress",
        description="PROGRESS",
    )
    n.build(
        outputs="progress",
        rule="progress",
        implicit=[report_path, configure_script, python_lib],
    )
    n.newline()

    n.default(["all_source", "all_target"])
    n.newline()

    ###
    # Regenerate on change
    ###
    n.comment("Reconfigure on change")
    n.rule(
        name="configure",
        command=f"$python {configure_script} $configure_args",
        generator=True,
        description=f"RUN {configure_script}",
    )
    n.build(
        outputs=["build.ninja", "objdiff.json"],
        rule="configure",
        implicit=[
            configure_script,
            python_lib,
            config.tools_dir / "ninja_syntax.py",
        ],
    )
    out.close()


def generate_objdiff_config(config: Ps2ProjectConfig) -> None:
    units: List[Dict[str, Any]] = []
    for obj in config.objects:
        metadata: Dict[str, Any] = {
            "complete": False,
            "source_path": str(config.src_dir / obj.source),
            "auto_generated": False,
        }
        if obj.progress_category is not None:
            metadata["progress_categories"] = [obj.progress_category]
        units.append(
            {
                "name": f"main/{Path(obj.name).with_suffix('')}",
                "target_path": config.target_obj_path(obj).as_posix(),
                "base_path": config.base_obj_path(obj).as_posix(),
                "metadata": metadata,
            }
        )
    objdiff_config: Dict[str, Any] = {
        "min_version": "2.0.0-beta.5",
        "custom_make": "ninja",
        "build_target": False,
        "watch_patterns": ["*.c", "*.cpp", "*.h", "*.hpp", "*.inc", "*.py", "*.txt"],
        "units": units,
        "progress_categories": [
            {"id": c.id, "name": c.name} for c in config.progress_categories
        ],
    }
    with open("objdiff.json", "w", encoding="utf-8") as w:
        json.dump(objdiff_config, w, indent=2)


def calculate_progress(config: Ps2ProjectConfig) -> None:
    report_path = config.report_path()
    if not report_path.is_file():
        sys.exit(f"Report {report_path} not found; run `ninja {report_path}` first")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    measures = report.get("measures", {})

    def pct(key: str) -> float:
        return float(measures.get(key, 0) or 0)

    print(f"Progress for {config.version} (no link; split units only):")
    print(
        f"  Code: {pct('matched_code_percent'):.2f}% matched "
        f"({measures.get('matched_functions', 0)} / "
        f"{measures.get('total_functions', 0)} functions), "
        f"fuzzy {pct('fuzzy_match_percent'):.2f}%"
    )
    print(f"  Data: {pct('matched_data_percent'):.2f}% matched")
