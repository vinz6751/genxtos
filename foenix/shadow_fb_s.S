
.EQU A2560U_DIRTY_CELLS_SIZE, 256 // Needs to be the same as in shadow_fb.c
                                   // The smaller this is, the higher MAX_CELLS_RENDERED_PER_FRAME needs to be.
.EQU DIRTY_CELLS_SIZE_SIZEOF, A2560U_DIRTY_CELLS_SIZE*4

    // Exports
    .GLOBAL _a2560u_sfb_mark_cell_dirty
    .GLOBAL a2560u_sfb_copy_fb_to_vram

    // Imports
    .GLOBAL _a2560u_sfb_dirty_cells
    .GLOBAL _a2560u_bios_vram_fb
    .GLOBAL _a2560u_sfb_line_size_in_bytes
    .GLOBAL _a2560u_sfb_text_cell_height


#define DISABLE_VIDEO_ENGINE_DURING_COPY 0// That doesn't seem to bring any visible performance, and may cause flicker

a2560u_sfb_copy_fb_to_vram: // Copies our SRAM buffer to video ram

// Allow to limit the number of cells rendered per frame, to avoid spending too much time. -1 means unlimited.
// If it's not enough the dirty cells array may be too small and we loose data. If it's too high, we may spend
// too much time copying data and miss lower-priority interrupts (e.g. keyboard/mouse).
// Note: this doesn't apply to full screen refresh
#define MAX_CELLS_RENDERED_PER_FRAME 20

#if MAX_CELLS_RENDERED_PER_FRAME != -1
    move.l  #MAX_CELLS_RENDERED_PER_FRAME-1,d6 // max number of characters we render per frame
#endif

#if DISABLE_VIDEO_ENGINE_DURING_COPY
    move.l  0xb40000,d0
    move.l  d0,-(sp)
    //andi.l  #~0x-30,d0
    //ori.l   #0x80,d0
    move.l  d0,0xb40000
#endif

    lea     _a2560u_sfb_dirty_cells,a0
    tst.w   4(a0)           // Test "full_copy" to know if we should copy the whole frame buffer to VRAM ?
    jne     copy_whole_screen  // Yes -> go there
    // Otherwise process the list of dirty cells from the ring buffer  
    move.w  2(a0),d0        // Get reader
    cmp.w   (a0),d0         // Compare with writer
    jeq     cell_copy_done  // Nothing to read -> end
copy_cell_to_vram:
    // Cache information in registers
    movea.l _a2560u_bios_vram_fb,a3
    move.l  _a2560u_sfb_addr,d2    // Cache
    move.w  _a2560u_sfb_text_cell_height,d4    // Cache. d3 is the actual decounter used in the loop
    subq.w  #1,d4           // For the dbra of cell height
    clr.l   d5
    move.w  _a2560u_sfb_line_size_in_bytes,d5    // Cache for moving to next line of the frame buffer
cell_copy:
    addq.w  #4,d0           // Advance reader and wrap at 256 (offset 1024)
    andi.w  #DIRTY_CELLS_SIZE_SIZEOF-1,d0
    move.l  6(a0,d0),a1     // Get dirty cell address (in RAM frame buffer)
    movea.l a1,a2           // Compute target VRAM address
    suba.l  d2,a2           // d2: offset of dirty cell in frame buffer
    adda.l  a3,a2
    move.w  d4,d3           // Prepare line decounter
copy_cell_to_fb:
    // Copy one row (8 bytes)
    // TODO copy 32bits at a time once possible (with current flash, it's not)
    //.rept 16 // Font height
    move.w  0(a1),0(a2)
    move.w  2(a1),2(a2)
    move.w  4(a1),4(a2)
    move.w  6(a1),6(a2)
    adda.l  d5,a1   // Next line
    adda.l  d5,a2
    //.endr
    dbra    d3,copy_cell_to_fb // If we're short on ROM space or the font is not 16pixcels high we can use dbra instead of .rept
#if MAX_CELLS_RENDERED_PER_FRAME != 1
    dbra    d6,cell_copy_more
    jbra    cell_copy_done
#endif
cell_copy_more:
    cmp.w   (a0),d0    // More to read ?
    jne     cell_copy  // If so, nothing more to read
cell_copy_done:
#if DISABLE_VIDEO_ENGINE_DURING_COPY
    move.l  (sp)+,0xb40000 // restore previous state of VICKY control register
#endif
    move.w  d0,2(a0)   // Update reader
    rts

copy_whole_screen:
    // TODO Use DMA once available
    // Copy the whole SRAM frame buffer to VRAM
    move.w  (a0),2(a0)          // Flush the ring buffer (reader <- writer)
    move.w  #0,4(a0)            // Put down the flag to force full copy, as we're doing it now
    move.l  _a2560u_sfb_size,d0
    movea.l  _a2560u_sfb_addr,a1       // Source
    movea.l _a2560u_bios_vram_fb,a3  // Destination
    lsr.l   #5,d0               // We unroll the loop to copy 32 bytes at a time
fbcpy: // We can only do movem.w not movem.l as the later doesn't work (yet ?)
    .rept 4
    movem.w (a1)+,d0-d7/a0/a2/a4-a6
    movem.w d0-d7/a0/a2/a4-a6,(a3)
    lea     13*2(a3),a3
    .endr
    movem.w (a1)+,d0-d7/a0/a2/a4-a5 // We do one less register here so an iteration is 128 bytes.
    movem.w d0-d7/a0/a2/a4-a5,(a3)  // As all resolutions have a screensize multiple of 128,
    lea     12*2(a3),a3             // That ensures we won't copy beyond the end of the FB in VRAM.
    cmpa.l  #0xc4b000,a3            // TODO BUG FIXME don't hardcode end of FB
    bmi.s   fbcpy
#if DISABLE_VIDEO_ENGINE_DURING_COPY
     move.l  (sp)+,0xb40000 // restore previous state of VICKY control register
#endif
    rts


_a2560u_sfb_mark_cell_dirty:
    // Add the given text cell address to the ring buffer of dirty cells
    lea     _a2560u_sfb_dirty_cells,a0
    move.w  (a0),d0     // Writer
    addq.w  #4,d0       // Next position
    andi.w  #DIRTY_CELLS_SIZE_SIZEOF-1,d0   // Wrap if needed
    cmp.w   2(a0),d0    // Writer = Reader means no more space in buffer ?
    bne.s   addtobfr    // There is space
    move.w  #-1,4(a0)   // No more space, so refresh full screen
    rts
addtobfr:
    move.l  4(sp),6(a0,d0)
    move.w  d0,(a0)     // write writer
    rts
