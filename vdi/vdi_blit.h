/*
 * vdi_blit.h - types shared by the VDI raster/blit code
 *
 * These declarations used to live inside vdi_raster.c.  They are published
 * here so that framebuffer-format-specific drivers (see vdi_raster_driver.h)
 * can implement the raster entries of VDI_RASTER_DRIVER.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _VDI_BLIT_H
#define _VDI_BLIT_H

#include "emutos.h"

#ifdef __mcoldfire__
#define ASM_BLIT_IS_AVAILABLE   0   /* assembler routine does not support ColdFire */
#else
#define ASM_BLIT_IS_AVAILABLE   1   /* may use m68k assembler fast_bit_blt routine */
#endif

/* bitblt modes */
#define BM_ALL_WHITE   0
#define BM_S_AND_D     1
#define BM_S_AND_NOTD  2
#define BM_S_ONLY      3
#define BM_NOTS_AND_D  4
#define BM_D_ONLY      5
#define BM_S_XOR_D     6
#define BM_S_OR_D      7
#define BM_NOT_SORD    8
#define BM_NOT_SXORD   9
#define BM_NOT_D      10
#define BM_S_OR_NOTD  11
#define BM_NOT_S      12
#define BM_NOTS_OR_D  13
#define BM_NOT_SANDD  14
#define BM_ALL_BLACK  15

/* flag:1 SOURCE and PATTERN   flag:0 SOURCE only */
#define PAT_FLAG        16

/* 76-byte line-A BITBLT struct passing parameters to bitblt */
struct blit_frame {
    WORD b_wd;          /* +00 width of block in pixels */
    WORD b_ht;          /* +02 height of block in pixels */
    WORD plane_ct;      /* +04 number of consecutive planes to blt */
    UWORD fg_col;       /* +06 foreground color (logic op table index:hi bit) */
    UWORD bg_col;       /* +08 background color (logic op table index:lo bit) */
    UBYTE op_tab[4];    /* +10 logic ops for all fore and background combos */
    WORD s_xmin;        /* +14 minimum X: source */
    WORD s_ymin;        /* +16 minimum Y: source */
    UWORD * s_form;     /* +18 source form base address */
    WORD s_nxwd;        /* +22 offset to next word in line  (in bytes) */
    WORD s_nxln;        /* +24 offset to next line in plane (in bytes) */
    WORD s_nxpl;        /* +26 offset to next plane from start of current plane */
    WORD d_xmin;        /* +28 minimum X: destination */
    WORD d_ymin;        /* +30 minimum Y: destination */
    UWORD * d_form;     /* +32 destination form base address */
    WORD d_nxwd;        /* +36 offset to next word in line  (in bytes) */
    WORD d_nxln;        /* +38 offset to next line in plane (in bytes) */
    WORD d_nxpl;        /* +40 offset to next plane from start of current plane */
    UWORD * p_addr;     /* +42 address of pattern buffer   (0:no pattern) */
    WORD p_nxln;        /* +46 offset to next line in pattern  (in bytes) */
    WORD p_nxpl;        /* +48 offset to next plane in pattern (in bytes) */
    WORD p_mask;        /* +50 pattern index mask */

    /* these frame parameters are internally set */
    WORD p_indx;        /* +52 initial pattern index */
    UWORD * s_addr;     /* +54 initial source address */
    WORD s_xmax;        /* +58 maximum X: source */
    WORD s_ymax;        /* +60 maximum Y: source */
    UWORD * d_addr;     /* +62 initial destination address */
    WORD d_xmax;        /* +66 maximum X: destination */
    WORD d_ymax;        /* +68 maximum Y: destination */
    WORD inner_ct;      /* +70 blt inner loop initial count */
    WORD dst_wr;        /* +72 destination form wrap (in bytes) */
    WORD src_wr;        /* +74 source form wrap (in bytes) */
};

/* Raster definitions */
typedef struct {
    void *fd_addr;
    WORD fd_w;
    WORD fd_h;
    WORD fd_wdwidth;
    WORD fd_stand;
    WORD fd_nplanes;
    WORD fd_r1;
    WORD fd_r2;
    WORD fd_r3;
} MFDB;

#if ASM_BLIT_IS_AVAILABLE
void fast_bit_blt(struct blit_frame *blit_info);    /* defined in vdi_blit.S */
#endif

#if CONF_WITH_BLITTER || !ASM_BLIT_IS_AVAILABLE
void bit_blt(struct blit_frame *blit_info);         /* defined in vdi_raster.c */
#endif

/*
 * perform a blit using the fastest available implementation
 *
 * we call the assembler version if we're not on ColdFire and either
 * (a) the blitter isn't configured, or
 * (b) it's configured but not available.
 */
void blit_frame_copy(struct blit_frame *info);

/*
 * shared VDI_RASTER_DRIVER entries for planar device-dependent forms
 * (see vdi_raster.c)
 */
BOOL vdi_setup_forms_planar(struct blit_frame *info, const MFDB *src, const MFDB *dst);
void vdi_transform_form_planar(MFDB *src_mfdb, MFDB *dst_mfdb);
void vdi_copy_raster_opaque_planar(struct blit_frame *info);
void vdi_copy_raster_transparent_planar(struct blit_frame *info);

#if CONF_WITH_VDI_16BIT
/* packed 16-bit raster copy, in vdi_raster_truecolor.c */
void vro_cpyfm16(struct blit_frame *info);
void vrt_cpyfm16(struct blit_frame *info);
#endif

#endif /* _VDI_BLIT_H */
