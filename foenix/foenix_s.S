
    // Exports
    .global _vicky_chan_a_logger_init
    .global _kprintf_outc_foenix_channel_a

    // Imports
    .extern _MSX_CP437_8x8_bin // 8x8 font to use
    .extern _stktop

    .text

_vicky_chan_a_logger_init:
    move.l #bus_error,0x8
    move.l #address_error,0xc
    
 move.w #0x2700,sr
        lea 0xfec40000,a0        // Vicky channel a
        move.l #0x00080801,4(a0) // Border enabled, 8 pixels
        move.l #0x00000088,8(a0) // Border color
        move.l #0x112233,0xc(a0) // Background
        move.l #0,0x10(a0)       // No cursor, 8x8 font
        move.l #0,0x14(a0)       // Cursor position
        move.l #0,0x18(a0)       // Line 0 & 1 interrupt
        move.l #0,0x1c(a0)       // Line 0 & 1 interrupt
        move.l #0x090808ff,0x20(a0) // Font manager 0
        // Font manager 1 ?

        lea 0xfec60000,a0 // Blank text memory
        move.w #0x4000-1,d0
        moveq  #0,d1
.lp1:   move.b d1,(a0)+
        dbra    d0,.lp1

        lea 0xfec68000,a0 // Blank text color
        move.w #0x4000-1,d0
        moveq  #0x00,d1
.lp2:   move.b d1,(a0)+
        dbra    d0,.lp2

        lea 0xfec6c400,a0 // Foreground colours palette
        move.l #0x00ffffff,(a0) // couleur 0
        lea 0xfec6c440,a0 // Backgroundcolours
        move.l #0,(a0) // couleur 0

        lea 0xfec48000,a0 // Charge font memory
        lea _MSX_CP437_8x8_bin,a1
        move.w #0x1000-1,d0
.lp3:   move.b (a1)+,(a0)+
        dbra d0,.lp3

#if 0
        lea debug_output_msg,a1
.lp4:   move.b (a1)+,d0
        beq.s  .end
        //move.w  d0,-(sp)
        //bsr.s  _kprintf_outc_foenix_channel_a
        //addq.l  #2,sp
        bra.s  .lp4
#endif
.end:
    rts


bus_error: // Red
        move.l #0x00ff0000,0xfec40008
        bra.s wait

address_error: // Green
        move.l #0x0000ff00,0xfec40008
        bra.s wait

wait:   nop
    bra.s wait

_kprintf_outc_foenix_channel_a: // Log le charactère sur la pile
        // pourri a0,d1.w,d2,d3
        move.w 4(sp),d0
        //move.b #'#',d0
        movem.l d1-d3/a0-a2,-(sp)

        lea 0xfec60000,a0 // Channel A text memory
        move.l 0xfec40000+0x14,d2 // Cursor position
        move.w d2,d1 // d1: x
        swap   d2    // d2: y
        cmpi.b #'\n',d0
        bne.s .notlf 
        addq.w #1,d2 // y++ TODO: scroll si nécessaire
        clr.w  d1 // x=0, traite \n comme \r\n
        bra.s .updcur
.notlf: cmpi.b #'\r',d0
        bne.s  .notcr
        clr.w  d1
        bra.s .updcur
.notcr: // Caractère affichable
        clr.l  d3
.out:   move.w d2,d3
        mulu.w #100,d3
        add.w  d1,d3
        move.b d0,(a0,d3) // Ecrit à la position du curseur
        // Avance le curseur
        addq.w #1,d1
        cmpi.w #100,d1
        bne.s  .noXwrap
        clr.w  d1
        addq.w #1,d2
.noXwrap:
.updcur:
        // TODO: scroll si nécessaire
        // Stocke nouvelle position du curseur
        swap   d1    // XX--
        move.w d2,d1 // XXYY
        swap   d1    // YYXX
        move.l d1,0xfec40000+0x14

        movem.l (sp)+,d1-d3/a0-a2
        rts


scroll: // Scroll l'écran (800x600)
        // a0,a1,d0.w,d1.w
        lea 0xfec60000+800/8,a0 // Deuxième ligne
        lea 0xfec68000+800/8,a1 // Couleurs (2ème ligne)
        move.w #600/8-1-1,d0    // Décompteur de lignes
.line:  move.w #100-1,d1        // Décompteur de colonnes
.col:   move.b (a0,d1),-100(a0,d1) // Copie vers ligne du dessus
        move.b (a1,d1),-100(a1,d1) // Copie vers ligne du dessus
        dbra   d1,.col
        add.w  #100,a0
        add.w  #100,a1
        dbra   d0,.line
        // TODO: effacer dernière ligne
        rts

        // On met ça dans TEXT et pas DATA parce que EmuTOS n'a pas de segment
        // DATA fonctionnel (c'est un pb quand il s'exécute en ROM)
debug_output_msg:
    .even
    .asciz "FOENIX debug output\r\nCoucou de la deuxieme ligne"
    .even

