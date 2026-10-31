/*
 * desk_rsc_pack.c - expand packed EmuDesk resource into RAM
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "string.h"
#include "obdefs.h"
#include "gemdos.h"
#include "desk_rsc.h"
#include "desk_rsc_data.h"

OBJECT *desk_rs_obj;
OBJECT *desk_rs_trees[RS_NTREE];
TEDINFO desk_rs_tedinfo[RS_NTED];
BITBLK desk_rs_bitblk[RS_NBB];
const char *desk_rs_fstr[DESK_RS_NFSTR];

/*
 * Rebuild ob_next/ob_head/ob_tail for the subtree at absolute index i
 * into desk_rs_obj[].  Links are TREE-LOCAL (relative to `root`), as
 * AES objc_* walks tree[obj].ob_next with tree = &desk_rs_obj[root].
 * Returns the absolute index just past the subtree.  Child counts are
 * in preorder, matching tools/deskpack.py.
 */
static WORD rec_links(WORD i, WORD root)
{
    UBYTE c = desk_rs_nchild[i];
    WORD p;
    WORD prev;
    WORD ci;

    if (c == 0)
    {
        desk_rs_obj[i].ob_head = -1;
        desk_rs_obj[i].ob_tail = -1;
        return i + 1;
    }

    p = i + 1;
    prev = -1;
    for (ci = 0; ci < (WORD)c; ci++)
    {
        if (ci)
            desk_rs_obj[prev].ob_next = p - root;
        prev = p;
        p = rec_links(p, root);
    }
    desk_rs_obj[i].ob_head = (i + 1) - root;
    desk_rs_obj[i].ob_tail = prev - root;
    desk_rs_obj[prev].ob_next = i - root;
    return p;
}

static char *pool_str(char *pool_base, UWORD idx)
{
    return pool_base + desk_rs_pooloff[idx];
}

int desk_rs_init(void)
{
    LONG nbytes;
    char *block;
    char *pool;
    WORD *bm;
    char *tebuf;
    WORD i;
    const UBYTE *tp;

    /* Pool may be odd; pad so the WORD bitmap is 68000-aligned. */
    {
        LONG pool_pad = ((LONG)DESK_RS_POOLSZ + 1) & ~1L;

        nbytes = (LONG)RS_NOBS * (LONG)sizeof(OBJECT)
               + pool_pad
               + (LONG)DESK_RS_BMWORDS * (LONG)sizeof(WORD)
               + (LONG)DESK_RS_TEBUFSZ;

        block = dos_alloc_anyram(nbytes);
        if (!block)
            return -1;

        desk_rs_obj = (OBJECT *)block;
        pool = block + (LONG)RS_NOBS * (LONG)sizeof(OBJECT);
        bm = (WORD *)(pool + pool_pad);
        tebuf = (char *)bm + (LONG)DESK_RS_BMWORDS * (LONG)sizeof(WORD);
    }

    memcpy(pool, desk_rs_pool, DESK_RS_POOLSZ);
    memcpy(bm, desk_rs_bmdata, (LONG)DESK_RS_BMWORDS * (LONG)sizeof(WORD));
    bzero(tebuf, DESK_RS_TEBUFSZ);

    for (i = 0; i < RS_NTREE; i++)
    {
        WORD root = desk_rs_treestart[i];

        rec_links(root, root);
        desk_rs_obj[root].ob_next = -1;
        desk_rs_trees[i] = &desk_rs_obj[root];
    }

    for (i = 0; i < RS_NOBS; i++)
    {
        OBJECT *o = &desk_rs_obj[i];
        UWORD typ = desk_rs_otype[i];
        UBYTE spec = desk_rs_ospec[i];
        const UBYTE *xy = &desk_rs_xylo[i * 4];

        o->ob_type = typ;
        o->ob_flags = desk_rs_flagdict[desk_rs_oflagi[i]];
        o->ob_state = desk_rs_ostate[i];
        o->ob_x = xy[0];
        o->ob_y = xy[1];
        o->ob_width = xy[2];
        o->ob_height = xy[3];

        switch (typ & 0xff)
        {
        case G_BOX:
        case G_IBOX:
            o->ob_spec = desk_rs_immdict[spec];
            break;
        case G_FTEXT:
        case G_FBOXTEXT:
            o->ob_spec = (LONG)&desk_rs_tedinfo[spec];
            break;
        case G_IMAGE:
            o->ob_spec = (LONG)&desk_rs_bitblk[spec];
            break;
        default:
            o->ob_spec = (LONG)pool_str(pool, spec);
            break;
        }
    }

    for (i = 0; i < DESK_RS_NHI; i++)
    {
        const UBYTE *e = &desk_rs_xyhi[i * 4];
        WORD oi = e[0] | (e[1] << 8);
        UBYTE field = e[2];
        UWORD hi = (UWORD)e[3] << 8;
        OBJECT *o = &desk_rs_obj[oi];

        switch (field)
        {
        case 0: o->ob_x |= hi; break;
        case 1: o->ob_y |= hi; break;
        case 2: o->ob_width |= hi; break;
        default: o->ob_height |= hi; break;
        }
    }

    tp = desk_rs_tedpack;
    for (i = 0; i < RS_NTED; i++)
    {
        TEDINFO *t = &desk_rs_tedinfo[i];
        UWORD tmpl = tp[0] | (tp[1] << 8);
        UWORD valid = tp[2] | (tp[3] << 8);
        UWORD txtlen;

        t->te_ptmplt = pool_str(pool, tmpl);
        t->te_pvalid = pool_str(pool, valid);
        t->te_font = IBM;
        t->te_junk1 = 0;
        t->te_just = tp[4];
        t->te_color = tp[5] | (tp[6] << 8);
        t->te_junk2 = 0;
        t->te_thickness = (SBYTE)tp[7];
        txtlen = tp[8];
        t->te_txtlen = txtlen;
        t->te_tmplen = (WORD)strlen(t->te_ptmplt) + 1;
        t->te_ptext = tebuf;
        tebuf += txtlen;
        tp += 9;
    }

    for (i = 0; i < DESK_RS_NFSTR; i++)
        desk_rs_fstr[i] = pool_str(pool, desk_rs_fstridx[i]);

    for (i = 0; i < RS_NBB; i++)
    {
        const WORD *info = &desk_rs_bminfo[i * 5];

        desk_rs_bitblk[i].bi_pdata = bm;
        desk_rs_bitblk[i].bi_wb = info[0];
        desk_rs_bitblk[i].bi_hl = info[1];
        desk_rs_bitblk[i].bi_x = info[2];
        desk_rs_bitblk[i].bi_y = info[3];
        desk_rs_bitblk[i].bi_color = info[4];
    }

    return 0;
}
