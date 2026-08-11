/*
 * vdi_raster_bitplane_blit_glyph_body.c - bitplane single-glyph blit body
 *
 * Include-only fragment: do NOT add to vdi_src in the Makefile.
 * #include inside a function only.
 *
 * Used from:
 *   vdi_textblit.c          text_blt() per-character hot path
 *   vdi_raster_bitplane.c   thin wrapper for VDI_RASTER_DRIVER.blit_glyph
 *
 * text_blt() calls this once per character.  The body must be included
 * directly in text_blt(), not only behind a vtable function, so the
 * compiler can inline it into the hot path.
 *
 * Required in scope before #include:
 *   LOCALVARS *bp_vars     glyph locals (see vdi_textblit.h)
 *   Line-A globals         TEXTFG, SOURCEX, SOURCEY, DELX, DELY, DESTX, DESTY
 *   Screen globals         v_planes, v_planes_shift, v_bas_ad, v_lin_wr
 *   normal_blit()        declared in vdi_textblit.h
 */
{
    LONG offset;

    bp_vars->forecol = TEXTFG;
    bp_vars->ambient = 0;          /* logically TEXTBG, but that isn't set up by the VDI */
    bp_vars->nbrplane = v_planes;
    bp_vars->nextwrd = bp_vars->nbrplane * (WORD)sizeof(WORD);
    bp_vars->height = bp_vars->DELY;
    bp_vars->width = bp_vars->DELX;

    /*
     * calculate the starting address for the character to be copied
     */
    bp_vars->tsdad = SOURCEX & 0x000f; /* source dot address */
    offset = (SOURCEY+bp_vars->DELY-1) * (LONG)bp_vars->s_next
             + ((SOURCEX >> 3) & ~1);
    bp_vars->sform += offset;
    bp_vars->s_next = -bp_vars->s_next;   /* we draw from the bottom up */

    /*
     * calculate the screen address
     *
     * note that the casts below allow the compiler to generate a mulu
     * instruction rather than calling _mulsi3(): this by itself speeds
     * up plain text output by about 3% ...
     */
    bp_vars->tddad = bp_vars->DESTX & 0x000f;
    bp_vars->dform = v_bas_ad;
    bp_vars->dform += (bp_vars->DESTX&0xfff0)>>v_planes_shift; /* add x coordinate part of addr */
    bp_vars->dform += (UWORD)(bp_vars->DESTY+bp_vars->DELY-1) * (ULONG)v_lin_wr; /* add y coordinate part of addr */
    bp_vars->d_next = -v_lin_wr;

    normal_blit(bp_vars+1, bp_vars->sform, bp_vars->dform);  /* call assembler helper function */
}
