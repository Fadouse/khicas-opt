#!/usr/bin/env python3
"""Build official or optimized sources in separate, cached directories."""

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
from repository import build_inputs

ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = "upstream/khicas-2026-07-31"
# Prioritize integration, simplification, solving and matrix arithmetic.
# yintg is in ROM: both linker regions must fit, not just the second add-in.
FAST_OBJECTS = "yintg.o zintgab.o zrisch.o zlin.o zseries.o zsolve.o zcsturm.o zvecteur.o zifactor.o zgauss.o zusual.o"


def portable_makefile(text):
    text = text.replace("/home/parisse/casiolocal", "$(CASIOLOCAL)")
    # Author-local copy destinations are not part of producing an add-in.
    text = (
        "\n".join(
            line
            for line in text.splitlines()
            if not line.lstrip().startswith("/bin/cp ")
        )
        + "\n"
    )
    # The single generator writes all language headers; serialize it under -j.
    text = text.replace(
        "khelpfr.o: khelpfr.cc static_help.h mkhelp\n\t./mkhelp",
        "khelpfr.o: khelpfr.cc .help-generated",
    )
    text = text.replace(
        "khelpen.o: khelpen.cc static_help.h\n\t./mkhelp",
        "khelpen.o: khelpen.cc .help-generated",
    )
    text += "\n.help-generated: static_help.h mkhelp\n\t./mkhelp\n\ttouch $@\n"
    text = text.replace("sort > dump_t", "sort > $@.symbols")
    text = text.replace("sort > dumpm_t", "sort > $@.symbols")
    return text


def prepare(profile, compiler):
    manifest = json.loads((ROOT / "UPSTREAM.json").read_text())
    digest = hashlib.sha256((profile + compiler + FAST_OBJECTS).encode())
    digest.update(Path(__file__).read_bytes())
    archive = None
    if profile == "official":
        commit = subprocess.check_output(["git", "rev-parse", UPSTREAM], cwd=ROOT)
        digest.update(commit)
    else:
        for name, path in sorted(build_inputs().items()):
            digest.update(name.encode())
            digest.update(path.read_bytes())
    fingerprint = digest.hexdigest()
    directory = ROOT / ".build" / profile
    marker = directory / ".source-stamp"
    if marker.exists() and marker.read_text() == fingerprint:
        return directory
    if directory.exists():
        if not marker.exists():
            raise RuntimeError(f"Unmanaged build directory: {directory}")
        shutil.rmtree(directory)
    directory.mkdir(parents=True)
    if profile == "official":
        archive = subprocess.check_output(["git", "archive", UPSTREAM], cwd=ROOT)
        with tarfile.open(fileobj=io.BytesIO(archive)) as source:
            source.extractall(directory, filter="data")
        for entry in manifest["files"]:
            path = directory / entry["path"]
            if hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
                raise RuntimeError(f"Official snapshot changed: {entry['path']}")
    else:
        for name, path in build_inputs().items():
            shutil.copy2(path, directory / name, follow_symlinks=False)
    makefile = portable_makefile((directory / "Makefile").read_text())
    if profile == "optimized":
        makefile += "\nCXXFLAGS += -ffunction-sections -fdata-sections -fstack-usage\n"
        makefile += "CFLAGS += -ffunction-sections -fdata-sections\n"
        makefile += f"\n{FAST_OBJECTS}: CXXFLAGS := $(filter-out -Os,$(CXXFLAGS)) -O2 -finline-functions\n"
    (directory / "Makefile").write_text(makefile)
    marker.write_text(fingerprint)
    return directory


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "profile", choices=("official", "optimized"), nargs="?", default="optimized"
    )
    parser.add_argument(
        "--tools-dir",
        type=Path,
        default=Path(os.environ.get("TOOLS_DIR", Path.home() / "khicas-toolchain")),
    )
    parser.add_argument("-j", "--jobs", type=int, default=min(os.cpu_count() or 4, 6))
    parser.add_argument("targets", nargs="*", default=["khicas50.g3a", "khicas50.ac2"])
    args = parser.parse_args()
    tools = args.tools_dir.resolve()
    candidates = (tools / "opt/sh3eb-elf/bin", tools / "casiolocal/opt/sh3eb-elf/bin")
    bindir = next(
        (path for path in candidates if (path / "sh3eb-elf-g++").exists()), None
    )
    if bindir is None:
        parser.error("Toolchain missing; see README.md and --tools-dir / TOOLS_DIR")
    env = dict(os.environ)
    env["PATH"] = f"{bindir}:{tools / 'bin'}:" + env.get("PATH", "")
    env["LD_LIBRARY_PATH"] = (
        str(tools / "runtime") + ":" + env.get("LD_LIBRARY_PATH", "")
    )
    compiler = subprocess.check_output(
        ["sh3eb-elf-g++", "--version"], env=env, text=True
    )
    source = prepare(args.profile, compiler)
    libraries = tools / "casiolocal"
    if not (libraries / "include/ustl").exists():
        parser.error(f"Libraries missing: {libraries}")
    subprocess.run(
        ["make", f"CASIOLOCAL={libraries}", f"-j{args.jobs}"] + args.targets,
        cwd=source,
        env=env,
        check=True,
    )
    print(f"Build outputs: {source}")


if __name__ == "__main__":
    main()
