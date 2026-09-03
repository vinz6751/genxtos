#ifndef XBIOS_TT_H
#define XBIOS_TT_H

#include "emutos.h"

#if CONF_WITH_TT_SHIFTER

WORD ARGS_ON_STACK esetshift(WORD mode);
WORD ARGS_ON_STACK egetshift(void);
WORD ARGS_ON_STACK esetbank(WORD bank);
WORD ARGS_ON_STACK esetcolor(WORD index,UWORD color);
WORD ARGS_ON_STACK esetpalette(WORD index,WORD count,UWORD *rgb);
WORD ARGS_ON_STACK egetpalette(WORD index,WORD count,UWORD *rgb);
WORD ARGS_ON_STACK esetgray(WORD mode);
WORD ARGS_ON_STACK esetsmear(WORD mode);

#endif /* CONF_WITH_TT_SHIFTER */

#endif
