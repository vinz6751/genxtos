#ifndef XBIOS_TT_H
#define XBIOS_TT_H

#include "emutos.h"

#if CONF_WITH_TT_SHIFTER

WORD esetshift(WORD mode);
WORD egetshift(void);
WORD esetbank(WORD bank);
WORD esetcolor(WORD index,UWORD color);
WORD esetpalette(WORD index,WORD count,UWORD *rgb);
WORD egetpalette(WORD index,WORD count,UWORD *rgb);
WORD esetgray(WORD mode);
WORD esetsmear(WORD mode);

#endif /* CONF_WITH_TT_SHIFTER */

#endif
