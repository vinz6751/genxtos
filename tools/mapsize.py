#!/usr/bin/env python3
"""Summarize EmuTOS/GenXTOS linker map TEXT footprint.

usage:
  tools/mapsize.py [-n N] [-r ROMSIZE_K] [map]
  tools/mapsize.py -b baseline.map [-r ROMSIZE_K] [map]

Reports total TEXT, free space vs a padded ROM size, top objects, per-tree
totals, and optional delta vs a baseline map.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

# Map object path → logical tree for rollups.
TREE_RULES = (
    ("cmd", "cli"),
    ("desk", "desk"),
    ("icons", "desk"),
    ("gem", "aes"),
    ("aes", "aes"),
    ("vdi", "vdi"),
    ("linea", "vdi"),
    ("raster", "vdi"),
    ("fs", "bdos"),
    ("bdos", "bdos"),
    ("proc", "bdos"),
    ("umem", "bdos"),
    ("osmem", "bdos"),
    ("imap", "bdos"),
    ("time", "bdos"),
    ("mem", "bdos"),
    ("pghurt", "bdos"),
    ("runcmnd", "bdos"),
    ("setjmp", "util"),
    ("string", "util"),
    ("nls", "util"),
    ("langs", "util"),
    ("miscutil", "util"),
    ("shellutl", "util"),
    ("optimize", "util"),
    ("gemdos", "util"),
    ("intmath", "util"),
    ("lisautil", "util"),
    ("fnt_", "font"),
    ("libfont", "font"),
)


OBJ_TEXT_RE = re.compile(
    r"^\s*(?:\.text(?:\.\S*)?\s+)?0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)\s+(\S+\.o)\s*$"
)
SECTION_RE = re.compile(r"^\.(\w+)")


def tree_for(obj: str) -> str:
    base = Path(obj).name
    for prefix, tree in TREE_RULES:
        if base.startswith(prefix) or prefix in base:
            return tree
    if base.startswith(("bios", "conout", "floppy", "disk", "ide", "clock",
                        "serport", "ikbd", "initinfo", "machine", "memory",
                        "natfeat", "screen", "sound", "dma", "blkdev",
                        "aciavecs", "mfp", "scc", "midi", "printer", "mouse",
                        "font", "keyb", "country", "cookie", "xhdi", "nvram",
                        "dmasound", "kprint", "panic", "processor", "paraport",
                        "delay", "chardev", "videl", "tt", "falcon", "blitter",
                        "a2560", "amiga", "lisa", "spi_", "endrom", "vectors",
                        "startup", "biosinit", "biosmem", "lowstram", "tosvars",
                        "lineainit", "mousex", "noblank", "panel", "pmmu",
                        "rtc", "scsi", "sd", "videl", "vmmu", "xbios")):
        return "bios"
    return "other"


def parse_map(path: Path) -> dict[str, int]:
    objs: dict[str, int] = defaultdict(int)
    section = None
    for line in path.read_text(errors="replace").splitlines():
        msec = SECTION_RE.match(line)
        if msec and " " in line:
            name = msec.group(1)
            if name in ("text", "data", "bss"):
                section = name
            elif line.startswith("."):
                # Other top-level output section.
                section = name
        if section != "text":
            continue
        m = OBJ_TEXT_RE.match(line)
        if m:
            objs[m.group(2)] += int(m.group(1), 16)
    return dict(objs)


def section_size(path: Path, name: str) -> int | None:
    """Return the size field of a top-level output section, if present."""
    pat = re.compile(rf"^\.{name}\s+0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)")
    for line in path.read_text(errors="replace").splitlines():
        m = pat.match(line)
        if m:
            return int(m.group(1), 16)
    return None


def report(objs: dict[str, int], romsize_k: int | None, top_n: int,
           baseline: dict[str, int] | None, map_path: Path,
           baseline_path: Path | None = None) -> None:
    text = section_size(map_path, "text")
    obj_sum = sum(objs.values())
    if text is None:
        text = obj_sum

    print(f"map:  {map_path}")
    print(f"TEXT: {text} bytes (0x{text:x})")
    print(f"objs: {obj_sum} bytes across {len(objs)} objects")
    if romsize_k:
        pad = romsize_k * 1024
        free = pad - text
        print(f"ROM:  {pad} bytes ({romsize_k} KiB) → {free} bytes free "
              f"({100.0 * free / pad:.2f}%)")

    trees: dict[str, int] = defaultdict(int)
    for obj, size in objs.items():
        trees[tree_for(obj)] += size
    print("\nby tree:")
    for tree, size in sorted(trees.items(), key=lambda kv: -kv[1]):
        print(f"  {size:7d}  {tree}")

    print(f"\ntop {top_n} objects:")
    for obj, size in sorted(objs.items(), key=lambda kv: -kv[1])[:top_n]:
        print(f"  {size:7d}  {obj}")

    if baseline is not None:
        # Prefer .text section sizes (LTO maps have few per-.o lines).
        b_text = section_size(baseline_path, "text") if baseline_path else None
        if b_text is None:
            b_text = sum(baseline.values())
        delta = text - b_text
        print(f"\nvs baseline TEXT {b_text}: delta {delta:+d} bytes")
        if not any("/tmp/" in k or "ltrans" in k for k in objs):
            deltas: dict[str, int] = {}
            all_keys = set(objs) | set(baseline)
            for k in all_keys:
                d = objs.get(k, 0) - baseline.get(k, 0)
                if d:
                    deltas[k] = d
            print("largest object deltas:")
            for obj, d in sorted(deltas.items(), key=lambda kv: -abs(kv[1]))[:top_n]:
                print(f"  {d:+7d}  {obj}")
        else:
            print("(object deltas omitted: LTO ltrans map)")

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("map", nargs="?", default="emutos.map",
                    help="linker map (default: emutos.map)")
    ap.add_argument("-n", type=int, default=25, help="top N objects (default 25)")
    ap.add_argument("-r", "--romsize", type=int, default=256,
                    metavar="K", help="padded ROM size in KiB (default 256; 0=omit)")
    ap.add_argument("-b", "--baseline", type=Path,
                    help="baseline map for delta comparison")
    args = ap.parse_args()

    map_path = Path(args.map)
    if not map_path.is_file():
        print(f"error: {map_path} not found", file=sys.stderr)
        return 2

    objs = parse_map(map_path)
    baseline = parse_map(args.baseline) if args.baseline else None
    romsize = args.romsize if args.romsize > 0 else None
    report(objs, romsize, args.n, baseline, map_path, args.baseline)
    return 0


if __name__ == "__main__":
    sys.exit(main())
