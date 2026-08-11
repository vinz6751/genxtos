/*
 * raster_driver_atari_bitplanes_blit_glyph_body.c - bitplane single-glyph blit body
 *
 * Include-only fragment: do NOT add to vdi_src in the Makefile.
 * #include inside a function only.
 *
 * Used from:
 *   vdi_textblit.c                    text_blt() per-character hot path
 *   raster_driver_atari_bitplanes.c   thin wrapper for VDI_RASTER_DRIVER.blit_glyph
 *
 * text_blt() calls this once per character.  The body must be included
 * directly in text_blt(), not only behind a vtable function, so the
 * compiler can inline it into the hot path.
 *
 * Required in scope before #include:
 *   LOCALVARS *bitplanes_vars     glyph locals (see vdi_textblit.h)
 *   Line-A globals         TEXTFG, SOURCEX, SOURCEY, DELX, DELY, DESTX, DESTY
 *   Screen globals         v_planes, v_planes_shift, v_bas_ad, v_lin_wr
 *   normal_blit()        declared in vdi_textblit.h
 */
{
    LONG offset;

    bitplanes_vars->forecol = TEXTFG;
    bitplanes_vars->ambient = 0;          /* logically TEXTBG, but that isn't set up by the VDI */
    bitplanes_vars->nbrplane = v_planes;
    bitplanes_vars->nextwrd = bitplanes_vars->nbrplane * (WORD)sizeof(WORD);
    bitplanes_vars->height = bitplanes_vars->DELY;
    bitplanes_vars->width = bitplanes_vars->DELX;

    /*
     * calculate the starting address for the character to be copied
     */
    bitplanes_vars->tsdad = SOURCEX & 0x000f; /* source dot address */
    offset = (SOURCEY+bitplanes_vars->DELY-1) * (LONG)bitplanes_vars->s_next
             + ((SOURCEX >> 3) & ~1);
    bitplanes_vars->sform += offset;
    bitplanes_vars->s_next = -bitplanes_vars->s_next;   /* we draw from the bottom up */

    /*
     * calculate the screen address
     *
     * note that the casts below allow the compiler to generate a mulu
     * instruction rather than calling _mulsi3(): this by itself speeds
     * up plain text output by about 3% ...
     */
    bitplanes_vars->tddad = bitplanes_vars->DESTX & 0x000f;
    bitplanes_vars->dform = v_bas_ad;
    bitplanes_vars->dform += (bitplanes_vars->DESTX&0xfff0)>>v_planes_shift; /* add x coordinate part of addr */
    bitplanes_vars->dform += (UWORD)(bitplanes_vars->DESTY+bitplanes_vars->DELY-1) * (ULONG)v_lin_wr; /* add y coordinate part of addr */
    bitplanes_vars->d_next = -v_lin_wr;

    normal_blit(bitplanes_vars+1, bitplanes_vars->sform, bitplanes_vars->dform);  /* call assembler helper function */
}
