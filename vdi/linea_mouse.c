/*
 * linea_mouse.c - line A mouse support
 *
 * Copyright (C) 2001-2022 by Authors:
 *
 * Authors:
 *  MAD  Martin Doering
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "aesdefs.h"
#include "linea.h"
#include "tosvars.h"   /* vblqueue */
#include "xbiosbind.h" /* Initmous */
#include "vdiext.h"
#include "a2560u.h"

/* In .S file */
void mousevec_handler(void);  /* Responds to incoming mouse packet */
void linea_mouse_moved(void); /* Code to call when mouse has moved to raise a flag
                               * (vbl_must_draw_mouse) that will tell the VBL handler 
                               * to update the mouse on the screen to the new location */
static void vbl_draw(void);   /* VBL queue item that update mouse location on the screen */

/* Initialize mouse via XBIOS in relative mode */
static const struct {
    UBYTE topmode;
    UBYTE buttons;
    UBYTE xparam;
    UBYTE yparam;
} mouse_params = {0, 0, 1, 1};


static BOOL linea_mouse_inited;

void linea_mouse_init(void)
{    
    a2560u_debug("linea_mouse_init");
    /* mouse settings */
    HIDE_CNT = 1;               /* mouse is initially hidden */
    GCURX = V_REZ_HZ / 2;       /* initialize the mouse to center */
    GCURY = V_REZ_VT / 2;
    MOUSE_BT = 0;               /* clear the mouse button state */
    cur_ms_stat = 0;            /* clear the mouse status */
    mouse_flag = 0;             /* clear the mouse flag */
    
    /* VBL mouse redraw setup */
    vbl_must_draw_mouse = 0;    /* VBL handler doesn't need to draw mouse */
#if false    /* That's not needed. They'll be set when we need them */
    newx = 0;                   /* set cursor x-coordinate to 0 */
    newy = 0;                   /* set cursor y-coordinate to 0 */   
#endif

    user_cur = linea_mouse_moved;
    user_but = just_rts;
    user_mot = just_rts;

#if CONF_WITH_EXTENDED_MOUSE
    user_wheel = just_rts;
#endif

    if (mouse_display_driver.init)
        mouse_display_driver.init();

    vblqueue[0] = vbl_draw;

    linea_mouse_set_form(&arrow_mform);

    Initmous(1, (LONG)&mouse_params, (LONG)mousevec_handler);

    linea_mouse_inited = TRUE;
    a2560u_debug("linea_mouse_init done");    
}


void linea_mouse_deinit(void)
{
    if (!linea_mouse_inited)
        return;

    vblqueue[0] = 0L;
    Initmous(0, 0, 0);
    linea_mouse_inited = FALSE;
}


static void vbl_draw(void)
{
    WORD old_sr, x, y;

    /* If the cursor is being modified, or is hidden, just exit */
    if (mouse_flag || HIDE_CNT)
        return;

    old_sr = set_sr(0x2700);        /* Disable interrupts */
    if (vbl_must_draw_mouse)
    {            
        vbl_must_draw_mouse = FALSE;
        x = newx;                   /* Get x/y atomically for vbl_draw() */
        y = newy;
        set_sr(old_sr);
        mouse_display_driver.vbl_draw(x, y);
    }
    else
        set_sr(old_sr);
}

/*
 * This forces the mouse to be displayed
 */
void linea_mouse_force_show(void)
{
    if (!INTIN[0])
        HIDE_CNT = 1;           /* reset cursor to on */

    linea_mouse_show();
}


/*
 * linea_mouse_show - Displays the mouse cursor if the number of hide operations has gone back to 0.
 *
 *  Decrement the counter for the number of hide operations performed.
 *  If this is not the last one then do nothing because the cursor
 *  should remain hidden.
 *
 *   Outputs:
 *      hide_cnt = hide_cnt - 1
 *      vbl_must_draw_mouse = 0
 */
void linea_mouse_show(void)
{
    a2560u_debug("linea_mouse_show HIDE_CNT=%d",HIDE_CNT);

    if (HIDE_CNT != 1)      /* if not about to be shown: */
    {
        HIDE_CNT--;             /* just decrement hide count */
        if (HIDE_CNT < 0)       /* but make sure it doesn't go negative! */
            HIDE_CNT = 0;
        return;
    }

    /* HIDE_CNT is precisely 1 at this point */
    mouse_display_driver.paint_mouse(GCURX, GCURY);  /* display the cursor */

    vbl_must_draw_mouse = 0;              /* disable VBL drawing routine */
    HIDE_CNT--;
}


/*
 * linea_mouse_hide
 *
 * This routine hides the mouse cursor if it has not already been hidden.
 *
 * Inputs:         None
 *
 * Outputs:
 *    hide_cnt = hide_cnt + 1
 *    vbl_must_draw_mouse = 0
 */
void linea_mouse_hide(void)
{
    /*
     * Increment the counter for the number of hide operations performed.
     * If this is the first one then remove the cursor from the screen.
     * If not then do nothing, because the cursor wasn't on the screen.
     */
    HIDE_CNT += 1;              /* increment it */
    if (HIDE_CNT == 1) {        /* if cursor was not hidden... */
    
        vbl_must_draw_mouse = 0;               /* disable VBL drawing routine */
        mouse_display_driver.unpaint_mouse();  /* remove the cursor from screen */
    }
}


void linea_mouse_transform(void)
{
    linea_mouse_set_form((const MFORM *)INTIN);
}


/* copies src mouse form to dst mouse sprite, constrains hotspot
 * position & colors and maps colors
 */
void linea_mouse_set_form(const MFORM *src)
{
    mouse_flag += 1;            /* disable updates while redefining cursor */

    mouse_display_driver.set_mouse_cursor(src);

    mouse_flag -= 1;                    /* re-enable mouse drawing */
}
