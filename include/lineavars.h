/*
 * lineavars.h - name of linea graphics related variables
 *
 * Copyright (C) 2001-2020 by Authors:
 *
 * Authors:
 *  MAD   Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * Put in this file only the low-mem vars actually used by C code!
 */

#ifndef LINEAVARS_H
#define LINEAVARS_H

#include "portab.h"
#include "biosdefs.h"
#include "fonthdr.h"

/* Screen related variables */

/*
 * mouse cursor save area
 *
 * NOTE: the lineA version of this only has the first 64 ULONGs,
 * to handle a maximum of 4 video planes.  Writing into area[64]
 * and above when referencing the lineA version will overwrite
 * other lineA variables with unpredictable results.
 */
typedef struct {
        WORD    len;            /* height of saved form */
        UWORD   *addr;          /* screen address of saved form */
        UBYTE    stat;          /* save status */
        char    reserved;
        ULONG   area[8*16];     /* handle up to 8 video planes */
} MCS;

/* Mouse / sprite structure (16x16) */
typedef struct {
    WORD    xhot;
    WORD    yhot;
    WORD    planes;
    WORD    bg_col;
    WORD    fg_col;
    UWORD   maskdata[32];   /* mask & data are interleaved */
} MCDB;

/* defines for 'stat' above */
#define MCS_VALID   0x01        /* save area is valid */
#define MCS_LONGS   0x02        /* saved data is in longword format */

/* VDI local parameter block (VDIPB) */
extern WORD *CONTRL; /* +4   ptr to the CONTRL array */
extern WORD *INTIN;  /* +8   ptr to the INTIN array */
extern WORD *PTSIN;  /* +12  ptr to the PTSIN array */
extern WORD *INTOUT; /* +16  ptr to the INTOUT array */
extern WORD *PTSOUT; /* +20  ptr to the PTSOUT array */

/* Mouse management variables */
extern UBYTE vbl_must_draw_mouse;    /* non-zero means draw mouse form on vblank */
extern UBYTE mouse_flag;             /* non-zero while mouse cursor is being modified */
extern MCS   mouse_cursor_save;      /* in linea variable area */
extern MCS   ext_mouse_cursor_save;  /* use for v_planes > 4 */
extern WORD  HIDE_CNT;               /* number of levels the mouse is hidden */
extern MCDB  mouse_cdb;              /* storage for mouse sprite */
extern WORD  GCURX;                  /* mouse X position */
extern WORD  GCURY;                  /* mouse Y position */
extern WORD  MOUSE_BT;               /* mouse button state */
extern UBYTE cur_ms_stat;            /* current mouse status */
extern WORD  newx;                   /* new mouse x position when setting the mouse position manually */
extern WORD  newy;                   /* new mouse y position when setting the mouse position manually */
extern void  (*user_but)(void);      /* user button vector */
extern void  (*user_cur)(void);      /* user cursor vector, called when the mouse has moved */
extern void  (*user_mot)(void);      /* user motion vector */

extern WORD TERM_CH;  /* pressed key, ASCII + scancode */

/* The following line-A variables contain the VDI color palette entries.
 * REQ_COL contains the first 16 entries; req_col2 contains entries
 * 16-255 (only applicable for 8-plane resolutions).  Note that the
 * location of req_col2 is not documented by Atari, but is derived from
 * disassembly of TOS ROMs, and source code for MagiC's VDI. */
extern WORD REQ_COL[16][3];
extern WORD req_col2[240][3];

#define line_a_vars (void *)&v_planes   /* start of linea variables */
extern UWORD v_planes;          /* count of color planes */
extern UWORD v_lin_wr;          /* line wrap : bytes per line */
extern UWORD v_cel_ht;          /* cell height (width is 8) */
extern UWORD v_cel_mx;          /* number of columns - 1 */
extern UWORD v_cel_my;          /* number of rows - 1 */
extern UWORD v_cel_wr;          /* length (in bytes) of a line of characters */
extern UWORD v_cur_cx;          /* current cursor column */
extern UWORD v_cur_cy;          /* current cursor row */
extern UWORD V_REZ_HZ;          /* screen horizontal resolution */
extern UWORD V_REZ_VT;          /* screen vertical resolution */
extern UWORD BYTES_LIN;         /* width of line in bytes */
extern WORD  DEV_TAB[];         /* intout array for open workstation */

extern UWORD linea_max_x;       /* max X coordinate (= V_REZ_HZ - 1) */
extern UWORD linea_max_y;       /* max Y coordinate (= V_REZ_VT -1) */

extern UBYTE v_planes_shift;   /* pixel to address helper */

/* Line-drawing related variables */
extern WORD X1, Y1, X2, Y2;     /* coordinates for end points */
extern WORD WRT_MODE;           /* write mode */
extern WORD COLBIT0, COLBIT1, COLBIT2, COLBIT3; /* colour bit values for planes 0-3 */
extern WORD CLIP;               /* clipping flag */
extern WORD XMINCL, XMAXCL, YMINCL, YMAXCL; /* clipping rectangle */
extern UWORD *PATPTR;           /* fill pattern pointer */
extern UWORD PATMSK;            /* pattern mask */

/* text-blit related variables */
extern WORD XDDA;               /* accumulator for x DDA        */
extern UWORD DDAINC;            /* the fraction to be added to the DDA */
extern WORD SCALDIR;            /* 0 if scale down, 1 if enlarge */
extern WORD MONO;               /* True if current font monospaced */
extern WORD SOURCEX, SOURCEY;   /* upper left of character in font file */
extern WORD DESTX, DESTY;       /* upper left of destination on screen  */
extern UWORD DELX, DELY;        /* width and height of character    */
extern const UWORD *FBASE;      /* pointer to font data         */
extern WORD FWIDTH;             /* offset,segment and form width of font */
extern WORD STYLE;              /* Requested text special effects */
extern WORD LITEMASK, SKEWMASK; /* special effects          */
extern WORD WEIGHT;             /* special effects          */
extern WORD ROFF, LOFF;         /* skew above and below baseline    */
extern WORD SCALE;              /* True if current font scaled */
extern WORD CHUP;               /* Text baseline vector */
extern WORD TEXTFG;             /* text foreground colour */
extern WORD *SCRTCHP;           /* Pointer to text scratch buffer */
extern WORD SCRPT2;             /* Offset to large text buffer */

/* font-specific variables */
extern const Fonthead *CUR_FONT;/* most recently used font */
extern const UWORD *v_fnt_ad;   /* address of current monospace font */
extern const UWORD *v_off_ad;   /* address of font offset table */
extern UWORD v_fnt_nd;          /* ascii code of last cell in font */
extern UWORD v_fnt_st;          /* ascii code of first cell in font */
extern UWORD v_fnt_wr;          /* font cell wrap */
extern const Fonthead *def_font;/* default font of open workstation */

/*
 * font_ring is an array of four pointers, each of which points to
 * a linked list of font headers.  usage is as follows:
 *  font_ring[0]    system fonts that are available in all resolutions;
 *                  this is currently just the 6x6 font
 *  font_ring[1]    resolution-dependent system fonts; currently
 *                  the 8x8 and 8x16 fonts
 *  font_ring[2]    fonts loaded by GDOS; initially an empty list
 *  font_ring[3]    always NULL, marking the end of the list of lists
 */
extern const Fonthead *font_ring[4];/* all available fonts */
extern WORD font_count;             /* number of different font ids in font_ring[] */

/* timer-related vectors */
extern ETV_TIMER_T tim_addr;  /* timer interrupt vector */
extern ETV_TIMER_T tim_chain; /* timer interrupt vector save */

#endif /* LINEAVARS_H */
