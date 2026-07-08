#!/usr/bin/env python3
"""Build the host snapshot harness (LVGL + dash UI) with zig's clang.

Usage (from firmware/):  python tools/build_host.py [--run]
Objects are cached in .pio/host_objs; output exe is .pio/host_dash.exe.
Requires: pip install ziglang; LVGL fetched by `pio run` (.pio/libdeps/dash/lvgl).
"""
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LVGL = os.path.join(ROOT, ".pio", "libdeps", "dash", "lvgl")
OBJS = os.path.join(ROOT, ".pio", "host_objs")
EXE = os.path.join(ROOT, ".pio", "host_dash.exe")
RENDERS = os.path.join(ROOT, "renders")

INCLUDES = ["-I" + os.path.join(ROOT, "include"), "-I" + LVGL, "-I" + os.path.join(ROOT, "src")]
DEFINES = ["-DLV_CONF_INCLUDE_SIMPLE"]
CFLAGS = ["-O1", "-fno-sanitize=undefined"]

ZIG = [sys.executable, "-m", "ziglang"]


def gather_sources():
    srcs = []
    for base, _, files in os.walk(os.path.join(LVGL, "src")):
        for f in files:
            if f.endswith(".c"):
                srcs.append(os.path.join(base, f))
    for sub in ("ui", os.path.join("ui", "fonts")):
        d = os.path.join(ROOT, "src", sub)
        for f in os.listdir(d):
            if f.endswith(".c"):
                srcs.append(os.path.join(d, f))
    srcs += [
        os.path.join(ROOT, "src", "sim.cpp"),
        os.path.join(ROOT, "src", "faults.cpp"),
        os.path.join(ROOT, "src", "gear_calc.cpp"),
        os.path.join(ROOT, "host", "main_host.c"),
    ]
    return srcs


def obj_path(src):
    rel = os.path.relpath(src, ROOT).replace("\\", "_").replace("/", "_").replace("..", "up")
    return os.path.join(OBJS, rel + ".o")


def compile_one(src):
    obj = obj_path(src)
    if os.path.exists(obj) and os.path.getmtime(obj) > os.path.getmtime(src):
        return obj, None
    cc = "c++" if src.endswith(".cpp") else "cc"
    std = ["-std=c++17"] if src.endswith(".cpp") else ["-std=c11"]
    cmd = ZIG + [cc] + std + CFLAGS + DEFINES + INCLUDES + ["-c", src, "-o", obj]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        return obj, f"{src}\n{r.stderr[-3000:]}"
    return obj, None


def main():
    if not os.path.isdir(LVGL):
        sys.exit("LVGL not found — run `pio run -e dash` first to fetch it")
    os.makedirs(OBJS, exist_ok=True)
    os.makedirs(RENDERS, exist_ok=True)

    srcs = gather_sources()
    print(f"compiling {len(srcs)} sources…")
    objs, errors = [], []
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        for obj, err in ex.map(compile_one, srcs):
            objs.append(obj)
            if err:
                errors.append(err)
    if errors:
        print("\n\n".join(errors[:5]))
        sys.exit(f"{len(errors)} compile errors")

    print("linking…")
    rsp = os.path.join(OBJS, "link.rsp")
    with open(rsp, "w") as f:
        f.write("\n".join('"' + o.replace("\\", "/") + '"' for o in objs))
    r = subprocess.run(ZIG + ["c++", "@" + rsp, "-o", EXE], capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(r.stderr[-3000:])
    print(f"built {EXE}")

    if "--run" in sys.argv:
        r = subprocess.run([EXE, RENDERS])
        sys.exit(r.returncode)


if __name__ == "__main__":
    main()
