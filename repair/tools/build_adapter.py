#!/usr/bin/env python3
"""Builds GAYClientInternal.dll with the RmlUi in-game menu.

Compiles the vendored FreeType and RmlUi sources plus the adapter sources
into repair/build/ and links the final DLL with the zig cross-compiler
(`zig c++`, target x86_64-windows-gnu). Object files are cached: unchanged
sources are not recompiled on repeated runs.

Notes mirrored from the vendored sources:
  - FreeType uses the minimal module list from vendor/freetype-config/
    ftmodule.h (TrueType/OpenType rendering only).
  - RmlUi is compiled statically with -DRMLUI_STATIC_LIB so RMLUICORE_API
    is empty: no dllimport mismatches and no Rml exports leak into the DLL
    export table, which keeps exactly GAYClientConnectWheel.
  - -DNUM_MSAA_SAMPLES=0 makes the DX11 renderer create plain TEXTURE2D
    render targets, matching its own post-process path; with samples=1 the
    view dimension (TEXTURE2DMS) would not match the non-multisampled
    texture and render target creation would fail.

Usage:
    python3 repair/tools/build_adapter.py            # build the DLL
    python3 repair/tools/build_adapter.py --clean    # rebuild everything
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
REPAIR = os.path.join(REPO, "repair")
VENDOR = os.path.join(REPAIR, "vendor")
BUILD = os.path.join(REPAIR, "build")
TARGET = "x86_64-windows-gnu"

FREETYPE_FLAGS = [
    "-x", "c", "-c", "-O1", "-DFT2_BUILD_LIBRARY",
    "-DFT_CONFIG_MODULES_H=<ftmodule.h>",
    f"-I{VENDOR}{os.sep}freetype-config",
    f"-I{VENDOR}{os.sep}freetype{os.sep}include",
]

RMLUI_FLAGS = [
    "-std=c++17", "-c", "-O1", "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
    "-DRMLUI_STATIC_LIB", "-DNUM_MSAA_SAMPLES=0",
    f"-I{VENDOR}{os.sep}rmlui{os.sep}Include",
    f"-I{VENDOR}{os.sep}rmlui{os.sep}Source",
    f"-I{VENDOR}{os.sep}rmlui{os.sep}Backends",
    f"-I{VENDOR}{os.sep}freetype{os.sep}include",
]

ADAPTER_FLAGS = [
    "-std=c++17", "-c", "-O2", "-DNOMINMAX", "-DWIN32_LEAN_AND_MEAN",
    "-DRMLUI_STATIC_LIB",
    f"-I{REPO}",
    f"-I{REPAIR}{os.sep}src",
    f"-I{VENDOR}{os.sep}rmlui{os.sep}Include",
    f"-I{VENDOR}{os.sep}rmlui{os.sep}Backends",
    f"-I{VENDOR}{os.sep}imgui",
]

# Minimal FreeType build: the base library plus exactly the module set
# declared in vendor/freetype-config/ftmodule.h.
FREETYPE_SRCS = [
    "src/base/ftbase.c",
    "src/base/ftsystem.c",
    "src/base/ftinit.c",
    "src/base/ftdebug.c",
    "src/base/ftbitmap.c",
    "src/base/ftglyph.c",
    "src/base/ftsynth.c",
    "src/base/ftstroke.c",
    "src/base/ftmm.c",
    "src/gzip/ftgzip.c",
    "src/autofit/autofit.c",
    "src/cff/cff.c",
    "src/psaux/psaux.c",
    "src/psnames/psnames.c",
    "src/pshinter/pshinter.c",
    "src/raster/raster.c",
    "src/smooth/smooth.c",
    "src/sfnt/sfnt.c",
    "src/truetype/truetype.c",
]

ADAPTER_SRCS = [
    "compat.cpp",
    "uwp_extra.cpp",
    "china_hat_native.cpp",
    "menu_mouse_native.cpp",
    "menu_rml.cpp",
]

LINK_LIBS = [
    "-ld3d11", "-ldxgi", "-ld3dcompiler_47", "-luser32", "-lole32",
    "-lapi-ms-win-core-winrt-l1-1-0", "-lapi-ms-win-core-winrt-string-l1-1-0",
    "-luuid", "-lbcrypt",
]


def find_zig() -> str:
    zig = shutil.which("zig")
    if zig:
        return zig
    # The pip 'ziglang' package ships zig next to its python modules.
    candidates = []
    try:
        import site
        for directory in site.getsitepackages() + [site.getusersitepackages()]:
            candidates.append(os.path.join(directory, "ziglang", "zig"))
    except Exception:
        pass
    candidates.append("/usr/local/lib/python3.11/dist-packages/ziglang/zig")
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    sys.exit("zig not found: install it or run "
             "'pip3 install --break-system-packages ziglang'")


def ensure_group_stamps(groups: dict) -> None:
    """Invalidate cached objects whenever a group's flags change."""
    for group, flags in groups.items():
        directory = os.path.join(BUILD, group)
        stamp = os.path.join(directory, "flags.txt")
        content = " ".join(flags) + "\n"
        if os.path.isfile(stamp):
            try:
                if open(stamp, encoding="utf-8").read() == content:
                    continue
            except OSError:
                pass
        if os.path.isdir(directory):
            shutil.rmtree(directory)
        os.makedirs(directory, exist_ok=True)
        open(stamp, "w", encoding="utf-8").write(content)


def object_path(group: str, relative: str) -> str:
    name = relative.replace("..", "__").replace(os.sep, "_").replace("/", "_")
    return os.path.join(BUILD, group, name + ".o")


def compile_job(zig: str, flags, source: str, obj: str):
    if os.path.isfile(obj) and os.path.getmtime(obj) >= os.path.getmtime(source):
        return (obj, 0, "")
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    command = [zig, "c++", "-target", TARGET, *flags, source, "-o", obj]
    result = subprocess.run(command, capture_output=True, text=True)
    return (obj, result.returncode, result.stderr or result.stdout)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clean", action="store_true",
                        help="remove cached objects before building")
    args = parser.parse_args()

    if args.clean and os.path.isdir(BUILD):
        shutil.rmtree(BUILD)
    os.makedirs(BUILD, exist_ok=True)
    zig = find_zig()
    ensure_group_stamps({
        "freetype": FREETYPE_FLAGS,
        "rmlui": RMLUI_FLAGS,
        "adapter": ADAPTER_FLAGS,
    })

    jobs = []
    for src in FREETYPE_SRCS:
        jobs.append((FREETYPE_FLAGS, os.path.join(VENDOR, "freetype", src),
                     object_path("freetype", src)))
    rmlui_dir = os.path.join(VENDOR, "rmlui")
    rmlui_sources = []
    for root, _dirs, files in os.walk(os.path.join(rmlui_dir, "Source")):
        for file in files:
            if file.endswith(".cpp"):
                rmlui_sources.append(os.path.join(root, file))
    rmlui_sources.append(os.path.join(rmlui_dir, "Backends", "RmlUi_Renderer_DX11.cpp"))
    for src in sorted(rmlui_sources):
        relative = os.path.relpath(src, rmlui_dir)
        jobs.append((RMLUI_FLAGS, src, object_path("rmlui", relative)))
    for src in ADAPTER_SRCS:
        jobs.append((ADAPTER_FLAGS, os.path.join(REPAIR, "src", src),
                     object_path("adapter", src)))

    objects = []
    failures = []
    workers = min(16, (os.cpu_count() or 4) * 2)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(lambda job: compile_job(zig, *job), jobs))
    for obj, code, output in results:
        if code != 0:
            failures.append((obj, output))
        else:
            objects.append(obj)

    if failures:
        for obj, output in failures:
            print(f"FAILED: {obj}", file=sys.stderr)
            print(output, file=sys.stderr)
        return 1

    dll = os.path.join(REPAIR, "GAYClientInternal.dll")
    command = [zig, "c++", "-target", TARGET, "-std=c++17", "-shared", "-O2",
               *objects, "-o", dll, *LINK_LIBS]
    print(f"linking {len(objects)} objects -> {dll}")
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr or result.stdout, file=sys.stderr)
        return 1
    print(result.stdout.strip())
    print(f"built {dll} ({os.path.getsize(dll)} bytes, {len(objects)} objects)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
