#!/usr/bin/env python3
"""Bootstrap the MKD matching environment using only the Python standard library."""

from __future__ import annotations

import argparse
import hashlib
import os
import platform
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Sequence


ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
VERSION = "GQNE5D"
OBJECT_BASE = ROOT / "orig" / VERSION
CONFIG = ROOT / "config" / VERSION / "config.yml"
# Full raw GQNE5D image metadata: https://www.gametdb.com/Wii/GQNE5D
EXPECTED_ISO_SIZE = 1_459_978_240
EXPECTED_ISO_SHA1 = "489c6b57b70390933dff7d8d9d12424f58a8f821"

M2C_REPOSITORY = "https://github.com/matt-kempster/m2c.git"
M2C_ARCHIVES = (
    "https://github.com/matt-kempster/m2c/archive/refs/heads/main.zip",
    "https://github.com/matt-kempster/m2c/archive/refs/heads/master.zip",
)


def executable_name(name: str) -> str:
    return name + (".exe" if os.name == "nt" else "")


TOOL_OUTPUTS = (
    ("compilers", BUILD / "compilers", "compilers_tag"),
    ("dtk", BUILD / "tools" / executable_name("dtk"), "dtk_tag"),
    ("objdiff-cli", BUILD / "tools" / executable_name("objdiff-cli"), "objdiff_tag"),
    ("sjiswrap", BUILD / "tools" / "sjiswrap.exe", "sjiswrap_tag"),
    ("binutils", BUILD / "binutils", "binutils_tag"),
)


@dataclass
class Check:
    state: str
    label: str
    detail: str


class Reporter:
    def __init__(self) -> None:
        self.checks: list[Check] = []

    def _add(self, state: str, label: str, detail: str) -> None:
        self.checks.append(Check(state, label, detail))
        icon = {"PASS": "OK", "WARN": "!!", "FAIL": "XX", "SKIP": "--"}[state]
        print(f"[{icon}] {label}: {detail}")

    def pass_(self, label: str, detail: str) -> None:
        self._add("PASS", label, detail)

    def warn(self, label: str, detail: str) -> None:
        self._add("WARN", label, detail)

    def fail(self, label: str, detail: str) -> None:
        self._add("FAIL", label, detail)

    def skip(self, label: str, detail: str) -> None:
        self._add("SKIP", label, detail)

    def summary(self) -> int:
        print("\nSetup checklist")
        print("===============")
        marker = {"PASS": "x", "WARN": "!", "FAIL": " ", "SKIP": "-"}
        for check in self.checks:
            print(f"[{marker[check.state]}] {check.label} — {check.detail}")

        failures = sum(check.state == "FAIL" for check in self.checks)
        warnings = sum(check.state == "WARN" for check in self.checks)
        if failures:
            print(f"\nNOT READY: {failures} required check(s) failed; {warnings} warning(s).")
            return 1
        if warnings:
            print(f"\nREADY WITH WARNINGS: no required checks failed; {warnings} warning(s).")
            return 0
        print("\nREADY: retail input, matching tools, configuration, and build checks passed.")
        return 0


def run(
    command: Sequence[str],
    reporter: Reporter,
    label: str,
    *,
    required: bool = True,
    cwd: Path = ROOT,
) -> bool:
    printable = " ".join(str(part) for part in command)
    print(f"\n> {printable}")
    try:
        completed = subprocess.run(command, cwd=cwd, check=False)
    except OSError as error:
        message = str(error)
        (reporter.fail if required else reporter.warn)(label, message)
        return False
    if completed.returncode == 0:
        reporter.pass_(label, "completed")
        return True
    message = f"command exited with status {completed.returncode}"
    (reporter.fail if required else reporter.warn)(label, message)
    return False


def sha1_file(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def expected_dol_sha1() -> str:
    text = CONFIG.read_text(encoding="utf-8")
    match = re.search(r"^hash:\s*([0-9a-fA-F]{40})\s*$", text, re.MULTILINE)
    if match is None:
        raise ValueError(f"no SHA-1 hash found in {CONFIG.relative_to(ROOT)}")
    return match.group(1).lower()


def dol_size(header: bytes) -> int:
    if len(header) < 0x100:
        raise ValueError("truncated DOL header")
    text_offsets = struct.unpack_from(">7I", header, 0x00)
    data_offsets = struct.unpack_from(">11I", header, 0x1C)
    text_sizes = struct.unpack_from(">7I", header, 0x90)
    data_sizes = struct.unpack_from(">11I", header, 0xAC)
    ends = [offset + size for offset, size in zip(text_offsets, text_sizes) if size]
    ends.extend(offset + size for offset, size in zip(data_offsets, data_sizes) if size)
    if not ends:
        raise ValueError("DOL header has no loadable sections")
    return max(ends)


def embedded_dol_sha1(iso: Path) -> str:
    with iso.open("rb") as stream:
        stream.seek(0x420)
        raw_offset = stream.read(4)
        if len(raw_offset) != 4:
            raise ValueError("image is too small for a GameCube disc header")
        offset = struct.unpack(">I", raw_offset)[0]
        stream.seek(offset)
        header = stream.read(0x100)
        size = dol_size(header)
        stream.seek(offset)
        digest = hashlib.sha1()
        remaining = size
        while remaining:
            chunk = stream.read(min(1024 * 1024, remaining))
            if not chunk:
                raise ValueError("image ended inside the embedded main.dol")
            digest.update(chunk)
            remaining -= len(chunk)
    return digest.hexdigest()


def validate_iso(path: Path, expected: str, reporter: Reporter) -> bool:
    if not path.is_file():
        reporter.fail("Retail ISO", f"file not found: {path}")
        return False
    try:
        with path.open("rb") as stream:
            game_id = stream.read(6).decode("ascii", errors="replace")
        image_sha1 = sha1_file(path)
        image_ok = image_sha1 == EXPECTED_ISO_SHA1
        if image_ok:
            reporter.pass_("ISO SHA-1", f"{image_sha1} ({path})")
        else:
            reporter.fail(
                "ISO SHA-1",
                f"expected {EXPECTED_ISO_SHA1}, found {image_sha1} ({path})",
            )
        size = path.stat().st_size
        if size == EXPECTED_ISO_SIZE:
            reporter.pass_("ISO size", f"{size} bytes")
        else:
            reporter.fail("ISO size", f"expected {EXPECTED_ISO_SIZE}, found {size} bytes")
            image_ok = False
        if game_id != VERSION:
            reporter.fail("ISO game ID", f"expected {VERSION}, found {game_id!r}")
            return False
        reporter.pass_("ISO game ID", game_id)
        actual = embedded_dol_sha1(path)
    except (OSError, ValueError) as error:
        reporter.fail("Retail ISO", f"cannot validate raw ISO/GCM: {error}")
        return False
    if actual != expected:
        reporter.fail("Retail main.dol SHA-1", f"expected {expected}, found {actual}")
        return False
    reporter.pass_("Retail main.dol SHA-1", actual)
    dtk_ok = True
    try:
        path.resolve().relative_to(OBJECT_BASE.resolve())
    except ValueError:
        extracted = OBJECT_BASE / "sys" / "main.dol"
        if extracted.is_file() and sha1_file(extracted) == expected:
            reporter.pass_(
                "DTK retail input",
                "validated external ISO; matching extracted disc tree is present",
            )
        else:
            reporter.fail(
                "DTK retail input",
                f"ISO is outside {OBJECT_BASE.relative_to(ROOT)} and no matching extracted tree exists",
            )
            dtk_ok = False
    else:
        reporter.pass_("DTK retail input", f"ISO is under {OBJECT_BASE.relative_to(ROOT)}")
    return image_ok and dtk_ok


def validate_extracted_dol(path: Path, expected: str, reporter: Reporter) -> bool:
    if not path.is_file():
        reporter.fail("Retail input", f"missing {path.relative_to(ROOT)} and no ISO was supplied")
        return False
    actual = sha1_file(path)
    if actual != expected:
        reporter.fail("Retail main.dol SHA-1", f"expected {expected}, found {actual}")
        return False
    reporter.pass_("Retail main.dol SHA-1", f"{actual} ({path.relative_to(ROOT)})")
    reporter.pass_("DTK retail input", "extracted disc tree is present")
    return True


def locate_iso(explicit: Optional[Path]) -> Optional[Path]:
    if explicit is not None:
        return explicit.expanduser().resolve()
    if OBJECT_BASE.is_dir():
        for suffix in ("*.iso", "*.gcm"):
            candidates = sorted(OBJECT_BASE.glob(suffix))
            if candidates:
                return candidates[0]
    return None


def validate_retail_input(explicit_iso: Optional[Path], reporter: Reporter) -> bool:
    try:
        expected = expected_dol_sha1()
    except (OSError, ValueError) as error:
        reporter.fail("Retail checksum configuration", str(error))
        return False
    reporter.pass_("Expected retail main.dol SHA-1", expected)
    iso = locate_iso(explicit_iso)
    if iso is not None:
        return validate_iso(iso, expected, reporter)
    return validate_extracted_dol(OBJECT_BASE / "sys" / "main.dol", expected, reporter)


def command_version(command: str) -> Optional[str]:
    path = shutil.which(command)
    if path is None:
        return None
    try:
        result = subprocess.run(
            [path, "--version"], capture_output=True, text=True, check=False
        )
    except OSError:
        return path
    output = (result.stdout or result.stderr).strip().splitlines()
    return f"{path} ({output[0]})" if output else path


def check_host_tools(reporter: Reporter) -> tuple[Optional[str], Optional[str]]:
    reporter.pass_("Python", f"{sys.executable} ({platform.python_version()})")
    ninja = shutil.which("ninja")
    ninja_version = command_version("ninja")
    if ninja_version:
        reporter.pass_("Ninja", ninja_version)
    else:
        reporter.warn("Ninja", "not installed; setup can continue, but the build will be skipped")
    git = shutil.which("git")
    git_version = command_version("git")
    if git_version:
        reporter.pass_("Git", git_version)
    else:
        reporter.warn("Git", "not installed; submodules and m2c updates will use fallbacks")
    return ninja, git


def init_submodules(git: Optional[str], reporter: Reporter) -> None:
    if not (ROOT / ".gitmodules").is_file():
        reporter.pass_("Git submodules", "repository has no .gitmodules entries")
        return
    if git is None:
        reporter.warn("Git submodules", "git is missing; could not initialize submodules")
        return
    run(
        [git, "submodule", "update", "--init", "--recursive"],
        reporter,
        "Git submodules",
        required=False,
    )


def safe_extract_zip(archive: Path, destination: Path) -> None:
    with zipfile.ZipFile(archive) as zipped:
        base = destination.resolve()
        for member in zipped.infolist():
            target = (destination / member.filename).resolve()
            if target != base and base not in target.parents:
                raise ValueError(f"unsafe archive member: {member.filename}")
        zipped.extractall(destination)


def download_m2c_archive(destination: Path) -> None:
    last_error: Optional[Exception] = None
    for url in M2C_ARCHIVES:
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "mkd-init"})
            with tempfile.TemporaryDirectory(dir=BUILD) as temp_name:
                temp = Path(temp_name)
                archive = temp / "m2c.zip"
                with urllib.request.urlopen(request) as response, archive.open("wb") as output:
                    shutil.copyfileobj(response, output)
                unpacked = temp / "unpacked"
                unpacked.mkdir()
                safe_extract_zip(archive, unpacked)
                roots = [entry for entry in unpacked.iterdir() if entry.is_dir()]
                if len(roots) != 1 or not (roots[0] / "m2c.py").is_file():
                    raise ValueError("m2c archive has an unexpected layout")
                shutil.move(str(roots[0]), destination)
            return
        except (OSError, ValueError, urllib.error.URLError) as error:
            last_error = error
    raise RuntimeError(f"unable to download m2c: {last_error}")


def ensure_m2c(git: Optional[str], reporter: Reporter) -> None:
    destination = BUILD / "m2c"
    script = destination / "m2c.py"
    if script.is_file():
        if git and (destination / ".git").is_dir():
            run(
                [git, "-C", str(destination), "pull", "--ff-only"],
                reporter,
                "m2c update",
                required=False,
            )
        else:
            reporter.pass_("m2c checkout", str(destination.relative_to(ROOT)))
        return
    BUILD.mkdir(parents=True, exist_ok=True)
    if git:
        if run(
            [git, "clone", "--depth", "1", M2C_REPOSITORY, str(destination)],
            reporter,
            "m2c checkout",
            required=False,
        ):
            return
        if destination.exists() and not script.is_file():
            reporter.warn("m2c clone cleanup", f"remove incomplete path manually: {destination}")
            return
    try:
        download_m2c_archive(destination)
    except RuntimeError as error:
        reporter.fail("m2c checkout", str(error))
    else:
        reporter.pass_("m2c checkout", "downloaded source archive to build/m2c")


def venv_python() -> Path:
    if os.name == "nt":
        return BUILD / "venv" / "Scripts" / "python.exe"
    return BUILD / "venv" / "bin" / "python"


def setup_m2c_python(reporter: Reporter) -> None:
    script = BUILD / "m2c" / "m2c.py"
    if not script.is_file():
        reporter.fail("m2c Python setup", "build/m2c/m2c.py is missing")
        return
    python = venv_python()
    local_environment = python.is_file()
    if not python.is_file():
        local_environment = run(
            [sys.executable, "-m", "venv", str(BUILD / "venv")],
            reporter,
            "Local Python environment",
            required=False,
        )
        if not local_environment:
            python = Path(sys.executable)
    requirements = BUILD / "m2c" / "requirements.txt"
    if requirements.is_file():
        marker = BUILD / "venv" / ".m2c-requirements.sha1"
        digest = sha1_file(requirements)
        installed = marker.is_file() and marker.read_text(encoding="ascii").strip() == digest
        if not installed:
            if local_environment:
                ok = run(
                    [str(python), "-m", "pip", "install", "-r", str(requirements)],
                    reporter,
                    "m2c Python dependencies",
                    required=False,
                )
            else:
                reporter.warn(
                    "m2c Python dependencies",
                    "local venv is unavailable; refusing to install into the system Python",
                )
                ok = False
            if ok:
                marker.parent.mkdir(parents=True, exist_ok=True)
                marker.write_text(digest + "\n", encoding="ascii")
        else:
            reporter.pass_("m2c Python dependencies", "requirements are current")
    else:
        reporter.pass_("m2c Python dependencies", "checkout has no requirements.txt")
    run(
        [str(python), str(script), "--help"],
        reporter,
        "m2c smoke test",
    )


def tool_ready(name: str, output: Path) -> bool:
    if name == "compilers":
        return (
            (output / "GC" / "2.7" / "mwcceppc.exe").is_file()
            and (output / "GC" / "2.7" / "mwldeppc.exe").is_file()
        )
    if name == "binutils":
        return output.is_dir() and any(output.rglob(executable_name("powerpc-eabi-as")))
    return output.is_file()


def configured_tool_tags() -> dict[str, str]:
    text = (ROOT / "configure.py").read_text(encoding="utf-8")
    tags: dict[str, str] = {}
    for _, _, attribute in TOOL_OUTPUTS:
        match = re.search(
            rf'^config\.{re.escape(attribute)}\s*=\s*"([^"]+)"\s*$',
            text,
            re.MULTILINE,
        )
        if match is None:
            raise ValueError(f"configure.py does not define config.{attribute}")
        tags[attribute] = match.group(1)
    match = re.search(r'^config\.wibo_tag\s*=\s*"([^"]+)"\s*$', text, re.MULTILINE)
    if match is None:
        raise ValueError("configure.py does not define config.wibo_tag")
    tags["wibo_tag"] = match.group(1)
    return tags


def ensure_downloads(reporter: Reporter) -> None:
    downloader = ROOT / "tools" / "download_tool.py"
    try:
        tags = configured_tool_tags()
    except (OSError, ValueError) as error:
        reporter.fail("Tool version configuration", str(error))
        return
    for name, output, attribute in TOOL_OUTPUTS:
        if tool_ready(name, output):
            reporter.pass_(f"Tool: {name}", str(output.relative_to(ROOT)))
            continue
        downloaded = run(
            [
                sys.executable,
                str(downloader),
                name,
                str(output),
                "--tag",
                tags[attribute],
            ],
            reporter,
            f"Download: {name}",
        )
        if downloaded:
            if tool_ready(name, output):
                reporter.pass_(f"Tool: {name}", str(output.relative_to(ROOT)))
            else:
                reporter.fail(f"Tool: {name}", "download completed but expected files are missing")

    if os.name != "nt":
        output = BUILD / "tools" / "wibo"
        if tool_ready("wibo", output):
            reporter.pass_("Tool: wibo", str(output.relative_to(ROOT)))
        else:
            downloaded = run(
                [
                    sys.executable,
                    str(downloader),
                    "wibo",
                    str(output),
                    "--tag",
                    tags["wibo_tag"],
                ],
                reporter,
                "Download: wibo",
            )
            if downloaded:
                if tool_ready("wibo", output):
                    reporter.pass_("Tool: wibo", str(output.relative_to(ROOT)))
                else:
                    reporter.fail("Tool: wibo", "download completed but expected file is missing")


def configure_and_build(
    ninja: Optional[str],
    reporter: Reporter,
    retail_ok: bool,
) -> None:
    configured = run(
        [sys.executable, str(ROOT / "configure.py")],
        reporter,
        "Generate build files",
    )
    if not configured:
        return
    if ninja is None:
        reporter.fail("Initial matching build", "ninja is missing; install it and rerun setup")
        return
    if not retail_ok:
        reporter.skip("Initial matching build", "retail input did not pass SHA-1 validation")
        return
    run([ninja], reporter, "Initial matching build")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate the GQNE5D retail input and bootstrap all local matching tools."
    )
    parser.add_argument(
        "--iso",
        type=Path,
        help="raw GQNE5D ISO/GCM to hash and validate; by default search orig/GQNE5D",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    os.chdir(ROOT)
    reporter = Reporter()

    print("MKD matching environment setup")
    print(f"Repository: {ROOT}")
    print()

    ninja, git = check_host_tools(reporter)
    retail_ok = validate_retail_input(args.iso, reporter)
    init_submodules(git, reporter)
    ensure_m2c(git, reporter)
    setup_m2c_python(reporter)
    ensure_downloads(reporter)
    configure_and_build(ninja, reporter, retail_ok)
    return reporter.summary()


if __name__ == "__main__":
    raise SystemExit(main())
