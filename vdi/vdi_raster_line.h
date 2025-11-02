#ifndef _RASTER_LINE_H
#define _RASTER_LINE_H

#include "emutos.h"
#include "has.h"        /* for blitter-related items */
#include "intmath.h"
#include "asm.h"
#include "aesext.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "blitter.h"
#include "tosvars.h"
#include "biosext.h"    /* for cache control routines */
#include "lineavars.h"
#include "vdi_inline.h"

#if CONF_WITH_BLITTER
#if CONF_WITH_VDI_VERTLINE
void hwblit_vertical_line(const Line *line, WORD wrt_mode, UWORD color);
#endif
void hwblit_rect_nonstd(const VwkAttrib *attr, const Rect *rect, UWORD *addr);
void hwblit_rect_common(const VwkAttrib *attr, const Rect *rect);
#endif

#if CONF_WITH_VDI_16BIT
void OPTIMIZE_SMALL swblit_rect_common16(const VwkAttrib *attr, const Rect *rect);
#endif
void OPTIMIZE_SMALL swblit_rect_common(const VwkAttrib *attr, const Rect *rect);

#if CONF_WITH_VDI_16BIT
void draw_line16(const Line *line, WORD wrt_mode, UWORD color);
#endif
void draw_line(const Line *line, WORD wrt_mode, UWORD color);

#if CONF_WITH_VDI_VERTLINE
#if CONF_WITH_VDI_16BIT
void swblit_vertical_line16(const Line *line, WORD wrt_mode, UWORD color);
#endif
void vertical_line(const Line *line, WORD wrt_mode, UWORD color);
#endif


#endif /* _RASTER_LINE_H */
