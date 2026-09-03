/*
 * conout.h - lowlevel color model dependent screen handling routines
 *
 *
 * Copyright (C) 2004-2022 by Authors:
 *
 * Authors:
 *  MAD     Martin Doering
 *  VB      Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef CONOUT_H
#define CONOUT_H

#include "portab.h"
#include "fonthdr.h"

/* Defines for cursor */
#define  M_CFLASH       0x0001  /* cursor flash         0:disabled 1:enabled */
#define  M_CSTATE       0x0002  /* cursor flash state   0:off 1:on */
/*
 * The visibility flag is also used as a semaphore to prevent
 * the interrupt-driven cursor blink logic from colliding with
 * escape function/sequence cursor drawing activity.
 */
#define  M_CVIS         0x0004  /* cursor visibility    0:invisible 1:visible */

#define  M_CEOL         0x0008  /* end of line handling 0:overwrite 1:wrap */
#define  M_REVID        0x0010  /* reverse video        0:on        1:off */
#define  M_SVPOS        0x0020  /* position saved flag. 0:false,    1:true */
#define  M_CRIT         0x0040  /* reverse video        0:on        1:off */

/* Color related linea variables */

extern WORD v_col_bg;           /* current background color */
extern WORD v_col_fg;           /* current foreground color */

/* Represents the type used to address a character in the console's screen */
typedef union {
    UWORD  cellno;   /* This is for addressing cells in a text-cell-oriented drivers */
    UBYTE* pxaddr;   /* This is for addressing cells in pixel-oriented drivers */
} CHAR_ADDR;


/* Cursor related linea variables */

extern CHAR_ADDR v_cur_ad;      /* current cursor address */
extern WORD v_cur_of;           /* cursor offset */
extern UBYTE v_cur_tim;         /* cursor blink timer */

extern UBYTE v_period;
extern WORD disab_cnt;          /* disable depth count. (>0 means disabled) */

extern UBYTE v_stat_0;          /* video cell system status */
#define CURSOR_ENABLE        (v_stat_0 |= M_CVIS)
#define CURSOR_DISABLE       (v_stat_0 &= ~M_CVIS)
#define CURSOR_IS_ENABLED    (v_stat_0 & M_CVIS)
#define CURSOR_FLASH_ENABLED (v_stat_0 & M_CFLASH)
#define CURSOR_FLASH_ENABLE  (v_stat_0 |= M_CFLASH)
#define CURSOR_FLASH_DISABLE (v_stat_0 &= ~M_CFLASH)
#define CURSOR_FLASH_UP      (v_stat_0 |= M_CSTATE)
#define CURSOR_FLASH_IS_UP   (v_stat_0 & M_CSTATE)
#define CURSOR_FLASH_DOWN    (v_stat_0 &= ~M_CSTATE)

extern WORD sav_cur_x;          /* saved cursor cell x */
extern WORD sav_cur_y;          /* saved cursor cell y */

/* OS-internal dispatch wrappers; FP_CALLBACK driver methods stay cdecl. */
#if defined(FASTCALL_BUILD)
# define CONOUT_API FASTCALL_ATTR
#else
# define CONOUT_API ARGS_ON_STACK
#endif

void CONOUT_API conout_init(const Fonthead *font);
void CONOUT_API conout_ascii_out(int);
void CONOUT_API conout_enable_cursor(void);
void CONOUT_API conout_disable_cursor(void);
void CONOUT_API conout_move_cursor(int x, int y);
void CONOUT_API conout_blank_out (int, int, int, int);
void CONOUT_API conout_invert_cell(int, int);
void CONOUT_API conout_scroll_up(UWORD top_line);
void CONOUT_API conout_scroll_down(UWORD start_line);
void CONOUT_API conout_paint_cursor(void);
void CONOUT_API conout_unpaint_cursor(void);
void CONOUT_API conout_blink_cursor(void);

#if defined(FASTCALL_BUILD) \
    && !defined(MACHINE_A2560U) && !defined(MACHINE_A2560K) \
    && !defined(MACHINE_A2560M) && !defined(MACHINE_A2560X) \
    && !defined(MACHINE_GENX) && !defined(MACHINE_AMIGA)
# define CONOUT_ATARI_FASTPATH 1
void FASTCALL_ATTR conout_atari_blit_char(UBYTE *src, UBYTE *dst);
UBYTE * FASTCALL_ATTR conout_atari_cell_addr(UWORD x, UWORD y);
void FASTCALL_ATTR conout_atari_next_cell(void);
void FASTCALL_ATTR conout_atari_neg_cell(UBYTE *cell);
void FASTCALL_ATTR conout_atari_blank_out(int topx, int topy, int botx, int boty);
void FASTCALL_ATTR conout_atari_scroll_up(UBYTE *src, UBYTE *dst, ULONG count);
void FASTCALL_ATTR conout_atari_scroll_down(UBYTE *src, UBYTE *dst, LONG count, UWORD start_line);
#if defined(FASTCALL_BUILD)
void FASTCALL_ATTR conout_atari_init(const Fonthead *font);
#endif
void FASTCALL_ATTR conout_atari_paint_cursor(void);
#else
# define CONOUT_ATARI_FASTPATH 0
#endif

typedef struct {
    void (FP_CALLBACK *init)(const Fonthead *font);
    void (FP_CALLBACK *blank_out)(int, int, int, int);
    void (FP_CALLBACK *neg_cell)(CHAR_ADDR cell);
    void (FP_CALLBACK *next_cell)(void);
    void (FP_CALLBACK *cursor_moved)(void);
    void (FP_CALLBACK *scroll_up)(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count);
    void (FP_CALLBACK *scroll_down)(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line);
    int (FP_CALLBACK *get_char_source)(unsigned char c, CHAR_ADDR *src);
    CHAR_ADDR (FP_CALLBACK *cell_addr)(UWORD x, UWORD y);
    void (FP_CALLBACK *cell_xfer)(const CHAR_ADDR src, CHAR_ADDR dst);
    void (FP_CALLBACK *con_paint_cursor)(void);
    void (FP_CALLBACK *unpaint_cursor)(void);
    void (FP_CALLBACK *blink_cursor)(void); /* If null, a fallback is used */
} CONOUT_DRIVER;

#endif