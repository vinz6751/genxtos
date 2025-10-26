/*
 * keyboard.c - Keyboard routines
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *        Robert de Vries (Linux m68k)
 *        Bjoern Brauel   (Linux m68k)
 *        Roman Hodek     (Linux m68k)
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * Some code I got from Linux m68k, thanks to the authors! (MAD)
 */

/*
 * not supported:
 * - alt-help screen hardcopy
 * - KEYTBL.TBL config with _AKP cookie (tos 5.00 and later)
 */

/*#define ENABLE_KDEBUG*/

#include "emutos.h"
#include "aciavecs.h"
#include "country.h"
#include "acia.h"
#include "tosvars.h"
#include "biosext.h"
#include "lineavars.h"
#include "keyboard_mouse_emulation.h"
#include "ikbd.h"
#include "keyboard.h"
#include "iorec.h"
#include "asm.h"
#include "ikbd.h"
#include "sound.h"              /* for keyclick */
#include "scancode.h"
#include "delay.h"
#include "bios.h"
#include "coldfire.h"
#include "amiga.h"
#include "lisa.h"
#include "a2560u_bios.h"

#define KEY_CTRL_HOME 0x77      /* scancode values set when ctrl modifies scancode */
#define KEY_CTRL_LTARROW 0x73
#define KEY_CTRL_RTARROW 0x74

#define TOPROW_START 0x02       /* numeric keys, minus, equals */
#define TOPROW_END  0x0d

#define KEYPAD_START 0x67       /* numeric keypad: 7 8 9 4 5 6 1 2 3 0 */
#define KEYPAD_END  0x70

/* standard ascii */
#define LF          0x0a


/*=== interrupt routine support ===================================*/

/* the number of the current dead key if any, else -1. */
static int kb_dead;

/* the decimal number collected in an alt_keypad sequence, or -1 */
static int kb_altnum;

/* a boolean value, non-zero iff the scancode lookup table has been switched */
/* (this can only happen for keyboards with the DUAL_KEYBOARD feature)       */
static UBYTE kb_switched;

static struct keytbl current_keytbl;

static WORD convert_scancode(UBYTE *scancodeptr);


/*
 * key repeat is handled as follows:
 * As a key is hit and enters the buffer, the scancode, ascii equivalent,
 * and the state of 'shifty' are recorded in kb_last.  At the same time a
 * downward counter kb_ticks is set to the first delay, kb_initial_delay.
 *
 * Every fourth timer C interrupt (50 Hz ticks), kb_tick is decremented if
 * not zero (zero means no more key repeat for this key).  If kb_tick
 * reaches zero, the value of 'shifty' is compared to the saved value.  If
 * they are the same, the value in kb_last is re-emitted; otherwise:
 *  . the scancode is re-evaluated using the new value of 'shifty'
 *  . a (probably new) key is emitted
 *  . kb_last is updated
 * In either case, kb_tick is now set to the second delay kb_repeat.
 *
 * Any release of a *non-modifier* key stops the repeat handling by
 * setting kb_tick back to zero.  It's also set back to zero by the release
 * of the alt key during alt-numpad processing.  Allowing the repeat to
 * continue when e.g. the state of the shift key changes provides the
 * same functionality as Atari TOS.
 *
 * There is no need to disable interrupts when modifying kb_tick, kb_initial_delay
 * and kb_repeat since:
 * - each routine not in interrupt routines only accesses kb_tick once;
 * - the interrupt routines (ACIA and timer C) cannot happen at the same
 *   time (they both have IPL level 6);
 * - interrupt routines do not modify kb_initial_delay and kb_repeat.
 */

static WORD kb_initial_delay;
static WORD kb_repeat;
/* countdown of 50 tickss until next key repeat. 0 means nothing to repeat */
static WORD kb_ticks;
static union {
    ULONG key;                  /* combined value */
    struct {
        UBYTE shifty;           /* state of 'shifty' */
        UBYTE scancode;         /* possibly-converted scancode */
        UWORD ascii;            /* derived ascii value */
    } k;
} kb_last;
static UBYTE kb_last_actual;    /* actual last scancode */
static PFVOID kb_last_ikbdsys;  /* ikbdsys when kb_last was set */

static void key_repeat_init(void);
static void key_repeat_stop(void);
static void key_repeat_do(void);

/*
 *      FUNCTION:  This routine resets the keyboard,
 *        configures the MFP so we can get interrupts
 */

void kbd_init(void)
{
#if CONF_WITH_IKBD_ACIA
    /* initialize ikbd ACIA */
    ikbd_acia.ctrl = ACIA_RESET;        /* master reset */

    ikbd_acia.ctrl = ACIA_RIE | /* enable interrupts */
        ACIA_RLTID |            /* RTS low, TxINT disabled */
        ACIA_DIV64 |            /* clock/64 */
        ACIA_D8N1S;             /* 8 bit, 1 stop, no parity */
#endif /* CONF_WITH_IKBD_ACIA */

#if CONF_WITH_FLEXCAN
    /* On ColdFire machines, an Eiffel adapter may be present on the CAN bus. */
    coldfire_init_flexcan();
#endif

#ifdef MACHINE_AMIGA
    amiga_kbd_init();
#endif

#ifdef MACHINE_LISA
    lisa_kbd_init();
#endif

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_bios_kbd_init();
#endif

#if CONF_WITH_IKBD_ACIA
    /* initialize the IKBD */
    ikbd_reset();
#endif

    key_repeat_init();

    mouse_emulation_init();

    kb_dead = -1;      /* not in a dead key sequence */
    kb_altnum = -1;    /* not in an alt-numeric sequence */
    kb_switched = 0;   /* not switched initially */

    conterm = 7;       /* keyclick, key repeat & system bell on by default */

    shifty = 0;        /* initial state of modifiers */

    /* install default keymap */
    bioskeys();
}


static void handle_reset_key_combo(UBYTE scancode) {
    /* keyboard warm/cold reset */
    if ((scancode == KEY_DELETE)
        && ((shifty & (MODE_ALT|MODE_CTRL|MODE_LSHIFT)) == (MODE_ALT|MODE_CTRL))) {
        /* Del key and shifty is Alt+Ctrl but not LShift */
        if (shifty & MODE_RSHIFT) {
            /* Ctrl+Alt+RShift+Del means cold reset */
            cold_reset();
        }
        else {
            /* Ctrl+Alt+Del means warm reset */
            warm_reset();
        }
    }
}

/*
 * kbd_int : called by the interrupt routine for key events.
 */

void kbd_int(UBYTE scancode)
{
    WORD ascii = 0;
    UBYTE scancode_only = scancode & ~KEY_RELEASED;  /* get rid of release bits */
    BOOL modifier;

    KDEBUG(("================\n"));
    KDEBUG(("Key-scancode: 0x%02x, key-shift bits: 0x%02x\n", scancode, shifty));

    handle_reset_key_combo(scancode);

#if CONF_WITH_EXTENDED_MOUSE
    /* the additional mouse buttons use a separate vector */
    if (   scancode_only == 0x37  /* Mouse button 3 */
        || scancode_only == 0x5e  /* Mouse button 4 */
        || scancode_only == 0x5f  /* Mouse button 5 */
        || scancode_only == 0x59  /* Wheel up */
        || scancode_only == 0x5a  /* Wheel down */
        || scancode_only == 0x5c  /* Wheel left */
        || scancode_only == 0x5d) /* Wheel right */
    {
        mousexvec(scancode);
        return;
    }
#endif

    if (scancode & KEY_RELEASED) {
        switch (scancode_only) {
        case KEY_RSHIFT:
            shifty &= ~MODE_RSHIFT;     /* clear bit */
            break;
        case KEY_LSHIFT:
            shifty &= ~MODE_LSHIFT;     /* clear bit */
            break;
        case KEY_CTRL:
            shifty &= ~MODE_CTRL;       /* clear bit */
            break;          
        case KEY_ALT:
            shifty &= ~MODE_ALT;        /* clear bit */
            if (mouse_emulation_is_active())
                key_repeat_stop();
            if (kb_altnum >= 0) {
                ascii = LOBYTE(kb_altnum);
                kb_altnum = -1;
                key_repeat_stop();
                goto push_value;
            }
            break;
#ifdef MACHINE_AMIGA
        case KEY_CAPS:
            if (conterm & 1) {
                keyclick(KEY_CAPS);
            }
            shifty &= ~MODE_CAPS;       /* clear bit */
            break;
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_GENX)
        case KEY_RCTRL:
            shifty &= ~MODE_CTRL;       /* clear bit */
            break;
        case KEY_ALTGR:
            shifty &= ~MODE_ALTGR;       /* clear bit */
            break;
#endif  
        case KEY_EMULATE_LEFT_BUTTON:
            shifty &= ~MODE_MOUSEL;       /* clear bit */
            key_repeat_stop();
            break;
        case KEY_EMULATE_RIGHT_BUTTON:
            shifty &= ~MODE_MOUSER;     /* clear bit */
            key_repeat_stop();
            break;
        default:                    /* non-modifier keys: */
            if (scancode_only == kb_last_actual)    /* key-up matches last key-down: */
                key_repeat_stop();
        }

        mouse_emulation_handle_key_released(scancode);

        return;
    }

    /*
     * a key has been pressed
     */
    mouse_emulation_handle_key_pressed(scancode);
    modifier = TRUE;
    switch (scancode) {
    case KEY_RSHIFT:
        shifty |= MODE_RSHIFT;  /* set bit */
        break;
    case KEY_LSHIFT:
        shifty |= MODE_LSHIFT;  /* set bit */
        break;
    case KEY_CTRL:
        shifty |= MODE_CTRL;    /* set bit */
        break;
    case KEY_ALT:
        shifty |= MODE_ALT;     /* set bit */
        break;
    case KEY_CAPS:
        if (conterm & 1) {
            keyclick(KEY_CAPS);
        }
#ifdef MACHINE_AMIGA
        shifty |= MODE_CAPS;    /* set bit */
#else
        shifty ^= MODE_CAPS;    /* toggle bit */
#endif
        break;
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_GENX)
        case KEY_RCTRL:
            shifty |= MODE_CTRL;  /* set bit */
            break;
        case KEY_ALTGR:
            shifty |= MODE_ALTGR; /* set bit */
            break;
#endif        
    default:
        modifier = FALSE;
        break;
    }
    if (modifier) {     /* the user has pressed a modifier key */
        /*
         * first, check for keyboard hot switch and if so, do the switch
         */
        if (current_keytbl.features&DUAL_KEYBOARD)
            if ((shifty&MODE_SCA) == HOTSWITCH_MODE)
                kb_switched ^= 0x01;
        /*
         * check if an arrow key was already down and, if so, send the appropriate mouse packet
         */
        handle_mouse_mode(scancode);
        return;
    }

    /*
     * a non-modifier key has been pressed
     */
    kb_last_actual = scancode;  /* save because 'scancode' may get changed */
    if (shifty & MODE_ALT) {    /* only if the Alt key is down ... */
        switch(scancode) {
        case KEY_HOME:
            shifty |= MODE_MOUSEL;    /* set bit */
            break;
        case KEY_INSERT:
            shifty |= MODE_MOUSER;  /* set bit */
            break;
        }
    }

    ascii = convert_scancode(&scancode);
    if (ascii < 0)      /* dead key (including alt-keypad) processing */
        return;

    kb_ticks = kb_initial_delay;

  push_value:
    if (conterm & 1)
        keyclick(scancode);

    /*
     * save last key info for key_repeat_do()
     */
    kb_last.k.shifty = shifty;
    kb_last.k.scancode = scancode;
    kb_last.k.ascii = ascii;
    kb_last_ikbdsys = kbdvecs.ikbdsys;

    /*
     * if we're not sending mouse packets, send a real key
     */
    if (!mouse_emulation_is_active())
        iorec_put_long(&ikbdiorec, kb_last.key);
}


/*=== Keymaps handling (xbios) =======================================*/

LONG keytbl(const UBYTE* norm, const UBYTE* shft, const UBYTE* caps)
{
    if (norm != (UBYTE*)-1) {
        current_keytbl.norm = norm;
    }
    if (shft != (UBYTE*)-1) {
        current_keytbl.shft = shft;
    }
    if (caps != (UBYTE*)-1) {
        current_keytbl.caps = caps;
    }
    return (LONG) & current_keytbl;
}

void bioskeys(void)
{
    /* ask country.c for the default key table of the current country */
    current_keytbl = *get_keytbl();
}


LONG kbshift(WORD flag)
{
    WORD oldshifty = shifty;

    if (flag >= 0)
        shifty = flag;

    return oldshifty;
}

LONG bconstat2(void)
{
#if CONF_SERIAL_CONSOLE_POLLING_MODE
    /* Poll the serial port */
    return bconstat(1);
#else
    /* Check the IKBD IOREC */
    return iorec_can_read(&ikbdiorec);
#endif
}

LONG bconin2(void)
{
    ULONG value;
#if CONF_SERIAL_CONSOLE_POLLING_MODE
    /* Poll the serial port */
    UBYTE ascii = (UBYTE)bconin(1);
    value = ikbdiorec_from_ascii(ascii);
#else

    while (!bconstat2()) {
#if USE_STOP_INSN_TO_FREE_HOST_CPU
        stop_until_interrupt();
#endif
    }

    value = iorec_get_long(&ikbdiorec);

#endif /* CONF_SERIAL_CONSOLE_POLLING_MODE */

    if (!(conterm & 8))         /* shift status not wanted? */
        value &= 0x00ffffffL;   /* true, so clean it out */

    return value;
}


/*
 * convert a scancode to an ascii character
 *
 * for certain keys, we also update the scancode:
 *  shifted function keys
 *  some keys when modified by ctrl
 */
static WORD convert_scancode(UBYTE *scancodeptr)
{
    UBYTE scancode = *scancodeptr;
    WORD ascii = 0;
    const UBYTE *a;

    /*
     * do special processing for some keys that are in the same position
     * on all keyboards:
     * (a) invariant scancodes (unaffected by modifier keys)
     * (b) return/enter (unaffected except that ctrl causes LF to be returned)
     */
    switch(scancode) {
    case KEY_RETURN:
    case KEY_ENTER:
        if (shifty & MODE_CTRL)
            return LF;
        FALLTHROUGH;
    case KEY_ESCAPE:
    case KEY_BACKSPACE:
    case KEY_TAB:
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
#else    
    case KEY_UNDO:
#endif    
        return current_keytbl.norm[scancode];
    }

    /*
     * do special processing for alt-arrow, alt-help, alt-keypad,
     * alt-number & shift-function keys, then return
     */
    if (shifty & MODE_ALT) {
        /*
         * the alt key is down: check if the user has pressed an arrow key
         * and, if so, send the appropriate mouse packet
         */
        if (handle_mouse_mode(scancode))    /* we sent a packet, */
            return 0;                       /* so we're done     */

#if 0 // TODO support printscreen key
        if (scancode == KEY_HELP) {
            dumpflg++;      /* tell VBL to call scrdmp() function */
            return 0;
        }
#endif

        /* ALT-keypad means that char number */
        if ((scancode >= KEYPAD_START) && (scancode <= KEYPAD_END)) {
            if (kb_altnum < 0)
                kb_altnum = 0;
            else kb_altnum *= 10;
            kb_altnum += "\7\10\11\4\5\6\1\2\3\0" [scancode-KEYPAD_START];
            return -1;
        }
        if ((scancode >= TOPROW_START) && (scancode <= TOPROW_END)) {
            *scancodeptr += 0x76;
            return 0;
        }
    } else if (shifty & MODE_SHIFT) {
        /* function keys F1 to F10 => F11 to F20 */
        if ((scancode >= KEY_F1) && (scancode <= KEY_F10)) {
            *scancodeptr += 0x19;
            return 0;
        }
    }

    /*
     * which keyboard table to use, and how to handle it, depends on
     * the presence or absence of the DUAL_KEYBOARD feature
     *
     * Alt-X handling:
     * we obtain a default ascii value by using the standard keyboard
     * tables.  if the DUAL_KEYBOARD feature is not present, we may then
     * override that value via a lookup using the 'alternate' keyboard
     * tables, which are set up as (scancode,ascii) pairs.  finally we
     * zero out alphabetic ascii values.
     *
     * All other key handling:
     * if 'kb_switched' is set (only set if a keyboard table has the
     * DUAL_KEYBOARD feature AND the tables have been switched via the
     * hotswitch key), the alternate keyboard tables are used; otherwise
     * the main keyboard tables are used (these are all 128-byte direct
     * scancode lookup tables).
     */
    if (shifty & MODE_ALTGR) {
        if (shifty & MODE_SHIFT) {
            a = current_keytbl.shft;
        } else if (shifty & MODE_CAPS) {
            a = current_keytbl.caps;
        } else {
            a = current_keytbl.norm;
        }
        ascii = a[scancode];
        if ((current_keytbl.features & DUAL_KEYBOARD) == 0) {
            if (shifty & MODE_SHIFT) {
                a = current_keytbl.altshft;
            } else if (shifty & MODE_CAPS) {
                a = current_keytbl.altcaps;
            } else {
                a = current_keytbl.altnorm;
            }
            while (*a && (*a != scancode)) {
                a += 2;
            }
            if (*a++) {
                ascii = *a;
            }
        }
        if (((ascii >= 'A') && (ascii <= 'Z'))
         || ((ascii >= 'a') && (ascii <= 'z')))
            ascii = 0;
    } else {
        if (shifty & MODE_SHIFT) {
            a = kb_switched ? current_keytbl.altshft : current_keytbl.shft;
        } else if (shifty & MODE_CAPS) {
            a = kb_switched ? current_keytbl.altcaps : current_keytbl.caps;
        } else {
            a = kb_switched ? current_keytbl.altnorm : current_keytbl.norm;
        }
        ascii = a[scancode];
    }

    /*
     * Ctrl key handling is mostly straightforward, but there are a few warts
     */
    if (shifty & MODE_CTRL) {
        switch(ascii) {
        case '-':
            ascii = 0x1f;
            break;
        case '2':
            ascii = 0x00;
            break;
        case '6':
            ascii = 0x1e;
        }
        switch(scancode) {
        case KEY_HOME:
            *scancodeptr = KEY_CTRL_HOME;
            break;
        case KEY_LTARROW:
            *scancodeptr = KEY_CTRL_LTARROW;
            break;
        case KEY_RTARROW:
            *scancodeptr = KEY_CTRL_RTARROW;
            break;
        default:
            ascii &= 0x1F;
        }
    } else if (kb_dead >= 0) {
        a = current_keytbl.dead[kb_dead];
        while (*a && *a != ascii) {
            a += 2;
        }
        if (*a++) {
            ascii = *a;
        }
        kb_dead = -1;
    } else if ((ascii >= DEADMIN) && (ascii <= DEADMAX)) {
        kb_dead = ascii - DEADMIN;
        return -1;
    }

    return ascii;
}


static void key_repeat_init(void) {
    /* initialize the key repeat stuff */
    key_repeat_stop();
    kb_initial_delay = KB_INITIAL;
    kb_repeat = KB_REPEAT;
}

WORD kbrate(WORD initial, WORD repeat)
{
    WORD ret = MAKE_UWORD(kb_initial_delay, kb_repeat);

    if (initial >= 0)
        kb_initial_delay = initial & 0x00ff;
    if (repeat >= 0)
        kb_repeat = repeat & 0x00ff;

    return ret;
}

/* Keyboard timer interrupt handler.
 * It is called at 50 Hz (each 4th Timer C call).
 */
void key_repeat_tick(void)
{
    key_repeat_do();
}


static void key_repeat_stop(void)
{
    kb_ticks = 0;
}

static void key_repeat_do(void)
{
    if (kb_ticks > 0 && kbdvecs.ikbdsys != kb_last_ikbdsys)
    {
        /*
         * There is a key repeat in progress, but the ikbdsys handler has
         * changed since the key was pressed. The new handler may handle
         * the key repeat totally differently, so stop the current key repeat
         * right now.
         *
         * Concretely, this scenario happens when starting FreeMiNT on ARAnyM.
         * When the user presses a key at the EmuTOS welcome screen,
         * FreeMiNT installs its new ikbdsys handler before the user has time
         * to release the key. So this EmuTOS key repeat routine keeps running
         * indefinitely, filling FreeMiNT's keyboard buffer, but without
         * producing keyboard events. As soon as the user presses any key,
         * he sees the ghost repeating keys.
         *
         * This trick fixes the problem.
         */
        key_repeat_stop();
        return;
    }

    if (!(conterm & 2))
    {
        /* Key repeat is globally disabled */
        return;
    }

    if (kb_ticks == 0)
    {
        /* No key is currently repeating */
        return;
    }

    /* Decrease delay */
    if (--kb_ticks > 0)
    {
        /* Nothing to do this time */
        return;
    }

    /* Play the key click sound */
    if (conterm & 1)
        keyclick(kb_last.k.scancode);

    /*
     * changing the modifier keys no longer stops key repeat, so when
     * they change, we must do the scancode conversion again
     */
    if (shifty != kb_last.k.shifty) {
        /* get actual last scancode because convert_scancode() can change it */
        kb_last.k.scancode = kb_last_actual;
        kb_last.k.ascii = convert_scancode(&kb_last.k.scancode);
        kb_last.k.shifty = shifty;
        kb_last_ikbdsys = kbdvecs.ikbdsys;
    }

    /*
     * Simulate a key press or a mouse action
     */
    if (mouse_emulation_is_active())
    {
        mouse_emulation_repeat();
    }
    else
        iorec_put_long(&ikbdiorec, kb_last.key);

    /* Reload the decounter. The key will repeat again until some key up */
    kb_ticks = kb_repeat;
}



