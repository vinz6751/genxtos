#!/usr/bin/env python3
"""Pack erd-generated desk_rsc.c into a compact tree encoding.

usage:
  tools/deskpack.py [desk/desk_rsc.c] [desk/desk_rsc_data.c]

Reads the C that would be compiled (desk_rsc.c or desk_rsc.tr.c after
bug translate) and emits parallel-column tables plus a string pool for
desk/desk_rsc_pack.c to expand at desktop start.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_IN = ROOT / "desk" / "desk_rsc.c"
DEFAULT_OUT = ROOT / "desk" / "desk_rsc_data.c"

FLAG_WORDS = {
    "NONE": 0x0000,
    "SELECTABLE": 0x0001,
    "DEFAULT": 0x0002,
    "EXIT": 0x0004,
    "EDITABLE": 0x0008,
    "RBUTTON": 0x0010,
    "LASTOB": 0x0020,
    "TOUCHEXIT": 0x0040,
    "HIDETREE": 0x0080,
    "INDIRECT": 0x0100,
    "FL3DOBJ": 0x0200,
    "FL3DIND": 0x0200,
    "FL3DBAK": 0x0400,
    "FL3DACT": 0x0600,
    "SUBMENU": 0x0800,
}

STATE_WORDS = {
    "NORMAL": 0x0000,
    "SELECTED": 0x0001,
    "CROSSED": 0x0002,
    "CHECKED": 0x0004,
    "DISABLED": 0x0008,
    "OUTLINED": 0x0010,
    "SHADOWED": 0x0020,
    "WHITEBAK": 0x0040,
}

TYPE_WORDS = {
    "G_BOX": 20,
    "G_TEXT": 21,
    "G_BOXTEXT": 22,
    "G_IMAGE": 23,
    "G_USERDEF": 24,
    "G_IBOX": 25,
    "G_BUTTON": 26,
    "G_BOXCHAR": 27,
    "G_STRING": 28,
    "G_FTEXT": 29,
    "G_FBOXTEXT": 30,
    "G_ICON": 31,
    "G_TITLE": 32,
    "G_CICON": 33,
}

JUST_WORDS = {"TE_LEFT": 0, "TE_RIGHT": 1, "TE_CNTR": 2}

KNOWN_TYPES = frozenset(
    {"G_BOX", "G_IBOX", "G_STRING", "G_BUTTON", "G_TITLE", "G_FTEXT", "G_FBOXTEXT", "G_IMAGE"}
)


def unesc(s: str) -> bytes:
    out = bytearray()
    i = 0
    mp = {"n": 10, "r": 13, "t": 9, "\\": 92, "'": 39, '"': 34}
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            c = s[i + 1]
            if c in mp:
                out.append(mp[c])
                i += 2
            elif c in "01234567":
                j = i + 1
                octs = ""
                while j < len(s) and len(octs) < 3 and s[j] in "01234567":
                    octs += s[j]
                    j += 1
                out.append(int(octs, 8))
                i = j
            else:
                out.append(ord(c))
                i += 2
        else:
            out.append(ord(s[i]))
            i += 1
    return bytes(out)


def parse_or_expr(expr: str, table: dict[str, int]) -> int:
    expr = expr.strip()
    # Allow TYPE|0xNNNN for extended object type high byte
    val = 0
    for part in expr.split("|"):
        part = part.strip()
        if part in table:
            val |= table[part]
        elif part.startswith("0x") or part.startswith("0X"):
            val |= int(part, 16)
        elif part.isdigit() or (part.startswith("-") and part[1:].isdigit()):
            val |= int(part, 0)
        else:
            raise ValueError(f"unknown flag/state token {part!r} in {expr!r}")
    return val


def c_bytes(data: bytes, width: int = 12) -> str:
    lines = []
    for i in range(0, len(data), width):
        chunk = data[i : i + width]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    return "\n".join(lines)


def c_u16(vals: list[int], width: int = 8) -> str:
    lines = []
    for i in range(0, len(vals), width):
        chunk = vals[i : i + width]
        lines.append("    " + ", ".join(f"0x{v & 0xffff:04x}" for v in chunk) + ",")
    return "\n".join(lines)


def c_u32(vals: list[int], width: int = 4) -> str:
    lines = []
    for i in range(0, len(vals), width):
        chunk = vals[i : i + width]
        lines.append("    " + ", ".join(f"0x{v & 0xffffffff:08x}UL" for v in chunk) + ",")
    return "\n".join(lines)


def parse_desk_rsc(text: str) -> dict:
    named: dict[str, bytes] = {}
    for m in re.finditer(
        r"static const char (rs_str_\w+)\[\] = (?:N_)?\(\"((?:\\.|[^\"\\])*)\"\)",
        text,
    ):
        named[m.group(1)] = unesc(m.group(2))

    # Also catch N_("...") assigned without named array when bug leave them inline
    tr = {int(a): int(b) for a, b in re.findall(r"#define TR(\d+) (\d+)", text)}
    if not tr:
        raise SystemExit("no TR# tree starts found")
    starts = [tr[i] for i in range(max(tr) + 1)]

    # Objects
    start = text.find("static const OBJECT desk_rs_obj_rom[]")
    if start < 0:
        raise SystemExit("desk_rs_obj_rom not found")
    end = text.find("OBJECT * desk_rs_trees", start)
    body = text[start:end]
    chunks = re.split(r"\n\s*\{", body)[1:]
    objs = []
    for ch in chunks:
        m = re.match(r"\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*([^,\n]+)", ch)
        if not m:
            raise SystemExit(f"bad object head: {ch[:80]!r}")
        rest = [ln.strip() for ln in ch.splitlines()[1:] if ln.strip()]
        fl, st, spec = rest[0].rstrip(","), rest[1].rstrip(","), rest[2].rstrip(",")
        mm = re.match(r"(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}", rest[3])
        if not mm:
            raise SystemExit(f"bad object xywh: {rest[3]!r}")
        typ_expr = m.group(4).strip()
        # strip trailing comment markers already handled
        base_type = typ_expr.split("|", 1)[0].strip()
        if base_type not in KNOWN_TYPES:
            raise SystemExit(f"unsupported object type {typ_expr!r}")
        objs.append(
            {
                "nx": int(m.group(1)),
                "hd": int(m.group(2)),
                "tl": int(m.group(3)),
                "typ_expr": typ_expr,
                "typ": parse_or_expr(typ_expr, TYPE_WORDS),
                "base": base_type,
                "fl": parse_or_expr(fl, FLAG_WORDS),
                "st": parse_or_expr(st, STATE_WORDS),
                "spec": spec,
                "x": int(mm.group(1)),
                "y": int(mm.group(2)),
                "w": int(mm.group(3)),
                "h": int(mm.group(4)),
            }
        )
    n = len(objs)

    # Make tree-local indices absolute for link walk
    abs_objs = [dict(o) for o in objs]
    for ti, s in enumerate(starts):
        e = starts[ti + 1] if ti + 1 < len(starts) else n
        for i in range(s, e):
            for k in ("nx", "hd", "tl"):
                if abs_objs[i][k] >= 0:
                    abs_objs[i][k] += s

    def kids(i: int) -> list[int]:
        o = abs_objs[i]
        if o["hd"] < 0:
            return []
        out = []
        c = o["hd"]
        for _ in range(64):
            out.append(c)
            if c == o["tl"]:
                return out
            c = abs_objs[c]["nx"]
            if c < 0:
                raise SystemExit(f"broken sibling walk at object {i}")
        raise SystemExit(f"sibling walk overflow at object {i}")

    counts = [len(kids(i)) for i in range(n)]

    # Verify reconstruction as TREE-LOCAL links (AES walks tree[obj].ob_*).
    nx = [-1] * n
    hd = [-1] * n
    tl = [-1] * n

    def rec(i: int, root: int) -> int:
        c = counts[i]
        if c == 0:
            hd[i] = tl[i] = -1
            return i + 1
        p = i + 1
        prev = None
        for ci in range(c):
            if ci:
                nx[prev] = p - root
            prev = p
            p = rec(p, root)
        hd[i] = (i + 1) - root
        tl[i] = prev - root
        nx[prev] = i - root
        return p

    for s in starts:
        rec(s, s)
        nx[s] = -1
    for i in range(n):
        # Original erd output stores local links already.
        want = (objs[i]["nx"], objs[i]["hd"], objs[i]["tl"])
        got = (nx[i], hd[i], tl[i])
        if want != got:
            raise SystemExit(f"link rebuild mismatch at {i}: got {got} want {want}")

    # String pool + named resolution
    pool: list[bytes] = []
    imap: dict[bytes, int] = {}

    def intern(b: bytes) -> int:
        if b not in imap:
            imap[b] = len(pool)
            pool.append(b)
        return imap[b]

    def spec_str(sp: str) -> bytes | None:
        m = re.search(r'"((?:\\.|[^"\\])*)"', sp)
        if m:
            return unesc(m.group(1))
        # N_("...") may remain if bug multilang — reject
        if "NLS_MSGID_FLAG" in sp or re.search(r"\b\d+\s*\|\s*NLS_MSGID_FLAG", sp):
            raise SystemExit(
                "deskpack: NLS message ids found; pack UNIQUE-translated C only"
            )
        m = re.search(r"rs_str_\w+", sp)
        if m:
            return named[m.group(0)]
        # Bare N_(...) already matched as string above when bug rewrote to text;
        # if still N_("x") form:
        m = re.search(r'N_\("((?:\\.|[^"\\])*)"\)', sp)
        if m:
            return unesc(m.group(1))
        return None

    # TEDINFOs
    ted_body = text.split("desk_rs_tedinfo_rom[]")[1].split("};")[0]
    # Split on record boundaries: each starts with {&desk_rs_te_text or similar
    ted_recs = re.findall(r"\{([^{}]+(?:\{[^{}]*\}[^{}]*)*)\}", ted_body, re.S)
    # Fallback: split on /* N */
    if len(ted_recs) < 2:
        ted_recs = [r for r in re.split(r"\n\n", ted_body) if "IBM" in r]

    teds = []
    te_ptext_size = 0
    for rec in ted_recs:
        if "IBM" not in rec:
            continue
        lits = [unesc(s) for s in re.findall(r'"((?:\\.|[^"\\])*)"', rec)]
        names = re.findall(r"rs_str_\w+", rec)
        # template: named or first string after ptext; validation: last string
        # Pattern: {ptext, tmplt, valid, IBM, junk, JUST, color, junk, thick, txtlen, tmplen}
        if names:
            # first named is usually template; may also appear as cast
            tmpl = named[names[0]]
            # remaining literals are validation (and maybe inline templates)
            if lits:
                # if template was named, first lit might be valid OR extra
                valid = lits[-1]
            else:
                raise SystemExit("TEDINFO missing validation string")
        else:
            if len(lits) < 2:
                raise SystemExit(f"TEDINFO needs template+valid: {rec[:80]!r}")
            tmpl, valid = lits[0], lits[-1]
        just_m = re.search(r"TE_\w+", rec)
        if not just_m:
            raise SystemExit("TEDINFO missing justification")
        just = JUST_WORDS[just_m.group(0)]
        after = rec.split(just_m.group(0), 1)[1]
        nums = [int(x) for x in re.findall(r"-?\d+", after)]
        # color, junk2, thickness, txtlen, tmplen
        color, thickness, txtlen = nums[0], nums[2], nums[3]
        te_ptext_size += txtlen
        teds.append(
            {
                "tmpl": intern(tmpl),
                "valid": intern(valid),
                "just": just,
                "color": color,
                "thick": thickness,
                "txtlen": txtlen,
            }
        )

    # Object specs: intern strings
    flags: list[int] = []
    fmap: dict[int, int] = {}
    imms: list[int] = []
    immmap: dict[int, int] = {}

    def flag_index(v: int) -> int:
        if v not in fmap:
            fmap[v] = len(flags)
            flags.append(v)
        return fmap[v]

    def imm_index(v: int) -> int:
        if v not in immmap:
            immmap[v] = len(imms)
            imms.append(v)
        return immmap[v]

    specs: list[int] = []
    for o in objs:
        flag_index(o["fl"])
        sp = o["spec"]
        base = o["base"]
        if base in ("G_FTEXT", "G_FBOXTEXT"):
            m = re.search(r"tedinfo\[(\d+)\]", sp)
            if not m:
                raise SystemExit(f"tedinfo spec missing: {sp}")
            specs.append(int(m.group(1)))
        elif base == "G_IMAGE":
            m = re.search(r"bitblk\[(\d+)\]", sp)
            if not m:
                raise SystemExit(f"bitblk spec missing: {sp}")
            specs.append(int(m.group(1)))
        elif base in ("G_BOX", "G_IBOX"):
            m = re.search(r"-?\d+", sp.replace("LONG", ""))
            # (LONG) 4352L or (LONG) 0L
            m = re.search(r"(-?\d+)L?", sp)
            if not m:
                raise SystemExit(f"box color missing: {sp}")
            specs.append(imm_index(int(m.group(1))))
        else:
            b = spec_str(sp)
            if b is None:
                raise SystemExit(f"string spec missing: {sp}")
            specs.append(intern(b))

    flagi = [fmap[o["fl"]] for o in objs]
    types = [o["typ"] for o in objs]
    states = [o["st"] for o in objs]

    # Coordinates
    xylo: list[int] = []
    xyhi: list[tuple[int, int, int]] = []
    for i, o in enumerate(objs):
        for field, val in enumerate((o["x"], o["y"], o["w"], o["h"])):
            if val < 0 or val > 0xFFFF:
                raise SystemExit(f"coord out of range at {i}: {val}")
            xylo.append(val & 0xFF)
            hi = (val >> 8) & 0xFF
            if hi:
                xyhi.append((i, field, hi))

    # Frestrings: strip /* */ so adjacent N_("a" /*x*/ "b") concatenates.
    fstr_m = re.search(
        r"const char \* (?:const )?desk_rs_fstr\[\]\s*=\s*\{", text
    )
    if not fstr_m:
        raise SystemExit("desk_rs_fstr[] not found")
    fstr_body = text[fstr_m.end() :].split("};")[0]
    fstr_clean = re.sub(r"/\*.*?\*/", " ", fstr_body, flags=re.S)
    fstr_idx: list[int] = []
    for m in re.finditer(
        r"(?:N_\(\s*)?(?P<body>(?:\"(?:\\.|[^\"\\])*\"\s*)+)\)?",
        fstr_clean,
    ):
        parts = re.findall(r'"((?:\\.|[^"\\])*)"', m.group("body"))
        if not parts:
            continue
        fstr_idx.append(intern(b"".join(unesc(p) for p in parts)))
    if len(fstr_idx) != 56:
        # Desktop currently has STINFOST..STDUPCUT = 56 frestrings.
        raise SystemExit(f"unexpected frestring count {len(fstr_idx)} (expected 56)")

    # Bitmap
    bm = re.search(r"static const WORD rs_bitblk0\[\] = \{(.*?)\};", text, re.S)
    if not bm:
        raise SystemExit("rs_bitblk0 not found")
    bitwords = [int(w, 16) for w in re.findall(r"0x[0-9A-Fa-f]+", bm.group(1))]

    # BITBLK descriptors: { (void *)rs_bitblk0, wb, hl, x, y, color }
    bb_m = re.search(r"(?:const )?BITBLK desk_rs_bitblk\[\]\s*=\s*\{", text)
    if not bb_m:
        raise SystemExit("desk_rs_bitblk[] not found")
    bb_body = text[bb_m.end() :].split("};")[0]
    bitblks = []
    for m in re.finditer(
        r"\{\s*\(void\s*\*\)\s*rs_bitblk0\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}",
        bb_body,
    ):
        bitblks.append(tuple(int(m.group(i)) for i in range(1, 6)))
    if not bitblks:
        raise SystemExit("no BITBLK descriptors parsed")

    # Header defines
    nobs_m = re.search(r"#define\s+RS_NOBS\s+(\d+)", text)
    # Prefer values from desk_rsc.h via input; also accept from sibling .h
    return {
        "nobs": n,
        "ntree": len(starts),
        "nted": len(teds),
        "nbb": len(bitblks),
        "starts": starts,
        "counts": counts,
        "types": types,
        "states": states,
        "flagi": flagi,
        "flags": flags,
        "specs": specs,
        "imms": imms,
        "xylo": xylo,
        "xyhi": xyhi,
        "teds": teds,
        "te_ptext_size": te_ptext_size,
        "pool": pool,
        "fstr_idx": fstr_idx,
        "bitwords": bitwords,
        "bitblks": bitblks,
        "rs_nobs_hdr": int(nobs_m.group(1)) if nobs_m else n,
    }


def emit(data: dict, out: Path) -> None:
    pool = data["pool"]
    pool_bytes = b"\0".join(pool) + b"\0"
    # Build offset table for pool strings
    offs = []
    pos = 0
    for s in pool:
        offs.append(pos)
        pos += len(s) + 1

    xyhi_flat: list[int] = []
    for i, field, hi in data["xyhi"]:
        xyhi_flat.extend([i & 0xFF, (i >> 8) & 0xFF, field, hi])

    ted_flat: list[int] = []
    for t in data["teds"]:
        ted_flat.extend(
            [
                t["tmpl"] & 0xFF,
                (t["tmpl"] >> 8) & 0xFF,
                t["valid"] & 0xFF,
                (t["valid"] >> 8) & 0xFF,
                t["just"] & 0xFF,
                t["color"] & 0xFF,
                (t["color"] >> 8) & 0xFF,
                t["thick"] & 0xFF,
                t["txtlen"] & 0xFF,
            ]
        )

    bitblk_flat: list[int] = []
    for wb, hl, x, y, color in data["bitblks"]:
        bitblk_flat.extend([wb, hl, x, y, color])

    n = data["nobs"]
    lines = []
    lines.append("/*")
    lines.append(" * desk_rsc_data.c - packed EmuDesk resource (generated by deskpack.py)")
    lines.append(" * Do not edit; regenerate from desk/desk_rsc.c or desk/desk_rsc.tr.c.")
    lines.append(" */")
    lines.append('#include "emutos.h"')
    lines.append('#include "desk_rsc_data.h"')
    lines.append("")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_nchild[{n}] = {{")
    lines.append(c_bytes(bytes(data["counts"])))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_otype[{n}] = {{")
    lines.append(c_bytes(bytes(data["types"])))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_oflagi[{n}] = {{")
    lines.append(c_bytes(bytes(data["flagi"])))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_ostate[{n}] = {{")
    lines.append(c_bytes(bytes(data["states"])))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_xylo[{n * 4}] = {{")
    lines.append(c_bytes(bytes(data["xylo"])))
    lines.append("};")
    lines.append("")
    nhi = max(len(data["xyhi"]), 1)
    lines.append(f"const UBYTE desk_rs_xyhi[{nhi * 4}] = {{")
    lines.append(c_bytes(bytes(xyhi_flat)) if xyhi_flat else "    0,")
    lines.append("};")
    lines.append("")
    lines.append(f"const UWORD desk_rs_flagdict[{len(data['flags'])}] = {{")
    lines.append(c_u16(data["flags"]))
    lines.append("};")
    lines.append("")
    lines.append(f"const LONG desk_rs_immdict[{len(data['imms'])}] = {{")
    lines.append(c_u32(data["imms"]))
    lines.append("};")
    lines.append("")
    # Spec indices fit in a byte (pool < 256, ted/imm/bitblk tiny).
    if any(s > 255 for s in data["specs"]):
        raise SystemExit("desk_rs_ospec value exceeds UBYTE")
    lines.append(f"const UBYTE desk_rs_ospec[{n}] = {{")
    lines.append(c_bytes(bytes(data["specs"])))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_tedpack[{len(data['teds']) * 9}] = {{")
    lines.append(c_bytes(bytes(ted_flat)))
    lines.append("};")
    lines.append("")
    lines.append(f"const UBYTE desk_rs_pool[{len(pool_bytes)}] = {{")
    lines.append(c_bytes(pool_bytes))
    lines.append("};")
    lines.append("")
    lines.append(f"const UWORD desk_rs_pooloff[{len(pool)}] = {{")
    lines.append(c_u16(offs))
    lines.append("};")
    lines.append("")
    lines.append(f"const UWORD desk_rs_fstridx[{len(data['fstr_idx'])}] = {{")
    lines.append(c_u16(data["fstr_idx"]))
    lines.append("};")
    lines.append("")
    lines.append(f"const UWORD desk_rs_treestart[{len(data['starts'])}] = {{")
    lines.append(c_u16(data["starts"]))
    lines.append("};")
    lines.append("")
    lines.append(f"const UWORD desk_rs_bmdata[{len(data['bitwords'])}] = {{")
    lines.append(c_u16(data["bitwords"]))
    lines.append("};")
    lines.append("")
    lines.append(f"const WORD desk_rs_bminfo[{len(data['bitblks']) * 5}] = {{")
    flat: list[int] = []
    for t in data["bitblks"]:
        flat.extend(t)
    lines.append("    " + ", ".join(str(v) for v in flat) + ",")
    lines.append("};")
    lines.append("")

    out.write_text("\n".join(lines) + "\n")
    print(
        f"deskpack: {n} objects, pool {len(pool_bytes)}, "
        f"flags {len(data['flags'])}, fstr {len(data['fstr_idx'])} -> {out}"
    )


def emit_header(out: Path, data: dict) -> None:
    hdr = out.parent / "desk_rsc_data.h"
    text = f"""\
/*
 * desk_rsc_data.h - packed EmuDesk resource tables (generated)
 */
#ifndef DESK_RSC_DATA_H
#define DESK_RSC_DATA_H

/* Requires emutos.h / portab.h for UBYTE, UWORD, WORD, LONG. */

#define DESK_RS_NFSTR   {len(data['fstr_idx'])}
#define DESK_RS_NFLAG   {len(data['flags'])}
#define DESK_RS_NIMM    {len(data['imms'])}
#define DESK_RS_NHI     {len(data['xyhi'])}
#define DESK_RS_POOLSZ  {sum(len(s) + 1 for s in data['pool'])}
#define DESK_RS_TEBUFSZ {data['te_ptext_size']}
#define DESK_RS_BMWORDS {len(data['bitwords'])}
#define DESK_RS_NPOOL   {len(data['pool'])}

extern const UBYTE desk_rs_nchild[];
extern const UBYTE desk_rs_otype[];
extern const UBYTE desk_rs_oflagi[];
extern const UBYTE desk_rs_ostate[];
extern const UBYTE desk_rs_xylo[];
extern const UBYTE desk_rs_xyhi[];
extern const UWORD desk_rs_flagdict[];
extern const LONG desk_rs_immdict[];
extern const UBYTE desk_rs_ospec[];
extern const UBYTE desk_rs_tedpack[];
extern const UBYTE desk_rs_pool[];
extern const UWORD desk_rs_pooloff[];
extern const UWORD desk_rs_fstridx[];
extern const UWORD desk_rs_treestart[];
extern const UWORD desk_rs_bmdata[];
extern const WORD desk_rs_bminfo[];

#endif /* DESK_RSC_DATA_H */
"""
    hdr.write_text(text)


def main() -> None:
    inp = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_IN
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUT
    data = parse_desk_rsc(inp.read_text(errors="replace"))
    emit(data, out)
    emit_header(out, data)


if __name__ == "__main__":
    main()
