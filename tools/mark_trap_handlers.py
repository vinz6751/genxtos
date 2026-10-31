#!/usr/bin/env python3
"""Add ARGS_ON_STACK to BIOS/XBIOS trap handler function definitions and declarations."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

HANDLERS = """
balloc_stram bconin bconout bconstat bcostat bmem_gettpa disk_drvrem drvmap getbpb getmpb
kbshift lrwabs mediach setexc tickcal bconmap bioskeys blitmode buffoper buffptr cursconf
devconnect DMAread DMAwrite dosound dsp_available dsp_blkbytes dsp_blkhandshake dsp_blkunpacked
dsp_blkwords dsp_doblock dsp_execboot dsp_execprog dsp_flushsubroutines dsp_getprogability
dsp_getwordsize dsp_hf0 dsp_hf1 dsp_hf2 dsp_hf3 dsp_hstat dsp_inqsubrability dsp_instream
dsp_iostream dsp_loadprog dsp_loadsubroutine dsp_lock dsp_lodtobinary dsp_multblocks
dsp_outstream dsp_removeinterrupts dsp_requestuniqueability dsp_reserve dsp_runsubroutine
dsp_setvectors dsp_triggerhc dsptristate dsp_unlock egetpalette egetshift esetbank esetcolor
esetgray esetpalette esetshift esetsmear flopfmt floprate floprd flopver flopwr getrez gettime
giaccess gpio ikbdws Initmous iorec jdisint jenabint kbdvbase kbrate keytbl locksnd logbase
mfpint midiws nvmaccess offgibit ongibit physbase protobt random rsconf scrdmp setbuffer
setcolor setinterrupt setmontracks setpalette setprt setscreen setsndmode settime settracks
sndstatus soundcmd unlocksnd vfixmode vgetrgb vgetsize vmontype vsetmode vsetrgb vsetsync vsync
xbtimer xbios_do_unimpl
""".split()

SKIP = {"supexec", "xbios_unimpl", "just_rts"}

RET_TYPES = r"(?:LONG|long|void|WORD|UWORD|ULONG|BOOL|UBYTE|const\s+UBYTE|static\s+UBYTE|static\s+const\s+UBYTE|UBYTE\s*\*|const\s+UBYTE\s*\*|static\s+LONG|static\s+long|static\s+void|static\s+WORD|static\s+ULONG|static\s+BOOL|static\s+UWORD|static\s+inline\s+LONG|static\s+inline\s+void|static\s+inline\s+WORD|OPTIMIZE_O2\s+LONG|OPTIMIZE_O2\s+void|OPTIMIZE_O3\s+LONG|OPTIMIZE_O3\s+void|OPTIMIZE_SMALL\s+LONG|OPTIMIZE_SMALL\s+void|NORETURN\s+void)"

FUNC_RE = re.compile(
    rf"^(\s*(?:{RET_TYPES})\s+)(?!ARGS_ON_STACK\s)({ '|'.join(re.escape(h) for h in HANDLERS if h not in SKIP) })\s*\(",
    re.MULTILINE,
)

def process_file(path: Path) -> bool:
    text = path.read_text()
    new_text, n = FUNC_RE.subn(r"\1ARGS_ON_STACK \2(", text)
    if n:
        path.write_text(new_text)
        print(f"{path}: {n} annotation(s)")
        return True
    return False


def main():
    changed = 0
    for ext in ("*.c", "*.h"):
        for path in ROOT.rglob(ext):
            if "tools/" in str(path) or ".git/" in str(path):
                continue
            if process_file(path):
                changed += 1
    print(f"Updated {changed} file(s)")


if __name__ == "__main__":
    main()
