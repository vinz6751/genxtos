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
#define  M_CVIS         0x0004  /* cursor visibility    0:invisible 1:visible */

/*
 * The visibility flag is also used as a semaphore to prevent
 * the interrupt-driven cursor blink logic from colliding with
 * escape function/sequence cursor drawing activity.
 */

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
extern WORD sav_cur_x;          /* saved cursor cell x */
extern WORD sav_cur_y;          /* saved cursor cell y */

/* Prototypes */
void conout_init(const Fonthead *font);
void conout_ascii_out(int);
void conout_move_cursor(int x, int y);
void conout_blank_out (int, int, int, int);
void conout_invert_cell(int, int);
void conout_scroll_up(UWORD top_line);
void conout_scroll_down(UWORD start_line);
void conout_paint_cursor(void);
void conout_unpaint_cursor(void);
void conout_blink_cursor(void);

typedef struct {
    void (*init)(const Fonthead *font);
    void (*blank_out)(int, int, int, int);
    void (*neg_cell)(CHAR_ADDR cell);
    void (*next_cell)(void);
    void (*cursor_moved)(void);
    void (*scroll_up)(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count);
    void (*scroll_down)(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line);
    int (*get_char_source)(unsigned char c, CHAR_ADDR *src);
    CHAR_ADDR (*cell_addr)(UWORD x, UWORD y);
    void (*cell_xfer)(const CHAR_ADDR src, CHAR_ADDR dst);
    void (*con_paint_cursor)(void);
    void (*unpaint_cursor)(void);
    void (*blink_cursor)(void); /* If null, a fallback is used */
} CONOUT_DRIVER;

#endif