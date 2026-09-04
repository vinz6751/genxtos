# FASTCALL small-ROM AES boot — fixes and rationale

Reference for **256K / 512K padded ROM** builds with `FASTCALL=1` reaching
the EmuDesk desktop under Hatari STE high-res mono.

**Primary test target:** `make 256` → `etos256us.img` (see
`.cursor/skills/genxtos-build-run/SKILL.md`).

## Terminology: ROM size vs ST-RAM

| Name | Meaning |
|------|---------|
| **`make 256`** / `etos256us.img` | **256 KiB ROM image** (`-DTARGET_256`, `SMALL_OPTFLAGS`) |
| **`make 512`** / `etos512us.img` | **512 KiB ROM image** (`-DTARGET_512`) |
| **initinfo “ST-RAM”** | **Physical RAM** at boot → `phystop` (`bios/memory.S`) |
| **`CONF_AES_INPROCESS`** | Set for `TARGET_256` and `TARGET_512` — in-process accdesk/desktop |

initinfo ST-RAM is **not** tied to ROM size. Use Hatari `--memsize 0` for
512 KiB RAM if emulating a stock ST.

The AES `-39` failure was **OS MD pool / subprocess overhead** on small ROM
builds, not wrong RAM detection.

---

## Build and run (256K — default)

```sh
make clean MINTELF=1 FASTCALL=1 NATFEAT_DEBUG=1 256
TOS=$PWD/etos256us.img tools/hatari-run.sh 8000 -- \
  --machine ste --tos-res sthigh --monitor mono
```

512K variant: replace `256` / `etos256us.img` with `512` / `etos512us.img`.

Related: `doc/fastcall-inline-asm-audit.md`.

---

## Root causes and fixes

### 1. FASTCALL `trap1_pexec` (`util/miscasm.S`)

Fourth argument (`env`) must come from caller stack at `12(sp)`, not a2.

### 1b. Variadic functions use `VARARGS` (cdecl)

Under `-mfastcall`, `va_start` is broken for fastcall functions. Marked
with `VARARGS` / `ARGS_ON_STACK`: `kprintf`, `cprintf`, `kcprintf`,
`dopanic`, `panic`, `sprintf`.

### 1c. GEMDOS `osif` pointer reassembly (`bdos/bdosmain.c`)

Trap parameters arrive as 16-bit words in `pw[]`. The generic `ww`/`www`
indirect calls no longer match `GEMDOS_API` callees under `-mfastcall`
(with `-mshort`). Fix: `PW_LONG()` reassembly in cases 2–5; explicit
dispatch for `Fsetdta` (0x1a) and `Fsfirst` (0x4e). `ixsfirst` is also
`GEMDOS_API` so `xsfirst` → `ixsfirst` stays stack-based.

### 2. OS MD pool (`bdos/osmem.c`, `bdos/proc.c`)

- `NUM_OSM_BLOCKS` **200** when `TARGET_256` or `TARGET_512`
- Static `null_env` for `PE_BASEPAGEFLAGS` when `env==NULL`

### 3. In-process accdesk + desktop (`CONF_AES_INPROCESS`)

| File | Change |
|------|--------|
| `include/config.h` | `CONF_AES_INPROCESS` for `TARGET_256` \|\| `TARGET_512` |
| `aes/geminit.c` | `run_accs_and_desktop()` instead of `aes_run_rom_program(accdesk_start)` |
| `aes/gemshlib.c` | `desk_run_inprocess()` instead of `aes_run_rom_program(deskstart)` |
| `desk/deskstart.S` | `desk_run_inprocess()` — user stack + user mode before `deskmain()` |
| `aes/geminit.c` | `fm_init` / `mn_init` / SCRENMGR deferred into `run_accs_and_desktop()` |

### 4. In-process desktop must use `deskstart` stack (`desk/deskstart.S`)

Calling `deskmain()` directly from the AES shell leaves the CPU on the
AES supervisor stack with no valid USP. `desk_run_inprocess()` mirrors
the normal `deskstart` path: switch to `deskustack`, user mode, then
`deskmain()`.

### 5. In-process AES must not use trap #2 + `dsptch()` (`aes/gemsuper.c`, `desk/gembind.c`)

While `sh_ldapp()` runs EmuDesk synchronously, AES trap #2 ends in
`dsptch()` which does not return to the caller. Symptom: init hangs after
the first `appl_init()` or `wind_update()`, and `menu_bar()` never draws.

Fix: `aes_inprocess_call()` — same as `xif()`/crysbind but without
`dsptch()`. All EmuDesk `gembind.c` calls use this when
`CONF_AES_INPROCESS`.

Also required:

| File | Change |
|------|--------|
| `aes/gemshlib.c` | `rlr = pd_index(0)` before desktop; skip `sh_show()` |
| `aes/gemwmlib.c` | `wm_inprocess_{reset,lock,unlock}()` instead of blocking `wind_update()` |
| `desk/gembind.c` | `menu_bar()` calls `mn_bar()` directly (no trap) |
| `desk/deskmain.c` | `desk_begin_update`/`desk_end_update` macros for in-process |

**Hatari:** ST/STe mode requires the **256K** ROM (`etos256us.img`); the
512K image is for Falcon mode only.

---

## Verification

- `VDI video mode = 640x400 1-plane`
- No repeating `aes_run_rom_program: rc=-39`
- Desktop background + menu bar visible
