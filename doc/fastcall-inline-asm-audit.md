# Fastcall inline-asm and stack-call audit

This document classifies every site that pushes arguments on the stack
before `jsr`/`trap`, for `-mfastcall` migration. User-facing sites must
keep their current behaviour.

## User-facing — frozen (do not change)

| File | Pattern | Purpose |
|------|---------|---------|
| `include/biosbind.h` | `move.w/l …,-(sp); trap #13` | User to OS BIOS |
| `include/xbiosbind.h` | `move.w/l …,-(sp); trap #14` | User to OS XBIOS |
| `include/bdosbind.h` | via `_trap1` in `util/miscasm.S` | User to OS GEMDOS |
| `bios/super.h` | `move.l fn,-(sp); trap #1` | Supexec / Super |
| `aes/gemoblib.c` | `call_usercode()` stack push | G_USERDEF user draw |
| `bios/aciavecs_c.c` | `call_kbdvecs_*()` stack push | KBDVECS user hooks |
| `foenix/trap_bindings.S` | reads `4(sp)+` | User trap #15 API |

## OS-internal — adapted for fastcall (FASTCALL_BUILD)

| File | Callee | Change |
|------|--------|--------|
| `bdos/rwa.S` | `osif(short *pw)` | Pass pointer in a0 |
| `aes/gemdosif.S` | `super()`, `b_click()`, `_forkq` | Register args |
| `bios/aciavecs.S` | `_kbd_int(WORD)` | Pass scancode in d0 |
| `vdi/linea_mouse_.S` | `_call_user_but` | Fastcall entry from C |
| All `.S` with `4(sp)` | C-callable stubs | Dual cdecl/fastcall entry |

## Convention-neutral — no change

| File | Notes |
|------|-------|
| `include/asm.h` | `regsafe_call()` — full register save |
| `include/intmath.h` | Register operands |
| `include/string.h` | Inline strcpy/cat |
| `bios/disk.c` | `dmaboot()` — TOS DMA boot register ABI |
| `tests/include/nat_feat.h` | Emulator test helper |

## VDI Line-A user hooks (register, preserve exactly)

| Vector | Args at `jsr` | Stack? |
|--------|--------------|--------|
| `user_but` | d0 + previous state on stack | Yes (undocumented) |
| `user_mot` | d0=x, d1=y | No |
| `user_cur` | d0=x, d1=y | No |
| `user_wheel` | d0=wheel#, d1=direction | No |

File: `vdi/linea_mouse_.S`
