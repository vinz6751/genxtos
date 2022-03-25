/*
 * OPL3 - Yamaha OPL3 support for the Foenix Retro Systems A2560U
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef OPL3_H
#define OPL3_H

#include <stdint.h>
#include <stdbool.h>

#if defined(__GNUC__) && !defined(PACKED)
# define __PACKED__ __attribute__((__packed__))
#else
    // TODO: add support for VBCC
#endif

#define OP_UNUSED 0xff

struct ops_2
{
    uint16_t carrier;
    uint16_t modulator;
};

struct ops_4
{
    uint16_t op1;
    uint16_t op2;
    uint16_t op3;
    uint16_t op4;
};

#ifndef OPL3_BASE
#define OPL3_BASE_L (0xB20000+0x200)
#define OPL3_BASE_H (0xB20000+0x300)

#endif

/* Association between operators in the three possible modes.
 * The index is the voice number */

/* 18 2-op voices
+------------+--------------------------------------------------------------+
| Channel    | 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17 |
+------------+--------------------------------------------------------------+
| Operator 1 | 0  1  2  6  7  8  12 13 14 18 19  20  24  25  26  30  31  32 |
| Operator 2 | 3  4  5  9  10 11 15 16 17 21 22  23  27  28  29  33  34  35 |
+------------+--------------------------------------------------------------+*/    

const struct ops_2 ops_2op[] = 
{
    {0,3}, {1,4}, {2,5}, {6,9}, {7,10}, {8,11}, {12,15}, {13,16}, /* 0-7 */
    {14,17}, {18,21}, {19,22}, {20,23}, {24,27}, {25,28}, {26,29}, {30,33}, /* 8-15 */
    {31,34}, {32,35} /* 16-17 */
};

/* 15 2-op voices and 5 drums
+------------+--------------------------------------------------------------+
| Channel    | 0  1  2  3  4  5  BD SD TT CY HH  9  10 11 12 13 14 15 16 17 |
+------------+--------------------------------------------------------------+
| Operator 1 | 0  1  2  6  7  8  12 16 14 17 13  18 19 20 24 25 26 30 31 32 |
| Operator 2 | 3  4  5  9  10 11 15              21 22 23 27 28 29 33 34 35 |
+------------+--------------------------------------------------------------+*/
const struct ops_2 ops_drums[] = 
{
    {0,3}, {1,4}, {2,5}, {6,9}, {7,10}, {8,11}, /* 0-5 */
    {12,15},            /* Bass drum */
    {16, OP_UNUSED},    /* Snare drum */
    {14, OP_UNUSED},    /* Tom */
    {17, OP_UNUSED},    /* Cymbal */
    {13, OP_UNUSED},    /* Hi-hat */
    {18,21}, {19,22}, {20,23}, {24,27}, {25,28}, {26,29}, {30,33}, /* 8-15 */
    {31,34}, {32,35}/* 16-17 */
};

/* 4-operator mode
+------------+---------------------------------------------------+
| Channel    | 0   1   2    6   7   8    9   10  11   15  16  17 |
+------------+---------------------------------------------------+
| Operator 1 | 0   1   2    12  13  14   18  19  20   30  31  32 |
| Operator 2 | 3   4   5    15  16  17   21  22  23   33  34  35 |
| Operator 3 | 6   7   8                 24  25  26              |
| Operator 4 | 9   10  11                27  28  29              |
+------------+---------------------------------------------------+*/
const struct ops_4 ops_4op[] = 
{
    {0,3,6,9}, {1,4,7,10}, {2,5,8,11}, /* 0-2 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 3 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 4 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 5 */    
    {12,15,OP_UNUSED,OP_UNUSED}, {13,16,OP_UNUSED,OP_UNUSED}, /* 6-7*/
    {14,17,OP_UNUSED,OP_UNUSED}, /* 8 */
    {18,21,24,27}, {19,22,25,28}, {20,23,26,29}, /* 9-11 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 12 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 13 */
    {OP_UNUSED,OP_UNUSED,OP_UNUSED,OP_UNUSED}, /* 14 */    
    {30,33,OP_UNUSED,OP_UNUSED}, {31,34,OP_UNUSED,OP_UNUSED}, /* 15-16 */
    {32,35,OP_UNUSED,OP_UNUSED}
};

/* Registers definition */
#define OPL3_TEST   0x01 /* Test register */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |       |WSE|   Test Register   |
   +---+---+---+---+---+---+---+---+
bit 5: Waveform Select Enable. If clear, all channels will use normal sine wave. If set, register E0-F5 (Waveform Select) contents will be used.
bits 0-4: Test Register. Must be reset to zero before any operation. */

#define OPL3_TIMER1 0x02 /* Upward 8 bit counter with a resolution of 80 usec. If an overflow occurs, the status register bit is set, and the preset value is loaded into the timer again. */
#define OPL3_TIMER2 0x03 /* Same as Timer 1, but with a resolution of 320 usec.  */

#define OPL3_IRQ    0x04
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |Rst|T1M|T2M|           |T2S|T1S|
   +---+---+---+---+---+---+---+---+
bit 7: IRQ-Reset. Resets timer and IRQ flags in status register. All other bits are ignored when this bit is set.
bit 6: Timer 1 Mask. If 1, status register is not affected in overflow.
bit 5: Timer 2 Mask. Same as above.
bit 1: Timer 2 Start. Timer on/off.
bit 0: Timer 1 Start. Same as above. */






 /* Four-operator enable */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |       |ChB|ChA|Ch9|Ch2|Ch1|Ch0|
   +---+---+---+---+---+---+---+---+
bit 5: Enable four-operator synthesis for channel pair 11 - 14 (decimal).
bit 4: Same as above for channel pair 10 - 13.
bit 3: Same as above for channel pair 9 - 12.
bit 2: Same as above for channel pair 2 - 5.
bit 1: Same as above for channel pair 1 - 4.
bit 0: Same as above for channel pair 0 - 3.

If reset to zero, OPL3 can produce 18 two-operator sounds at a time.
If nonzero, OPL3 produces four-operator sound in appropriate channel pair. */

#define OPL3_MODE   0x05 /* OPL3 Mode Enable */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |                           |OPL|
   +---+---+---+---+---+---+---+---+ */

#define OPL3_CSM    0x08
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |CSW|N-S|                       |
   +---+---+---+---+---+---+---+---+
bit 7: Composite sine wave mode on/off. All KEY-ON bits must be clear in order to use this mode. The card is unable to create any other sound when in CSW mode. (Unfortunately, I have no info how to use this mode :-< ).
bit 6: NOTE-SEL. Controls the split point of the keyboard. When 0, the keyboard split is the second bit from the bit 8 of the F-Number. When 1, the MSb of the F-Number is used. (???)*/

#define OPL3_AM_VID_EG_KSR_MULT 0x20 /* Tremolo/Vibrato/Sustain/KSR/Frequency Multiplication Factor */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |Tre|Vib|Sus|KSR| Multiplication|
   +---+---+---+---+---+---+---+---+
bit 7: Tremolo (Amplitude vibrato) on/off.
bit 6: Frequency vibrato on/off.
bit 5: Sound Sustaining. When 1, operator's output level will be held at its sustain level until a KEY-OFF is done.
bit 4: Envelope scaling (KSR) on/off. When 1, higher notes are shorter than lower notes.
bits 0-3: Frequency Multiplication Factor (MULTI). Operator's frequency is set to (see registers A0, B0) F-Number * Factor.
    +-------+--------+
    | MULTI | Factor |
    +-------+--------+
    |   0   |   1/2  |
    |   1   |    1   |
    |   2   |    2   |
    |   3   |    3   |
    |   4   |    4   |
    |   5   |    5   |
    |   6   |    6   |
    |   7   |    7   |
    |   8   |    8   |
    |   9   |    9   |
    |  10   |   10   |
    |  11   |   10   |
    |  12   |   12   |
    |  13   |   12   |
    |  14   |   15   |
    |  15   |   15   |
    +-------+--------+ */

#define OPL3_KS_OL  0x40 /* Key Scale Level / Output Level */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |  KSL  |      Output Level     |
   +---+---+---+---+---+---+---+---+
bits 6-7: Key Scale Level. Attenuates output level towards higher pitch:

		+-----+-------------+
		| KSL | Attenuation |
		+-----+-------------+
		|  0  | -           |
		|  1  | 1.5 dB/oct  |
		|  2  | 3.0 dB/oct  |
		|  3  | 6.0 dB/oct  |
		+-----+-------------+
bits 0-5: Output Level. Attenuates the operator output level. 0 is the loudest, 3F is the softest. In additive synthesis, varying the output level of any operator varies the volume of its corresponding channel. In FM synthesis, varying the output level of the carrier varies the volume of the channel. Varying the output of the modulator will change the frequency spectrum produced by the carrier.

The following table summarizes which operators' output levels should be updated when trying to change channel output level.

		+-------+------+------+------+------+
		| Mode  | Op 1 | Op 2 | Op 3 | Op 4 |
		+-------+------+------+------+------+
		|  AM   |  +   |  +   | N/A  | N/A  |
		|  FM   |  -   |  +   | N/A  | N/A  |
		| FM-FM |  -   |  -   |  -   |  +   |
		| AM-FM |  +   |  -   |  -   |  +   |
		| FM-AM |  -   |  +   |  -   |  +   |
		| AM-AM |  +   |  -   |  +   |  +   |
		+-------+------+------+------+------+*/

#define OPL3_ARDR 0x60 /* Attack rate / Decay rate */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |  Attack Rate  |  Decay Rate   |
   +---+---+---+---+---+---+---+---+
bits 4-7: Attack Rate. Determines the rising time for the sound. The higher the value, the faster the attack.
bits 0-3: Decay Rate. Determines the diminishing time for the sound. The higher the value, the shorter the decay. */

#define OPL3_SLRR 0x80 /* Sustain level / Release rate */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   | Sustain Level |  Release Rate |
   +---+---+---+---+---+---+---+---+

bits 4-7: Sustain Level. Determines the point at which the sound ceases to decay and chages to a sound having a constant level. The sustain level is expressed as a fraction of the maximum level. 0 is the softest and F is the loudest sustain level. Note that the Sustain-bit in the register 20-35 must be set for this to have an effect.
bits 0-3: Release Rate. Determines the rate at which the sound disappears after KEY-OFF. The higher the value, the shorter the release. */

#define OPL3_FN 0xA0 /* Frequency number */
/* Determines the pitch of the note. Highest bits of F-Number are stored in the register below. */

#define OPL3_KO_BN_FNH /* Key on / Block number, F-number hi bits */
/*  +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |       |KEY| Block Num.| Freq  |
   +---+---+---+---+---+---+---+---+
bit 5: KEY-ON. When 1, channel output is enabled.
bits 2-4: Block Number. Roughly determines the octave.
bits 0-1: Frequency Number. 2 highest bits of the above register.

The following formula is used to determine F-Number and Block:

  F-Number = Music Frequency * 2^(20-Block) / 49716 Hz

Note: In four-operator mode only the register value of Operators 1 and 2 is used, value of Operators 3 and 4 in this register is ignored. In other words: one channel uses only one frequency, block and KEY-ON value at a time, regardless whether it is a two- or four-operator channel. */

#define OPL3_TVPD 0xBD /* Tremolo depth, vibrato depth, percussion mode, BD/SD/TT/CY/HH key on */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |Tre|Vib|Per|BD |SD |TT |CY |HH |
   +---+---+---+---+---+---+---+---+
bit 7: Tremolo (Amplitude Vibrato) Depth. 0 = 1.0dB, 1 = 4.8dB.
bit 6: Frequency Vibrato Depth. 0 = 7 cents, 1 = 14 cents. A "cent" is 1/100 of a semi-tone.
bit 5: Percussion Mode. 0 = Melodic Mode, 1 = Percussion Mode.
bit 4: BD On. KEY-ON of the Bass Drum channel.
bit 3: SD On. KEY-ON of the Snare Drum channel.
bit 2: TT On. KEY-ON of the Tom-Tom channel.
bit 1: CY On. KEY-ON of the Cymbal channel.
bit 0: HH On. KEY-ON of the Hi-Hat channel.

Note: KEY-ON bits of channels 6, 7 and 8 must be clear and their F-Nums, Attack/Decay/Release rates, etc. must be set properly to use percussion mode. */

#define OPL3_FDBALG 0xC0 /* Feedback, algorithm select */
/* +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |       | R | L | FeedBack  |Syn|
   +---+---+---+---+---+---+---+---+
bit 5: Right Speaker Enable. When set, channel output goes to right speaker.
bit 4: Left Speaker Enable. When set, channel output goes to left speaker. At least one of these bits must be set to hear the channel.
These two bits can be used to realize sound "panning", but this method offers only three pan positions (left/center/right). These bits apply only to operators producing sound (Carriers). Modulators are not affected by their setting.
bits 1-3: FeedBack Modulation Factor. If 0, no feedback is present. If 1-7, operator 1 will send a portion of its output back into itself.

		+----------+--------+
		| FeedBack | Factor |
		+----------+--------+
		|    0     |   0    |
		|    1     |  n/16  |
		|    2     |  n/8   |
		|    3     |  n/4   |
		|    4     |  n/2   |
		|    5     |   n    |
		|    6     |  2.n   |
		|    7     |  4.n   |
		+----------+--------+

When in four-operator mode, the FeedBack value is used only by Operator 1, value of Operators 2, 3 and 4 is ignored.
bit 0: Synthesis Type. 1 = Additive synthesis, 0 = Frequency Modulation
In four-operator mode, there are two bits controlling the synthesis type. Both are the bit 0 of register C0, one of Operators 1 and 2 and the second of Operators 3 and 4.

		+--------+--------+--------+
		| Op 1&2 | Op 3&4 |  Type  |
		+--------+--------+--------+
		|    0   |  NONE  |   FM   |
		|    1   |  NONE  |   AM   |
		|    0   |    0   | FM-FM  |
		|    1   |    0   | AM-FM  |
		|    0   |    1   | FM-AM  |
		|    1   |    1   | AM-AM  |
		+--------+--------+--------+*/

#define OPL3_WAVE 0xE0 /* Wave select **/
/* *+-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
   |                   | WaveForm  |
   +---+---+---+---+---+---+---+---+
bits 0-2: WaveForm Select (WS):
    WaveForm 0: Sine
    WaveForm 1: Half-Sine
    WaveForm 2: Abs-Sine
    WaveForm 3: Pulse-Sine
    WaveForm 4: Sine - even periods only
    WaveForm 5: Abs-Sine - even periods only
    WaveForm 6: Square
    WaveForm 7: Derived Square
    Note: Bit 5 of register 01 must be set to use waveforms other than sine. Waveforms 4-7 are available only on OPL3. */

void opl3_init(void)
{

}

#ifndef R8
    #define R8(x) *((int8_t * volatile)(x))
#endif
 
struct __PACKED__ opl3_base_l_t {
    uint8_t dummy0;      /* 0x00 */
    uint8_t lsi_test;   /* 0x01 */
    uint8_t timer1;     /* 0x02 */    
    uint8_t timer2;     /* 0x03 */
    uint8_t rst_mt1_mt2_st2_st1; /* 0x04 */
    uint8_t dummy5;
    uint8_t dummy6;
    uint8_t dummy7;
    uint8_t nts;        /* 0x 08 */    
};

struct __PACKED__ opl3_base_h_t {
    uint8_t dummy0;      /* 0x00 */
    uint8_t lsi_test;   /* 0x01 */
    uint8_t dummy1;     /* 0x02 */    
    uint8_t dummy2;     /* 0x03 */
    uint8_t connectionsel; /* 0x04 */
    uint8_t new;
    uint8_t dummy6;
    uint8_t dummy7;
    uint8_t dummy8;     /* 0x 08 */    
};

struct __PACKED__ opl3_data_l_t {
    uint8_t am_biv_egt_ksr_mult[22];
    uint8_t unused_36_3f[10];
    uint8_t ksl_tl[22];
    uint8_t unused_56_5f[10];
    uint8_t ar_dr[22];
    uint8_t unused_76_7f[10];
    uint8_t sl_rr[22];
    uint8_t unused_96_9f[10];
    uint8_t fnumberl[9];
    uint8_t unused_a9_af[7];
    uint8_t kon_block_fnumberh[9];
    uint8_t unused_b9_bc[4];
    uint8_t dam_dvb_ryt_bd_sd_tom_tc_hh;
    uint8_t unused_be_bf[2];
    uint8_t chd_chc_chb_cha_fb_cnt[9];
    uint8_t unused_c9_df[23];
    uint8_t ws[22];
};

struct __PACKED__ opl3_data_h_t {
    uint8_t am_biv_egt_ksr_mult[22];
    uint8_t unused_36_3f[10];
    uint8_t ksl_tl[22];
    uint8_t unused_56_5f[10];
    uint8_t ar_dr[22];
    uint8_t unused_76_7f[10];
    uint8_t sl_rr[22];
    uint8_t unused_96_9f[10];
    uint8_t fnumberl[9];
    uint8_t unused_a9_af[7];
    uint8_t kon_block_fnumberh[9];
    uint8_t unused_b9_bf[7];
    uint8_t chd_chc_chb_cha_fb_cnt[9];    
    uint8_t unused_c9_df[23];
    uint8_t ws[22];
};

struct __PACKED__ opl3_l_t {
    struct opl3_base_l_t b;
    uint8_t unused_09_1f[23];
    struct opl3_data_l_t d;    
};

struct __PACKED__ opl3_h_t {
    struct opl3_base_h_t b;
    uint8_t unused_09_1f[23];
    struct opl3_data_h_t d;    
};

static volatile struct opl3_l_t *opl3_l;
static volatile struct opl3_h_t *opl3_h;

/*
 * Direct write to any Adlib/SB Pro II FM synthetiser register.
 *   reg - register number (range 0x001-0x0F5 and 0x101-0x1F5). When high byte
 *         of reg is zero, data go to port FMport, otherwise to FMport+2
 *   data - register value to be written
 */
uint8_t opl3_write(uint16_t reg, uint8_t data)
{
    /* The Foenix's BEATRIX takes care of waits :) */
    if (reg & 0xf00)
        R8(OPL3_BASE_H + (reg & 0xff)) = data;
    else
        R8(OPL3_BASE_L+ (reg & 0xff)) = data;
}


void opl3_init(void)
{
    opl3_l->b.lsi_test= 0;
    opl3_h->b.new = 1;

}

void opl3_set_4op(uint8_t voice, bool four_op)
{

}

#include <stddef.h>
#include <stdio.h>
void main(void)
{
    /* Check structure offsets */
    printf("********************** Structure offsets:\n");
    printf("Low:\n");
    printf(" lsi_test: 0x%02lx\n",offsetof(struct opl3_l_t,b.lsi_test));
    printf(" timer1: 0x%02lx\n",offsetof(struct opl3_l_t,b.timer1));    
    printf(" timer2: 0x%02lx\n",offsetof(struct opl3_l_t,b.timer2));    
    printf(" rst_mt1_mt2_st2_st1: 0x%02lx\n",offsetof(struct opl3_l_t,b.rst_mt1_mt2_st2_st1));    
    printf(" nts: 0x%02lx\n",offsetof(struct opl3_l_t,b.nts));    
    printf(" am_biv_egt_ksr_mult: 0x%02lx\n",offsetof(struct opl3_l_t,d.am_biv_egt_ksr_mult));
    printf(" ksl_tl: 0x%02lx\n",offsetof(struct opl3_l_t,d.ksl_tl));    
    printf(" ar_dr: 0x%02lx\n",offsetof(struct opl3_l_t,d.ar_dr));    
    printf(" sl_rr: 0x%02lx\n",offsetof(struct opl3_l_t,d.sl_rr));    
    printf(" fnumberl: 0x%02lx\n",offsetof(struct opl3_l_t,d.fnumberl));    
    printf(" kon_block_fnumberh: 0x%02lx\n",offsetof(struct opl3_l_t,d.kon_block_fnumberh));    
    printf(" dam_dvb_ryt_bd_sd_tom_tc_hh: 0x%02lx\n",offsetof(struct opl3_l_t,d.dam_dvb_ryt_bd_sd_tom_tc_hh));    
    printf(" chd_chc_chb_cha_fb_cnt: 0x%02lx\n",offsetof(struct opl3_l_t,d.chd_chc_chb_cha_fb_cnt));    
    printf(" ws: 0x%02lx\n",offsetof(struct opl3_l_t,d.ws));    

    printf("\nHigh:\n");
    printf(" lsi_test: 0x%02lx\n",offsetof(struct opl3_h_t,b.lsi_test)); 
    printf(" connectionsel: 0x%02lx\n",offsetof(struct opl3_h_t,b.connectionsel));
    printf(" new: 0x%02lx\n",offsetof(struct opl3_h_t,b.new));
    printf(" am_biv_egt_ksr_mult: 0x%02lx\n",offsetof(struct opl3_h_t,d.am_biv_egt_ksr_mult));
    printf(" ksl_tl: 0x%02lx\n",offsetof(struct opl3_h_t,d.ksl_tl));    
    printf(" ar_dr: 0x%02lx\n",offsetof(struct opl3_h_t,d.ar_dr));    
    printf(" sl_rr: 0x%02lx\n",offsetof(struct opl3_h_t,d.sl_rr));    
    printf(" fnumberl: 0x%02lx\n",offsetof(struct opl3_h_t,d.fnumberl));    
    printf(" kon_block_fnumberh: 0x%02lx\n",offsetof(struct opl3_h_t,d.kon_block_fnumberh));    
    printf(" chd_chc_chb_cha_fb_cnt: 0x%02lx\n",offsetof(struct opl3_h_t,d.chd_chc_chb_cha_fb_cnt));    
    printf(" ws: 0x%02lx\n",offsetof(struct opl3_h_t,d.ws));  
}

#endif