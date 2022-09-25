    .GLOBL _run_showmouse
    .GLOBL _run_hidemouse
    .GLOBL _a2560u_sfb_dirty_cells
    .GLOBL _v_bas_ad
    .text
_run_showmouse:
    movem.l d1-a6,-(sp)

    .dc.w   0xa000     // Line A
    move.l  a0,lineaprms
    move.l  a2,lineavars

    move.l  lineaprms,a0      // Pointer parameter block LINEA
    move.l  8(a0),a1          // Address of intin
    move.w  #0,(a1)           // intin[0] = show mouse now
    .dc.w   0xa009            // Show mouse


    movem.l (sp)+,d1-a6
    moveq   #0,d0
    rts

_run_hidemouse:
    movem.l d1-a6,-(sp)
    .dc.w   0xa00a     // Hide mouse
    movem.l (sp)+,d1-a6
    moveq   #0,d0
    rts

// For later...
    // Put pixel
    #if 0
    move.l    lineaprms,a0      // Pointer parameter block LINEA
    move.l    8(a0),a1          // Address of intin
    move.w    #4,(a1)           // Colour in intin[0]
    move.l    12(a0),a1         // Address of ptsin
    move.w    #320,(a1)         // X-coordinate in ptsin[0]
    move.w    #200,2(a1)        // Y-coordinate in ptsin[1]
    .dc.w     0xA001            // Line-A opcode

    move.l  _v_bas_ad,a0
    adda.l   100 + 640*60,a0
    move.b   #1,(a0)

    move.w  #-1,_a2560u_sfb_dirty_cells+4 // DEBUG: force full screen redraw
    #endif

    .bss
lineavars:  ds.l 1
lineaprms:  ds.l 1