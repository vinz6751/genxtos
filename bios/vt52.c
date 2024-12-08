/*
 * vt52.c - vt52 like screen handling routines
 *
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 * Copyright (C) 2004 Martin Doering
 *
 * Authors:
 *  MAD     Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* The VT-52 module is responsible for console output. This includes:
 * - Book keeping of text cursor location (automatic advance, wrap,
 *   scroll, etc.).
 * - Displaying of characters using the conout module.
 * - Manage text cursor visibility / blinking.
 */

// #define ENABLE_KDEBUG

#include "emutos.h"
#include "lineavars.h"
#include "font.h"
#include "tosvars.h"            /* for save_row */
#include "sound.h"              /* for bell() */
#include "string.h"
#include "conout.h"
#include "vt52.h"
#include "bios.h"
#include "a2560u_bios.h"

#if CONF_SERIAL_CONSOLE_ANSI
/* We disable cursor home commands because it is more convenient */
# define SERIAL_CONSOLE_HONOR_HOME 0
#endif

/* converts from escape sequence value to column or row number */
#define POSITION_BIAS   32

/*
 * internal prototypes
 */
static void nop(void);
static void cursor_up(void);
static void cursor_down_impl(void);
static void cursor_down(void);
static void cursor_left_impl(void);
static void cursor_left(void);
static void cursor_right(void);
static void clear_and_home(void);
static void cursor_home(void);
static void reverse_linefeed(void);
static void erase_to_eos(void);
static void erase_to_eol_impl(void);
static void erase_to_eol(void);
static void insert_line(void);
static void delete_line(void);

static void set_fg(void);
static void set_bg(void);
static void erase_from_home(void);
static void cursor_off(void);
static void cursor_on(void);
static void cursor_on_cnt(void);
static void save_cursor_pos(void);
static void restore_cursor_pos(void);
static void erase_line(void);
static void erase_from_bol_impl(void);
static void erase_from_bol(void);
static void reverse_video_on(void);
static void reverse_video_off(void);
static void line_wrap_on(void);
static void line_wrap_off(void);

static void do_bell(void);
static void do_backspace(void);
static void do_tab(void);
static void ascii_lf(void);
static void ascii_cr(void);

/* handlers for the console state machine */
static void esc_ch1(WORD);
static void get_row(WORD);
static void get_column(WORD);


/* jumptable for ESC + uppercase character */
static void (* const am_tab[])(void) = {
    cursor_up,          /* Cursor Up */
    cursor_down,        /* Cursor Down */
    cursor_right,       /* Cursor Right */
    cursor_left,        /* Cursor Left */
    clear_and_home,     /* Clear and Home */
    nop,                /* <ESC> F not supported */
    nop,                /* <ESC> G not supported */
    cursor_home,        /* Home */
    reverse_linefeed,   /* Reverse Line Feed */
    erase_to_eos,       /* Erase to End of Screen */
    erase_to_eol,       /* Erase to End of Line */
    insert_line,        /* Insert Line */
    delete_line         /* Delete Line */
};


/* jumptable for ESC + lowercase character */
static void (* const bw_tab[])(void) = {
    set_fg,             /* Set foreground color (1 more char) */
    set_bg,             /* Set background color (1 more char) */
    erase_from_home,    /* Erase from beginning of page */
    cursor_on,          /* Cursor On */
    cursor_off,         /* Cursor Off */
    nop,                /* <ESC> g not supported */
    nop,                /* <ESC> h not supported */
    nop,                /* <ESC> i not supported */
    save_cursor_pos,    /* Save Cursor Position */
    restore_cursor_pos, /* Restore Cursor position */
    erase_line,         /* Erase line */
    nop,                /* <ESC> m not supported */
    nop,                /* <ESC> n not supported */
    erase_from_bol,     /* Erase from Beginning of Line */
    reverse_video_on,   /* Reverse Video On */
    reverse_video_off,  /* Reverse Video Off */
    nop,                /* <ESC> r not supported */
    nop,                /* <ESC> s not supported */
    nop,                /* <ESC> t not supported */
    nop,                /* <ESC> u not supported */
    line_wrap_on,       /* Wrap at End of Line */
    line_wrap_off       /* No Wrap at End of Line */
};


/* jumptable for ASCII control codes */
static void (* const cntl_tab[])(void) = {
    do_bell,            /* 7 = bell */
    do_backspace,       /* 8 = backspace */
    do_tab,             /* 9 = Horizontal tab */
    ascii_lf,           /* 10 = Line feed */
    ascii_lf,           /* 11 = Vertical tab (Treated as line feed) */
    ascii_lf,           /* 12 = Form Feed (Treated as line feed) */
    ascii_cr            /* 13 = Carriage Return */
};


/*
 * cputc - console output
 */
void cputc(WORD ch)
{
#if CONF_SERIAL_CONSOLE && !CONF_SERIAL_CONSOLE_ANSI
    /* When no translation needs to be performed, output the character
     * immediately and unconditionally to the serial port.
     * When ANSI translation is required, output will be done in the
     * appropriate subroutines.
     */
    bconout(1, ch);
#endif

    if (!con_state) {
        /* vt52_init() has not been called yet, ignore */
        return;
    }

    /* based on our state goto the correct stub routine */
    (*con_state)(LOBYTE(ch));
}


/*
 * normal_ascii - state is normal output
 */
static void normal_ascii(WORD ch)
{
    /* If the character is printable ascii, go print it */
    if ( ch >= ' ' ) {
#if CONF_SERIAL_CONSOLE_ANSI
        bconout(1, ch);
#endif
        conout_ascii_out(ch);
    }

    /* We handle the following control characters as special: */

    /* 7 = bell */
    /* 8 = backspace */
    /* 9 = Horizontal tab */
    /* 10 = Line feed */
    /* 11 = Vertical tab (Treated as line feed) */
    /* 12 = Form Feed (Treated as line feed) */
    /* 13 = Carriage Return */
    /* 27 = Escape (Start Command) */

    /* If escape character alter next state */
    else if ( ch == 0x1b ) {
        /* handle the control characters */
        con_state = esc_ch1;    /* set constate to handle esc codes */
    }

    /* Other control characters */
    else if ( ch >= 7 && ch <= 13 ) {
#if CONF_SERIAL_CONSOLE_ANSI
        bconout(1, ch);
#endif
        (*cntl_tab[ch - 7])();
    }
    /* All others are thrown away */
}


static void nop(void)
{
    return;
}


/*
 * do_bell - Ring the bell (in sound.c)
 */
static void do_bell(void)
{
    if (conterm & 4) {
        bell();
    }
}


/*
 * do_backspace - Same as Cursor Left
 */
static void do_backspace(void)
{
    cursor_left_impl();
}


/*
 * do_tab - calculate the tabulator values
 */
static void do_tab(void)
{
    conout_move_cursor((v_cur_cx & 0xfff8) + 8, v_cur_cy);
}


/*
 * esc_ch1 - state is: handle first character of an escape sequence
 */
static void esc_ch1(WORD ch)
{
    con_state = normal_ascii;           /* default state is normal ascii */

    if ( (ch >= 'A') && (ch <= 'M') ) {     /* handle the range A-M */
        (*am_tab[ch-'A'])();
    }
    else if ( (ch >= 'b') && (ch <= 'w') ) {/* handle b-w */
        (*bw_tab[ch-'b'])();
    }
    else if ( ch == 'Y' ) {                 /* direct cursor addressing, need more chars */
        con_state = get_row;
    }
}


/*
 * get_row - state is: calculate row from character
 */
static void get_row(WORD ch)
{
    save_row = ch - POSITION_BIAS;      /* Remove space bias */
    con_state = get_column;
}


/*
 * get_column - state is: calculate column from character
 */
static void get_column(WORD ch)
{
    int row, col;
    char ansi[20];

    MAYBE_UNUSED(ansi);

    col = ch - POSITION_BIAS;           /* Remove space bias */
    row = save_row;
#if CONF_SERIAL_CONSOLE_ANSI
    sprintf(ansi, "\033[%d;%dH", row + 1, col + 1);
    bconout_str(1, ansi);
#endif
    conout_move_cursor(col,row);
    con_state = normal_ascii;           /* Next char is not special */
}


/*
 * get_fg_col - state is: get foreground color
 */
static void get_fg_col(WORD ch)
{
#if CONF_SERIAL_CONSOLE_ANSI
    char ansi[10];
    sprintf(ansi, "\033[%dm", 30 + (ch & 7));
    bconout_str(1, ansi);
#endif

    /* set the foreground color from the 4 low-order bits only */
    v_col_fg = ch & 0x0f;
    con_state = normal_ascii;           /* Next char is not special */
}


/*
 * get_bg_col - state is: get background color
 */
static void get_bg_col(WORD ch)
{
#if CONF_SERIAL_CONSOLE_ANSI
    char ansi[10];
    sprintf(ansi, "\033[%dm", 40 + (ch & 7));
    bconout_str(1, ansi);
#endif

    /* set the foreground color from the 4 low-order bits only */
    v_col_bg = ch & 0x0f;
    con_state = normal_ascii;           /* Next char is not special */
}


static void set_fg(void)
{
    con_state = get_fg_col;             /* Next char is the FG color */
}


static void set_bg(void)
{
    con_state = get_bg_col;             /* Next char is the BG color */
}


/*
 * clear_and_home - Clear Screen and Home Cursor
 */
static void clear_and_home(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
# if SERIAL_CONSOLE_HONOR_HOME
    bconout_str(1, "\033[H\033[2J");
# else
    if ( v_cur_cx )
        bconout_str(1, "\r\n");
# endif
#endif

    cursor_off();                                /* hide cursor */
    conout_move_cursor(0, 0);                    /* cursor home */
    conout_blank_out (0, 0, v_cel_mx, v_cel_my); /* clear screen */
    cursor_on_cnt();                             /* show cursor */
}


/*
 * cursor_up - Alpha Cursor Up
 */
static void cursor_up(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[A");
#endif

    if (v_cur_cy)
        conout_move_cursor(v_cur_cx, v_cur_cy - 1);
}


/*
 * cursor_down_impl - Used by Cursor Down and LF
 */
static void cursor_down_impl(void)
{
    if (v_cur_cy != v_cel_my)
        conout_move_cursor(v_cur_cx, v_cur_cy + 1);
}


/*
 * cursor_down - Alpha Cursor Down
 */
static void cursor_down(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[B");
#endif

    cursor_down_impl();
}


/*
 * cursor_right - Alpha Cursor Right
 */
static void cursor_right(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[C");
#endif

    if (v_cur_cx != v_cel_mx)
        conout_move_cursor(v_cur_cx + 1, v_cur_cy);
}


/*
 * cursor_left_impl - Used by Cursor Left and Backspace
 */
static void cursor_left_impl(void)
{
    if (v_cur_cx )
        conout_move_cursor(v_cur_cx - 1, v_cur_cy);
}


/*
 * cursor_left - Alpha Cursor Left
 */
static void cursor_left(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[D");
#endif

    cursor_left_impl();
}


/*
 * cursor_home - Home Alpha Cursor
 */
static void cursor_home(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
# if SERIAL_CONSOLE_HONOR_HOME
    bconout_str(1, "\033[H");
# else
    if ( v_cur_cx )
        bconout_str(1, "\r\n");
# endif
#endif

    conout_move_cursor(0, 0);
}


/*
 * erase_to_eos - Erase to End of Screen
 */
static void erase_to_eos(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[J");
#endif

    erase_to_eol_impl(); /* erase to end of line */

    /* last line? */
    if (v_cur_cy == v_cel_my)
        return;    /* yes, done */

    /* erase from upper left corner to lower right corner */
    conout_blank_out (0, v_cur_cy + 1, v_cel_mx, v_cel_my);
}


/*
 * erase_to_eol_impl - Erase to End of Line (implementation)
 */
static void erase_to_eol_impl(void)
{
    BOOL wrap = v_stat_0 & M_CEOL;      /* save line wrap status */
    WORD s_cur_x, s_cur_y;

    v_stat_0 &= ~M_CEOL;    /* clear EOL handling bit (overwrite) */

    cursor_off();               /* hide cursor */
    /* save the x and y coords of cursor */
    s_cur_x = v_cur_cx;
    s_cur_y = v_cur_cy;

    /* is x = x maximum? */
    if (v_cur_cx == v_cel_mx)
        conout_ascii_out(' ');         /* output a space, the cell is odd! */
    else {
        /* test, if x is even or odd */
        if (IS_ODD(v_cur_cx))
            conout_ascii_out(' ');     /* first output a space */

        conout_blank_out (v_cur_cx, v_cur_cy, v_cel_mx, v_cur_cy);
    }

    /* restore wrap flag, the result of EOL test */
    if (wrap)
        v_stat_0 |= M_CEOL;

    conout_move_cursor(s_cur_x, s_cur_y); /* restore cursor position */
    cursor_on_cnt();            /* show cursor */
}


/*
 * erase_to_eol - Erase to End of Line
 */
static void erase_to_eol(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[K");
#endif

    erase_to_eol_impl();
}


/*
 * reverse_video_on - Reverse Video On
 */
static void reverse_video_on(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[7m");
#endif

    v_stat_0 |= M_REVID;    /* set the reverse bit */
}


/*
 * reverse_video_off - Reverse Video Off
 */
static void reverse_video_off(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[27m");
#endif

    v_stat_0 &= ~M_REVID;    /* clear the reverse bit */
}


/*
 * reverse_linefeed - Reverse Index
 */
static void reverse_linefeed(void)
{
    /* if not at top of screen */
    if (v_cur_cy) {
        conout_move_cursor(v_cur_cx, v_cur_cy - 1);
    }
    else
    {
        int savex = v_cur_cx;           /* save current x position */
        insert_line();                  /* Insert a line */
        conout_move_cursor(savex, 0);
    }
}


/*
 * insert_line - Insert Line
 */
static void insert_line(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[L");
#endif
    cursor_off();                      /* hide cursor */
    conout_scroll_down(v_cur_cy);      /* scroll down 1 line & blank current line */
    conout_move_cursor(0, v_cur_cy);   /* move cursor to beginning of line */
    cursor_on_cnt();                   /* show cursor */
}


/*
 * delete_line - Delete Line
 */
static void delete_line(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[M");
#endif
    cursor_off();               /* hide cursor */
    conout_scroll_up(v_cur_cy);        /* scroll up 1 line & blank bottom line */
    conout_move_cursor(0, v_cur_cy);   /* move cursor to beginning of line */
    cursor_on_cnt();            /* show cursor */
}


/*
 * erase_from_home - Erase from Beginning of Page to cursor
 */
static void erase_from_home(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[1J");
#endif

    erase_from_bol_impl(); /* erase from beginning of line */

    /* first line? */
    if (!v_cur_cy)
        return;    /* yes, done */

    /* erase rest of screen */
    conout_blank_out (0, 0, v_cel_mx, v_cur_cy - 1);        /* clear screen */
}


/*
 * do_cnt_esce - Enable Cursor
 */
static void do_cnt_esce(void)
{
    conout_enable_cursor();
}


/*
 * cursor_on - Enable Cursor forced
 */
static void cursor_on(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    /* Disabled because function used from internal VT52 implementation */
    /* bconout_str(1, "\033[?25h"); */
#endif

    /* if disable count is zero (cursor still shown) then return */
    if (!disab_cnt)
        return;

    disab_cnt = 0;                      /* reset the disable counter */
    do_cnt_esce();
}


/*
 * cursor_on_cnt - Enable Cursor (counted depth)
 */
static void cursor_on_cnt(void)
{
    /* if disable count is zero (cursor still shown) then return */
    if (!disab_cnt)
        return;

    disab_cnt--;                        /* decrement the disable counter */
    if (!disab_cnt)
        do_cnt_esce();                  /* if 0, do the enable */
}


/*
 * cursor_off - Disable Cursor
 */
static void cursor_off(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    /* Disabled because function used from internal VT52 implementation */
    /* bconout_str(1, "\033[?25l"); */
#endif

    disab_cnt++;                        /* increment the disable counter */

    conout_disable_cursor();
}


/*
 * save_cursor_pos - Save Cursor Position
 */
static void save_cursor_pos(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    /* Disabled because function used from internal VT52 implementation */
    /* bconout_str(1, "\033[s"); */
#endif

    v_stat_0 |= M_SVPOS;    /* set "position saved" status bit */

    /* save the x and y coords of cursor */
    sav_cur_x = v_cur_cx;
    sav_cur_y = v_cur_cy;
}


/*
 * restore_cursor_pos - Restore Cursor Position
 */
static void restore_cursor_pos(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    /* Disabled because function used from internal VT52 implementation */
    /* bconout_str(1, "\033[u"); */
#endif

    if ( v_stat_0 & M_SVPOS )
        conout_move_cursor(sav_cur_x, sav_cur_y);      /* move to saved position */
    else
        conout_move_cursor(0, 0);      /* if position was not saved, home cursor */

    v_stat_0 &= ~M_SVPOS;    /* clear "position saved" status bit */
}


/*
 * erase_line - Erase Entire Line
 *
 * upper left coords. (0,y), lower right coords. (max,y)
 */
static void erase_line(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[2K\033[1G");
#endif

    cursor_off();               /* hide cursor */
    conout_blank_out (0, v_cur_cy, v_cel_mx, v_cur_cy);   /* blank whole line */
    conout_move_cursor(0, v_cur_cy);   /* move cursor to beginning of line */
    cursor_on_cnt();            /* show cursor */
}


/*
 * erase_from_bol_impl - Erase from Beginning of Line (implementation)
 *
 * upper left coords. (0,y)
 * lower right coords. (x,y)
 */
static void erase_from_bol_impl(void)
{
    WORD s_cur_x, s_cur_y;

    cursor_off();               /* hide cursor */
    /* save the x and y coords of cursor */
    s_cur_x = v_cur_cx;
    s_cur_y = v_cur_cy;

    /*
     * because conout_blank_out() requires the ending x position to be
     * odd, we need to handle the two possibilities separately
     */
    if (!IS_ODD(s_cur_x)) {
        conout_ascii_out(' ');     /* first output a space */
        if (s_cur_x)
            conout_blank_out(0, s_cur_y, s_cur_x-1, s_cur_y);
    }
    else
        conout_blank_out(0, s_cur_y, s_cur_x, s_cur_y);

    conout_move_cursor(s_cur_x, s_cur_y); /* restore cursor position */
    cursor_on_cnt();            /* show cursor */
}


/*
 * erase_from_bol - Erase from Beginning of Line
 *
 * upper left coords. (0,y)
 * lower right coords. (x,y)
 */
static void erase_from_bol(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[1K");
#endif

    erase_from_bol_impl();
}


/*
 * line_wrap_on() - Wrap at End of Line
 */
static void line_wrap_on(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[7h");
#endif
    v_stat_0 |= M_CEOL;    /* set the eol handling bit */
}


/*
 * line_wrap_off - Discard at End of Line
 */
static void line_wrap_off(void)
{
#if CONF_SERIAL_CONSOLE_ANSI
    bconout_str(1, "\033[7l");
#endif
    v_stat_0 &= ~M_CEOL;    /* clear the eol handling bit */
}


/*
 * ascii_cr - carriage return
 */
static void ascii_cr(void)
{
    /* beginning of current line */
    conout_move_cursor(0, v_cur_cy);
}


/*
 * ascii_lf - line feed
 */
static void ascii_lf(void)
{
    /* at bottom of screen? */
    if ( v_cur_cy != v_cel_my )
        cursor_down_impl();
    else {
        cursor_off();                   /* yes, hide cursor */
        conout_scroll_up(0);                   /* scroll up 1 line */
        cursor_on_cnt();                /* show cursor */
    }
}


/*
 * cursconf - cursor configuration
 *
 * Arguments:
 *
 *   function =
 *   0 - switch off cursor
 *   1 - switch on cursor
 *   2 - blinking cursor
 *   3 - not blinking cursor
 *   4 - set cursor blink rate
 *   5 - get cursor blink rate
 *
 * Bits:
 *   M_CFLASH - cursor flash on
 *   M_CVIS   - cursor visibility on
 */
WORD cursconf(WORD function, WORD operand)
{
    KDEBUG(("cursconf\n"));
    switch (function) {
    case 0:
        cursor_off();                   /* set cursor non-visible */
        break;
    case 1:
        cursor_on();                    /* set cursor visible */
        break;
    case 2:
        CURSOR_FLASH_DISABLE;           /* unset cursor flash bit */
        break;
    case 3:
        CURSOR_FLASH_ENABLE;            /* set cursor flash bit */
        break;
    case 4:
        v_period = LOBYTE(operand);     /* set cursor flash interval */
        break;
    case 5:
        return(v_period);               /* get cursor flash interval */
    }
    KDEBUG(("cursconf exiting\n"));
    return 0;
}


/*
 * vt52_init - initialize the conout state machine
 */
void vt52_init(void)
{
    KDEBUG(("vt52_init\n"));
    /* set font-related lineA variables */
    conout_init(font_set_default());

    /* Initial cursor settings */
    v_cur_cx = 0;                       /* cursor to column 0, row 0 */
    v_cur_cy = 0;
    v_cur_of = 0;                       /* line offset is 0 */

    v_stat_0 = M_CFLASH;                /* cursor invisible, flash, nowrap, normal video */
    cursconf(4, 30);                    /* 0.5 second blink rate (@ 60Hz vblank) */
    v_cur_tim = v_period;               /* load initial value to blink timer */
    disab_cnt = 1;                      /* cursor disabled 1 level deep */

    /* set foreground color depending on color depth */
    switch (v_planes) {
    case 1:
        v_col_fg = 1;
        break;
    case 2:
        v_col_fg = 3;
        break;
    default:
        v_col_fg = 15;
    }
    v_col_bg = 0;

    con_state = normal_ascii;           /* Init conout state machine */

    clear_and_home();
    KDEBUG(("vt52_init exiting\n"));
}
