/*
 * conout.c - character screen manipulation routines
 *
 *
 * Copyright (C) 2004 by Authors (see below)
 * Copyright (C) 2016-2025 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  VB      Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * NOTE: the code currently assumes that the font width is 8 bits.
 * If we ever add a 16x32 font, the code will need changing!
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560_bios.h"

/* Driver in use for outputing text */
static CONOUT_DRIVER * conout;

#if defined(FASTCALL_BUILD)
/*
 * gcc mishandles forwarding of stack arguments to an indirect FP_CALLBACK.
 * Push explicit operands instead of reading 4(sp) from asm (broken if the
 * compiler adds a prologue before the asm block).
 */
static long char_addr_arg(CHAR_ADDR a)
{
    long v;

    memcpy(&v, &a, sizeof(v));
    return v;
}

static CHAR_ADDR char_addr_ret(long v)
{
    CHAR_ADDR a;

    memcpy(&a, &v, sizeof(a));
    return a;
}

static void fpcall_iiii(void (FP_CALLBACK *fn)(int, int, int, int),
                        int a, int b, int c, int d)
{
    __asm__ volatile(
        "move.l %4,a0\n\t"
        "move.l %3,sp@-\n\t"
        "move.l %2,sp@-\n\t"
        "move.l %1,sp@-\n\t"
        "move.l %0,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 16(sp),sp"
        :
        : "g"(a), "g"(b), "g"(c), "g"(d), "a"(fn)
        : "memory");
}

static CHAR_ADDR fpcall_uu(CHAR_ADDR (FP_CALLBACK *fn)(UWORD, UWORD),
                           UWORD x, UWORD y)
{
    long rc;

    __asm__ volatile(
        "move.l %3,a0\n\t"
        "move.l %2,sp@-\n\t"
        "move.l %1,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 8(sp),sp\n\t"
        "move.l d0,%0"
        : "=g"(rc)
        : "g"((long)x), "g"((long)y), "a"(fn)
        : "d0", "memory");
    return char_addr_ret(rc);
}

static int fpcall_ic(int (FP_CALLBACK *fn)(unsigned char, CHAR_ADDR *),
                     unsigned char c, CHAR_ADDR *src)
{
    int rc;

    __asm__ volatile(
        "move.l %3,a0\n\t"
        "move.l %2,sp@-\n\t"
        "move.l %1,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 8(sp),sp\n\t"
        "move.l d0,%0"
        : "=g"(rc)
        : "g"((long)c), "g"(src), "a"(fn)
        : "d0", "memory");
    return rc;
}

static void fpcall_CC(void (FP_CALLBACK *fn)(CHAR_ADDR, CHAR_ADDR),
                      CHAR_ADDR src, CHAR_ADDR dst)
{
    long s = char_addr_arg(src);
    long d = char_addr_arg(dst);

    __asm__ volatile(
        "move.l %2,a0\n\t"
        "move.l %1,sp@-\n\t"
        "move.l %0,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 8(sp),sp"
        :
        : "g"(s), "g"(d), "a"(fn)
        : "d0", "memory");
}

static void fpcall_C(void (FP_CALLBACK *fn)(CHAR_ADDR), CHAR_ADDR cell)
{
    long v = char_addr_arg(cell);

    __asm__ volatile(
        "move.l %1,a0\n\t"
        "move.l %0,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 4(sp),sp"
        :
        : "g"(v), "a"(fn)
        : "d0", "memory");
}

static void fpcall_CCC(void (FP_CALLBACK *fn)(const CHAR_ADDR, CHAR_ADDR, ULONG),
                       CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    long s = char_addr_arg(src);
    long d = char_addr_arg(dst);

    __asm__ volatile(
        "move.l %3,a0\n\t"
        "move.l %2,sp@-\n\t"
        "move.l %1,sp@-\n\t"
        "move.l %0,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 12(sp),sp"
        :
        : "g"(s), "g"(d), "g"(count), "a"(fn)
        : "d0", "memory");
}

static void fpcall_CCCi(void (FP_CALLBACK *fn)(const CHAR_ADDR, CHAR_ADDR, LONG, UWORD),
                        CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    long s = char_addr_arg(src);
    long d = char_addr_arg(dst);

    __asm__ volatile(
        "move.l %4,a0\n\t"
        "move.l %3,sp@-\n\t"
        "move.l %2,sp@-\n\t"
        "move.l %1,sp@-\n\t"
        "move.l %0,sp@-\n\t"
        "jsr a0@\n\t"
        "lea 16(sp),sp"
        :
        : "g"(s), "g"(d), "g"(count), "g"((long)start_line), "a"(fn)
        : "d0", "memory");
}

static void fpcall_v(void (FP_CALLBACK *fn)(void))
{
    __asm__ volatile(
        "move.l %0,a0\n\t"
        "jsr a0@"
        :
        : "m"(fn)
        : "d0", "a0", "memory");
}
#endif

#if defined(MACHINE_AMIGA)
extern const CONOUT_DRIVER conout_amiga;
#elif !(defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX))
extern const CONOUT_DRIVER conout_atarifb;
#endif


void CONOUT_API conout_init(const Fonthead *font)
{
    KDEBUG(("conout_init: v_cel_mx: %d v_cel_my:%d\n", v_cel_mx, v_cel_my));

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    conout = a2560_bios_get_conout();
#elif defined(MACHINE_AMIGA)
    conout = (CONOUT_DRIVER *)&conout_amiga;
#else
    conout = (CONOUT_DRIVER *)&conout_atarifb;
#endif
    KDEBUG(("conout_init calling init of driver\n"));

#if CONOUT_ATARI_FASTPATH
    conout_atari_init(font);
#elif defined(FASTCALL_BUILD)
    {
        void (FP_CALLBACK *initfn)(const Fonthead *) = conout->init;

        __asm__ volatile(
            "move.l %0,a0\n\t"
            "move.l %1,sp@-\n\t"
            "jsr a0@\n\t"
            "lea 4(sp),sp"
            :
            : "a"(initfn), "m"(font)
            : "a0", "memory");
    }
#else
    conout->init(font);
#endif

    KDEBUG(("conout_init exiting\n"));
}


/*
 * cell_addr - convert cell X,Y to a screen address.
 *
 * convert cell X,Y to a screen address. also clip cartesian coordinates
 * to the limits of the current screen.
 */

static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    /* check bounds against screen limits */
    if (x > v_cel_mx)
        x = v_cel_mx;           /* clipped x */

    if (y > v_cel_my)
        y = v_cel_my;           /* clipped y */

#if CONOUT_ATARI_FASTPATH
    {
        CHAR_ADDR a;

        a.pxaddr = conout_atari_cell_addr(x, y);
        return a;
    }
#elif defined(FASTCALL_BUILD)
    return fpcall_uu(conout->cell_addr, x, y);
#else
    return conout->cell_addr(x,y);
#endif
}


/*
 * neg_cell - negates
 *
 * This routine negates the contents of an arbitrarily-tall byte-wide cell
 * composed of 1 to 8 Atari-style bit-planes, or of Falcon-style 16-bit
 * graphics.
 * Cursor display can be accomplished via this procedure.  Since a second
 * negation restores the original cell condition, there is no need to save
 * the contents beneath the cursor block.
 *
 * input:
 *  cell    points to destination (1st plane, top of block)
 */

static void neg_cell(CHAR_ADDR cell)
{
    v_stat_0 |= M_CRIT;                 /* start of critical section. */

#if CONOUT_ATARI_FASTPATH
    conout_atari_neg_cell(cell.pxaddr);
#elif defined(FASTCALL_BUILD)
    fpcall_C(conout->neg_cell, cell);
#else
    conout->neg_cell(cell);
#endif

    v_stat_0 &= ~M_CRIT;                /* end of critical section. */
}


/*
 * next_cell - Return the next cell address.
 *
 * sets next cell address given the current position and screen constraints
 *
 * returns:
 *     false - no wrap condition exists
 *     true  - CR LF required (position has not been updated)
 */

static BOOL next_cell(void)
{
    /* check bounds against screen limits */
    if (v_cur_cx == v_cel_mx) {         /* increment cell ptr */
        if (!(v_stat_0 & M_CEOL)) {
            /* overwrite in effect */
            return 0;                   /* no wrap condition exists */
                                        /* don't change cell parameters */
        }

        /* call carriage return routine */
        /* call line feed routine */
        return 1;                       /* indicate that CR LF is required */
    }

    v_cur_cx += 1;                      /* next cell to right */

#if CONOUT_ATARI_FASTPATH
    conout_atari_next_cell();
#elif defined(FASTCALL_BUILD)
    fpcall_v(conout->next_cell);
#else
    conout->next_cell();
#endif

    return 0;                           /* indicate no wrap needed */
}


/*
 * conout_invert_cell - negates the cell's bits
 *
 * This routine negates the contents of an arbitrarily-tall byte-wide cell
 * composed of an arbitrary number of (Atari-style) bit-planes.
 *
 * Wrapper for neg_cell().
 *
 * in:
 * x - cell X coordinate
 * y - cell Y coordinate
 */

void CONOUT_API conout_invert_cell(int x, int y)
{
    /* fetch x and y coords and invert cursor. */
#if CONOUT_ATARI_FASTPATH
    conout_atari_neg_cell(cell_addr(x, y).pxaddr);
#elif defined(FASTCALL_BUILD)
    fpcall_C(conout->neg_cell, cell_addr(x, y));
#else
    conout->neg_cell(cell_addr(x, y));
#endif
}


/*
 * conout_move_cursor - move the cursor.
 *
 * move the cursor and update global parameters
 * erase the old cursor (if necessary) and draw new cursor (if necessary)
 *
 * in:
 * d0.w    new cell X coordinate
 * d1.w    new cell Y coordinate
 */

void CONOUT_API conout_move_cursor(int x, int y)
{
    /* update cell position */

    /* clamp x,y to valid ranges */
    if (x < 0)
        x = 0;
    else if (x > v_cel_mx)
        x = v_cel_mx;

    if (y < 0)
        y = 0;
    else if (y > v_cel_my)
        y = v_cel_my;

    v_cur_cx = x;
    v_cur_cy = y;
#if 1
    if (conout->cursor_moved) {
#if defined(FASTCALL_BUILD)
        fpcall_v(conout->cursor_moved);
#else
        conout->cursor_moved();
#endif
    }

    if (!CURSOR_IS_ENABLED) {
        /* not visible */
        v_cur_ad = cell_addr(x, y);             /* just set new coordinates */
        return;
    }

    /* is cursor flashing? */
    if (CURSOR_FLASH_ENABLED) {
        CURSOR_DISABLE;  /* yes, make invisible...semaphore. */

        /* if cursor presently displayed, display it when it moves */
        if (!CURSOR_FLASH_IS_UP) {
            /* not displayed */
            v_cur_ad = cell_addr(x, y);         /* just set new coordinates */

            /* show the cursor when it moves */
            conout_paint_cursor();
            CURSOR_FLASH_UP;

            v_cur_tim = v_period;          /* reset the timer. */

            CURSOR_ENABLE;                 /* end of critical section. */
            return;
        }
    }

    /* move the cursor after all special checks failed */
    conout_unpaint_cursor();    /* erase from current location */
    v_cur_ad = cell_addr(x, y); /* set new location */
    conout_paint_cursor();      /* paintthere */
    v_cur_tim = v_period;       /* reset the timer so the cursor doesn't flash when it moves */

    CURSOR_ENABLE;              /* end of critical section. */
#else
    /* is cursor visible? */
    if (!CURSOR_IS_ENABLED) {
        /* not visible */
        v_cur_ad = cell_addr(x, y);             /* just set new coordinates */
        return;                                 /* and quit */
    }

    /* is cursor flashing? */
    if (CURSOR_FLASH_ENABLED) {
        CURSOR_DISABLE;                    /* yes, make invisible...semaphore. */

        /* is cursor presently displayed ? */
        if (!CURSOR_FLASH_IS_UP) {
            /* not displayed */
            v_cur_ad = cell_addr(x, y);         /* just set new coordinates */

            /* show the cursor when it moves */
            neg_cell(v_cur_ad);                 /* complement cursor. */
            CURSOR_FLASH_UP;
            v_cur_tim = v_period;               /* reset the timer. */

            CURSOR_ENABLE;                 /* end of critical section. */
            return;
        }
    }

    /* move the cursor after all special checks failed */
    neg_cell(v_cur_ad);                         /* erase present cursor */

    v_cur_ad = cell_addr(x, y);                 /* fetch x and y coords. */
    neg_cell(v_cur_ad);                         /* complement cursor. */

    /* do not flash the cursor when it moves */
    v_cur_tim = v_period;                       /* reset the timer. */

    CURSOR_ENABLE;                         /* end of critical section. */

#endif
}



/*
 * conout_ascii_out - prints an ascii character on the screen
 *
 * in:
 *
 * ch.w      ascii code for character
 */

void CONOUT_API conout_ascii_out(int ch)
{
    CHAR_ADDR src;
    CHAR_ADDR dst;
    BOOL visible;                       /* was the cursor visible? */

#if defined(FASTCALL_BUILD)
#if CONOUT_ATARI_FASTPATH
    src.pxaddr = char_addr((unsigned char)ch);
    if (!src.pxaddr) {
        KDEBUG(("conout_ascii_out: no char source"));
        return;
    }
#else
    if (!fpcall_ic(conout->get_char_source, (unsigned char)ch, &src)) {
        KDEBUG(("conout_ascii_out: no char source"));
        return;
    }
#endif
#else
    if (!conout->get_char_source((unsigned char)ch, &src)) {
        KDEBUG(("conout_ascii_out: no char source"));
        return;
    }
#endif

    dst = v_cur_ad;                     /* a1 -> get destination */

    visible = CURSOR_IS_ENABLED;        /* test visibility bit */
    if (visible)
        CURSOR_DISABLE;            /* start of critical section */

    /* put the cell out (this covers the cursor) */
#if defined(FASTCALL_BUILD)
#if CONOUT_ATARI_FASTPATH
    conout_atari_blit_char(src.pxaddr, dst.pxaddr);
#else
    fpcall_CC(conout->cell_xfer, src, dst);
#endif
#else
    conout->cell_xfer(src, dst);
#endif

    /* advance the cursor and update cursor address and coordinates */
    if (next_cell()) {
        /* CRLF */
        CHAR_ADDR cell;
        UWORD y = v_cur_cy;

        /* perform cell carriage return. */        
        v_cur_cx = 0;                   /* set X to first cell in line */

        /* perform cell line feed. */
        if (y < v_cel_my) {
            cell = cell_addr(0, y + 1);
            v_cur_cy = y + 1;
        }
        else {
            cell = cell_addr(0, y);
            conout_scroll_up(0);
        }
        v_cur_ad = cell;                /* update cursor address */   
    }

    if (conout->cursor_moved) {
#if defined(FASTCALL_BUILD)
        fpcall_v(conout->cursor_moved);
#else
        conout->cursor_moved();
#endif
    }

    /* if visible */
    if (visible) {
#if 1
        conout_paint_cursor();          /* display cursor. */
#else
        neg_cell(v_cur_ad);             /* display cursor. */
#endif        
        CURSOR_FLASH_UP;                /* set state flag (cursor on). */
        CURSOR_ENABLE;                  /* end of critical section. */

        /* do not flash the cursor when it moves */
        if (CURSOR_FLASH_ENABLED) {
            v_cur_tim = v_period;       /* reset the timer. */
        }
    }
}


#if CONOUT_ATARI_FASTPATH
/*
 * gcc m68k -mfastcall passes the 4th int in arg on the stack at 4(sp).
 * Tail-call the fastcall implementation with d0-d3 already set up.
 */
__attribute__((naked))
void CONOUT_API conout_blank_out(int topx, int topy, int botx, int boty)
{
    __asm__ volatile(
        "move.w 4(sp),d3\n\t"
        "jra _conout_atari_blank_out"
        :
        :
        : "memory");
}
#else
void CONOUT_API conout_blank_out(int topx, int topy, int botx, int boty)
{
#if defined(FASTCALL_BUILD)
    fpcall_iiii(conout->blank_out, topx, topy, botx, boty);
#else
    conout->blank_out(topx, topy, botx, boty);
#endif
}
#endif


/*
 * conout_scroll_up - Scroll upwards
 *
 *
 * Scroll copies a source region as wide as the screen to an overlapping
 * destination region on a one cell-height offset basis.  Two entry points
 * are provided:  Partial-lower scroll-up, partial-lower scroll-down.
 * Partial-lower screen operations require the cell y # indicating the
 * top line where scrolling will take place.
 *
 * After the copy is performed, any non-overlapping area of the previous
 * source region is "erased" by calling conout_blank_out which fills the area
 * with the background color.
 *
 * in:
 *   top_line - cell y of cell line to be used as top line in scroll
 */

void CONOUT_API conout_scroll_up(UWORD top_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

    dst = cell_addr(0, top_line);
    src = cell_addr(0, top_line + 1);
    count = (ULONG)v_cel_wr * (v_cel_my - top_line);
#if CONOUT_ATARI_FASTPATH
    conout_atari_scroll_up(src.pxaddr, dst.pxaddr, count);
#elif defined(FASTCALL_BUILD)
    fpcall_CCC(conout->scroll_up, src, dst, count);
#else
    conout->scroll_up(src, dst, count);
#endif
}


/*
 * conout_scroll_down - Scroll (partially) downwards
 */

void CONOUT_API conout_scroll_down(UWORD start_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

    src = cell_addr(0, start_line);
    dst = cell_addr(0, start_line + 1);
    count = (ULONG)v_cel_wr * (v_cel_my - start_line);

#if CONOUT_ATARI_FASTPATH
    conout_atari_scroll_down(src.pxaddr, dst.pxaddr, count, start_line);
#elif defined(FASTCALL_BUILD)
    fpcall_CCCi(conout->scroll_down, src, dst, count, start_line);
#else
    conout->scroll_down(src, dst, count, start_line);
#endif
}


void CONOUT_API conout_paint_cursor(void)
{
#if CONOUT_ATARI_FASTPATH
    conout_atari_paint_cursor();
#elif defined(FASTCALL_BUILD)
    fpcall_v(conout->con_paint_cursor);
#else
    conout->con_paint_cursor();
#endif
}


void CONOUT_API conout_unpaint_cursor(void)
{
#if CONOUT_ATARI_FASTPATH
    conout_atari_paint_cursor();
#elif defined(FASTCALL_BUILD)
    fpcall_v(conout->unpaint_cursor);
#else
    conout->unpaint_cursor();
#endif
}


void CONOUT_API conout_enable_cursor(void)
{
    CURSOR_ENABLE;
    conout_paint_cursor();

    /* see if flashing is enabled */
    if (CURSOR_FLASH_ENABLED)
    {
        CURSOR_FLASH_UP;                     /* set cursor on */

        /* do not flash the cursor when it moves */
        v_cur_tim = v_period;                /* reset the timer */
    }
#if 0  // Previous EmuTOS code
    conout_invert_cell(v_cur_cx, v_cur_cy);  /* complement cursor */
    CURSOR_ENABLE;                           /* set visibility bit */

    /* see if flashing is enabled */
    if (CURSOR_FLASH_ENABLED) {
        CURSOR_FLASH_UP;                     /* set cursor on */

        /* do not flash the cursor when it moves */
        v_cur_tim = v_period;                   /* reset the timer */
    }
#endif
}


void CONOUT_API conout_disable_cursor(void)
{
    if (!CURSOR_IS_ENABLED)
        return;                         /* if already invisible, just return */
    
    CURSOR_DISABLE;                 /* make invisible! */

    /* see, if flashing is disabled */
    if (!CURSOR_FLASH_ENABLED) {
        conout_unpaint_cursor();
    }
    /* see, if cursor is on or off */
    else if (CURSOR_FLASH_IS_UP) {
        CURSOR_FLASH_DOWN;    /* cursor off? */
        conout_unpaint_cursor();;
    }
#if 0 // Previous EmuTOS code
    /* test and clear the visible state bit */
    if (!CURSOR_ENABLED)
        return;                         /* if already invisible, just return */

    CURSOR_DISABLE;                     /* make invisible! */

    /* see, if flashing is disabled */
    if (!CURSOR_FLASH_ENABLED) {
        conout_invert_cell(v_cur_cx, v_cur_cy);
    }
    /* see, if cursor is on or off */
    else if (CURSOR_FLASH_IS_UP) {
        CURSOR_FLASH_DOWN;    /* cursor off? */
        conout_invert_cell(v_cur_cx, v_cur_cy);
    }
#endif
}


/*
 * blink - cursor blink interrupt routine
 *
 * This used to be in vt52.c where it arguably belong but since the behaviour depends
 * on the underlying conout driver.
 */
void CONOUT_API conout_blink_cursor(void)
{
    if (conout->blink_cursor)
    {
#if CONOUT_ATARI_FASTPATH
        /* Atari driver has no custom blink handler */
#elif defined(FASTCALL_BUILD)
        fpcall_v(conout->blink_cursor);
#else
        conout->blink_cursor();
#endif
    }
    else if (CURSOR_IS_ENABLED && CURSOR_FLASH_ENABLE && (--v_cur_tim) == 0 )
    {
        v_cur_tim = v_period; /* reset timer */
        CURSOR_FLASH_IS_UP ? conout_unpaint_cursor() : conout_paint_cursor(); /* toggle state*/ 
    }
}
