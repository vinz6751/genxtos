/*
 * vdi_mouse.c
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2022 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "asm.h"
#include "biosbind.h"
#include "xbiosbind.h"
#include "linea.h"
#include "obdefs.h"
#include "aesdefs.h"
#include "aesext.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "tosvars.h"
#include "biosext.h"
#include "lineavars.h"
#include "a2560u_bios.h"
#include "../foenix/vicky2.h"
#if WITH_AES
#include "../aes/aesstub.h"
#endif


/* prototypes */
static void mform_color_validator(const MFORM *src, MCDB *dst);

/* prototypes for functions in vdi_asm.S */



#if CONF_WITH_EXTENDED_MOUSE
void wheel_int(void);           /* wheel interrupt routine */
void call_user_but(WORD status);/* call user_but from C */
void call_user_wheel(WORD wheel_number, WORD wheel_amount); /* call user_wheel from C */

/* pointers to callbacks called from vdi_asm.S */
PFVOID user_wheel;  /* user mouse wheel vector provided by vdi_vex_wheelv() */
PFVOID old_statvec; /* original IKBD status packet routine */
#endif

#if !WITH_AES
/* The AES has all the mouse forms, which may even be customized.
 * If it's there we use it otherwise we fallback on the default arrow cursor
 * provided by the Line-A */

#include "mform.h"

#define default_mform() &arrow_mform
#endif


#if CONF_WITH_EXTENDED_MOUSE

/*
 * vdi_mousex_handler - Handle additional mouse buttons
 */
static void vdi_mousex_handler (WORD scancode)
{
    WORD old_buttons = MOUSE_BT;

    if (scancode == 0x37)      /* Mouse button 3 press */
        MOUSE_BT |= 0x04;
    else if (scancode == 0xb7) /* Mouse button 3 release */
        MOUSE_BT &= ~0x04;
    else if (scancode == 0x5e) /* Mouse button 4 press */
        MOUSE_BT |= 0x08;
    else if (scancode == 0xde) /* Mouse button 4 release */
        MOUSE_BT &= ~0x08;
    else if (scancode == 0x5f) /* Mouse button 5 press */
        MOUSE_BT |= 0x10;
    else if (scancode == 0xdf) /* Mouse button 5 release */
        MOUSE_BT &= ~0x10;

    if (MOUSE_BT != old_buttons)
        call_user_but(MOUSE_BT);

    if (scancode == 0x59)      /* Wheel up */
        call_user_wheel(0, -1);
    else if (scancode == 0x5a) /* Wheel down */
        call_user_wheel(0, 1);
    else if (scancode == 0x5c) /* Wheel left */
        call_user_wheel(1, -1);
    else if (scancode == 0x5d) /* Wheel right */
        call_user_wheel(1, 1);
}

#endif /* CONF_WITH_EXTENDED_MOUSE */

#if CONF_WITH_EXTENDED_MOUSE
/*
 * vdi_vex_wheelv: a Milan VDI extension
 *
 * This routine replaces the mouse wheel vector with the
 * address of a user-supplied routine.  The previous value
 * is returned so that it also may be called when the mouse
 * wheel is used.
 *
 * Inputs:
 *    contrl[7], contrl[8] - pointer to user routine
 *
 * Outputs:
 *    contrl[9], contrl[10] - pointer to old routine
 *
 */
void vdi_vex_wheelv(Vwk * vwk)
{
    ULONG_AT(&CONTRL[9]) = (ULONG) user_wheel;
    user_wheel = (PFVOID) ULONG_AT(&CONTRL[7]);
}
#endif

/*
 * vdimouse_init - Initializes the mouse (VDI part)
 *
 * entry:          none
 * exit:           none
 */
void vdimouse_init(void)
{
    /* Input must be initialized here and not in init_wk */
    loc_mode = 0;               /* default is request mode  */
    val_mode = 0;               /* default is request mode  */
    chc_mode = 0;               /* default is request mode  */
    str_mode = 0;               /* default is request mode  */

    /* Move in the default mouse form (presently the arrow) */    
    mform_color_validator((const MFORM*)default_mform(), &mouse_cdb); /* FIXME CHEAT, not sure how to inject an implementation-specific validator... */
    linea_mouse_set_form(default_mform());

#if CONF_WITH_EXTENDED_MOUSE
    {
        struct kbdvecs *kbd_vectors = (struct kbdvecs *)Kbdvbase();
        old_statvec = kbd_vectors->statvec;
        kbd_vectors->statvec = wheel_int;
        mousexvec = vdi_mousex_handler;
    }
#endif
}



/*
 * vdimouse_exit - deinitialize/disable mouse
 */
void vdimouse_exit(void)
{
    user_but = just_rts;
    user_mot = just_rts;
    user_cur = just_rts;
#if CONF_WITH_EXTENDED_MOUSE
    user_wheel = just_rts;
#endif



#if CONF_WITH_EXTENDED_MOUSE
    {
        struct kbdvecs *kbd_vectors = (struct kbdvecs *)Kbdvbase();
        kbd_vectors->statvec = old_statvec;
    }
#endif
}



static void mform_color_validator(const MFORM *src, MCDB *dst)
{
    WORD col;

    /* check/fix background color index */
    col = validate_color_index(src->mf_bg);
    dst->bg_col = MAP_COL[col];

    /* check/fix foreground color index */
    col = validate_color_index(src->mf_fg);
    dst->fg_col = MAP_COL[col];
}