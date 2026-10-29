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
| `aes/gemshlib.c` | `dsptch()` then `rlr = pd_index(0)` before desktop; skip `sh_show()` |
| `aes/geminit.c` | Restore `rlr = pd_index(0)` after p_msg init / after `iprocess(SCRENMGR)` |
| `aes/gemwmlib.c` | `wm_inprocess_reset()` only at desk start; desk uses real `wind_update` |
| `desk/gembind.c` | `menu_bar()` → `mn_bar()`; blocking `wind_update`/`evnt_*` via `gem_if_block` |
| `desk/deskmain.c` | `desk_begin_update`/`desk_end_update` → `wind_update` |

**Hatari:** ST/STe mode requires the **256K** ROM (`etos256us.img`); the
512K image is for Falcon mode only.

### 5b. Menu hover / ctlmgr orphan (`rlr` vs SCRENMGR)

Normal path: `ctlmgr` waits `MU_M1` on `gl_ctwait` (menu titles). `mchange()`
hands `gl_mowner` to `ctl_pd` when the pointer enters that rect; `post_mouse`
wakes SCRENMGR → `hctl_rect()` → `mn_do()`.

Under `CONF_AES_INPROCESS` with `totpds==2` (no DAs), the `p_msg.action=-1`
init loop left `rlr` on PD 1. `iprocess(SCRENMGR)` then aliased `rlr` and
`ctl_pd`. Later `rlr = pd_index(0)` in `sh_ldapp` orphaned SCRENMGR: desk
`MU_BUTTON` still worked (icons), but menu hover never ran.

Fix: restore `rlr = pd_index(0)` after PD init and after `iprocess`;
`dsptch()` once before desk so ctlmgr reaches `mwait` on `nrl`. Also use
real `wind_update` via `gem_if_block` (not `wm_inprocess_lock`/`unlock`,
which discarded `wind_spb` waiters).

### 5c. `gem_if_block` — Super before any `mwait`/`dsptch`

In-process EmuDesk runs in **user mode** on the desk USP. AES paths that
`mwait` → `dsptch` (which does `ori` to SR) need supervisor mode or Hatari
reports Privilege Violation.

`desk/gembind.c` `gem_if_block()` does `Super(0)` → `aes_inprocess_call` →
`SuperToUser`. Use it **only** for AES calls that can block/dispatch, e.g.:

- `wind_update`, `evnt_timer` / `evnt_multi`, `appl_exit`
- `form_do` / `form_alert` / `form_error`
- `graf_rubbox` / grow/shrink, `fsel_exinput`

Do **not** wrap every AES call — plain `gem_if` is enough for non-blocking
ops (`objc_draw`, `wind_get`, …). Expanding the block list causes boot hangs
when Super nesting or early desktop init interacts badly.

### 5d. `unsync` must not `dsptch` away from in-process desk

When the in-process desk releases `wind_spb` (`wind_update(END)` after
`menu_bar`, etc.), `unsync()` would normally `dsptch()`. That never returns
to synchronous `deskmain`.

Invariant (`aes/gemflag.c` + `aes/gemsuper.h`):

- `aes_inprocess_call` sets `aes_inprocess_pd = rlr` while the desk is inside
  a direct AES call.
- `unsync` skips `dsptch` **only** when `aes_inprocess_depth && rlr ==
  aes_inprocess_pd` (the desk itself is the releaser).
- If another PD (e.g. ctlmgr) releases while the desk is blocked in `mwait`
  inside `aes_inprocess_call`, `dsptch` is still required so the desk can
  resume.

### 5e. `crysbind` needs `noclone` under `-mfastcall`

`noinline` alone does **not** stop GCC IPA-SRA: the compiler still emitted
`crysbind.isra.*`, remapping the many pointer parameters. Symptom: in-process
`objc_draw` via `gem_if` drew only ROOT (`depth=0`) and could corrupt the
return path.

Fix: `__attribute__((noinline, noclone))` on `crysbind` in `aes/gemsuper.c`.
Restore the normal `gem_if` / `aes_inprocess_call` path for `objc_draw` (no
direct `ob_draw` bypass).

### 5f. Desk `Super(0)` SSP restore (`desk/deskstart.S`)

After desk code calls `Super(0)`, restore the AES SSP from `d0` before `rts`.
Wrong SSP after launch showed up as `PC=$2`.

### 6. Stack ABI keep-list (not in-process specific)

| Site | Rule |
|------|------|
| `aes/gemstart.S` `_dos_exec` | Stay stack / `ARGS_ON_STACK` (GFA Illegal at bad PC was this class) |
| `bios/cartridge_atari.S` | `typebit` already in `d0` when `FASTCALL_BUILD` |
| `bios/conout_atarifb.c` | `CONOUT_ATARI_DEF` matches driver-table ABI |
| `util/miscasm.S` `trap1_pexec` | `env` from `12(sp)` |

Prefer fixing the callee/prototype over bypassing the call.

### 7. GFA `joyvec` / `call_kbdvecs_bp` (separate from AES desk)

GFA soft-hang after `Bconout(4, $16)` waiting on a joyvec flag is **not**
explained by the AES/desk diffs. `bios/aciavecs_c.c` `call_kbdvecs_bp` must
pass the packet pointer in **a0 and on the stack** (same dual convention as
`call_kbdvecs_p`), with the last data byte in **d0.b**. Pushing the data byte
as a stack word broke `ARGS_ON_STACK` / `KBDVEC_P_fn` joyvec handlers.

---

## Verification

- `VDI video mode = 640x400 1-plane`
- No repeating `aes_run_rom_program: rc=-39`
- Desktop background + menu bar visible
- `nm obj/gemsuper.o` shows `_crysbind` **without** `.isra`
- Window interiors redraw after app quit (`unsync` + desk `WM_REDRAW`)
