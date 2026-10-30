# 256 KiB ROM size reclaim (keep functionality)

Measured on GenXTOS `MINTELF=1 FASTCALL=1` ST builds (Hatari STe / sthigh).

## Baseline vs winning build

| Build | TEXT (bytes) | Free in 256 KiB | Boot |
|-------|-------------:|----------------:|------|
| Pre-work map (`obj/baseline-256-pre-size.*`) | 258 860 | 3 284 | yes (older tree) |
| `FASTCALL=1` `LTO=0` | ~246 644 | ~15 500 | yes |
| `FASTCALL=0` `LTO=1` | ~249 684 | ~12 460 | yes |
| **`FASTCALL=1` `LTO=1`** (after blank_out fix) | **238 102** | **24 042** | **yes** |

Delta vs pre-work baseline: **−20 758** bytes TEXT (~9% free).

Tooling: `tools/mapsize.py [-b baseline.map] [-r 256] emutos.map`.

## What worked

1. **`LTO=1` with `MINTELF=1`** — largest reclaim; defaulted for `make … 256` / `192` when `MINTELF=1` (pass `LTO=0` to disable).
2. **FASTCALL + LTO blank_out fix** — LTO rewrote a naked `conout_blank_out` trampoline; boot died in `vt52` `clear_and_home` → `conout_blank_out` (illegal insn, stacked PC looked like `0x30`). Fix:
   - [`bios/conout.c`](../bios/conout.c): call `conout_atari_blank_out_impl(&BLANK_RECT)` instead of naked `4(sp)`/`jra` trampoline.
   - [`bios/conout_atarifb.c`](../bios/conout_atarifb.c): keep naked `conout_atari_blank_out` for asm `scroll_*` callers; compile this TU **without** LTO (`NOLTO` in Makefile).
   - [`bios/conout.h`](../bios/conout.h): export `BLANK_RECT` / `conout_atari_blank_out_impl`.
3. **UNIQUE country** (already on `256`/`192`) — RSC string sharing (`NOSHAREARG` empty); `langs.o` empty.
4. **`localconf.h` STE cuts** — already applied (`MPS_BLITTER_ALWAYS_ON`, etc.).

## What did not win / not enabled by default

| Lever | Result |
|-------|--------|
| `GC_SECTIONS=1` (`-ffunction-sections` + `--gc-sections`) | See “GC_SECTIONS / `_dskbuf`” below. Optional — measure vs LTO-only before enabling. |
| `ikbd_inject` | Already empty unless `NATFEAT_DEBUG=1`. |
| Feature cuts (`WITH_CLI=0`, `CONF_WITH_IDE=0`, …) | Out of scope for this pass. |

## GC_SECTIONS / `_dskbuf`

**Symptom:** `LTO=1 GC_SECTIONS=1` failed the linker ASSERT `bad _dskbuf alignment` in `.low_stram`.

**Mechanism:** `bios/lowstram.c` used to declare `dskbuf` with only `aligned(2)`, plus an unreferenced `dskbuf_alignment[3]` “padding” array. Without `-fdata-sections`, GCC emits one `.bss` blob in reverse declaration order (`shifty`, then the 3-byte pad, then `dskbuf` at offset 4) — so the ASSERT passed by accident. With `-fdata-sections`, each object gets `.bss.*`; `--gc-sections` then **drops** the never-referenced `.bss.dskbuf_alignment`. Layout becomes `shifty` (1 byte) + align-2 padding → `dskbuf` at `…02`, which fails `(addr & 3) == 0`.

**Fix:**
1. Give `dskbuf` `__attribute__((aligned(4)))` and remove the padding array.
2. Change `.low_stram` from `SUBALIGN(2)` to `ALIGN(4)`. `SUBALIGN(2)` **overrides** input-section alignment, so even a 4-byte-aligned `.bss.dskbuf` was placed on a 2-byte boundary (`shifty` @ `0x1000` → `dskbuf` @ `0x1002`).

**After fix:** `LTO=1 GC_SECTIONS=1` links and boots, but TEXT was **larger** than LTO-only (~242 192 vs ~238 102 on one measurement). Keep `GC_SECTIONS` off by default; LTO already does the useful DCE.

## Build

```bash
tools/build-st-gcc15.sh clean MINTELF=1 FASTCALL=1 256
# LTO defaults on for mintelf 256; equivalent explicit:
# make clean MINTELF=1 FASTCALL=1 LTO=1 256
```

Smoke: `tools/hatari-run.sh 800 -- --machine ste --tos-res sthigh --monitor mono`  
Check: `make check` / `tools/checkrom.sh -b obj/baseline-256-pre-size.map emutos.img emutos.map`
