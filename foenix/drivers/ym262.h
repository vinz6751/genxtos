/* Definitions for the Yamaha YM262 OPL3 synthesizer */

#ifndef _YM262_H_
#define _YM262_H_

#include <stdint.h>
#include <stdbool.h>

#define YM262_L              (0xB20200) /* Base address of "Low" registers */
#define YM262_H              (0xB20300) /* Base address of "High" registers */

#define YM262_REG_TEST         (YM262_L+1)
#define  YM262_WS_ENABLE_MASK  (1<<5)         /* Waveform Select: 1: use  (e0-f5), 0: sine wave only */
#define  YM262_TEST_MASK       (31<<0)        /*  Must be set to 0 */

/* Timers and interrupts control */
#define YM262_REG_TIMER1       (YM262_L+2) /* Timer 1 period (80us resolution) */
#define YM262_REG_TIMER2       (YM262_L+3) /* Timer 1 period (80us resolution) */
#define YM262_REG_IRQ          (YM262_L+4)
#define  YM262_IRQ_RESET_MASK   (1<<7) /* Reset timers and IRQ flag in status register. Must be 0 for other regs to work */
#define  YM262_IRQ_T1_MASKED    (1<<6) /* Mask timer 1 interrupt */
#define  YM262_IRQ_T2_MASKED    (1<<5) /* Mask timer 2 interrupt */
#define  YM262_IRQ_T1_EN       (1<<0) /* Timer 1 on/off */
#define  YM262_IRQ_T2_EN       (1<<1) /* Timer 2 on/off */

/* OPL3 extensions enable */
#define YM262_REG_OPL3_EN      (YM262_L+5)
#define  YM262_OPL3_EN_MASK      (1<<0) /* Set to 1 to enable OPL3 extensions. If 0, behave like OPL2 */

#define YM262_REG_CSW_NOTE_SEL (YM262_L+8)
#define  YM262_NTS_MASK         (1<<6) /* Controls the split point of the keyboard to determine the key scale number. When 0, the keyboard split is the second bit from the bit 8 of the F-Number. When 1, the MSb of the F-Number is used. */

/* Per-oscillator settings */
#define YM252_REG_TONE         (YM262_L+0x20) /* 0x20 to 0x35: AM VIB EGT KSR MULT */
#define  YM262_AM_MASK          (1<<7) /* Amplitude modulation enable */
#define  YM262_VIB_MASK         (1<<6) /* Pitch modulation enable */
#define  YM262_EGT_MASK         (1<<5)
#define    YM262_EGT_SUSTAINED   (1<<5)
#define    YM262_EGT_DECAY       (0<<5)
#define  YM262_KSR_MASK         (1<<4)
#define  YM262_MULT_MASK        (0xf<<0)

#define YM252_REG_KSL_TL       (YM262_L+0x40) /* 0x40 to 0x55: KSL TL */
#define  YM262_KSL_MASK         (3<<6)
#define   YM262_KSL_0DB          (0<<6)
#define   YM262_KSL_05DB         (1<<6)
#define   YM262_KSL_3DB          (2<<6)
#define   YM262_KSL_6DB          (3<<6)
#define  YM262_TL_MASK          (63<<0)

#define YM252_REG_AR_DR        (YM262_L+0x60) /* 0x60 to 0x75: AR DR */
#define  YM262_AR_MASK          (0xf<<4) /* Attack rate */
#define  YM262_DR_MASK          (0xf<<0) /* Decay rate */

#define YM252_REG_SL_RR        (YM262_L+0x80) /* 0x80 to 0x95: SL RR */
#define  YM262_SL_MASK          (0xf<<4) /* Slope rate */
#define  YM262_RR_MASK          (0xf<<0) /* Release rate */

/* Note pitch / key-on (9 registers, one per channel) */
#define YM262_REG_FREQ_NR      (YM262_L+0xa0) /* a0 to a8 */
#define YM262_REG_FNUM         (YM262_L+0xb0) /* b0 to a8 */
#define  YM262_KEY_ON_MASK      (1<<5) /* 1: channel plays */
#define  YM262_FNUM_BLOCK_MASK  (7<<2) /* Roughly determines the octave */
#define  YM262_FNUM_FREQ_MASK   (3<<0) /* Pitch data within the octave. FNUM = f*2^19/fs/2^(block-1) or Music Frequency * 2^(20-Block) / 49716 Hz*/

/* Effects depth and drums key-on */
#define YM262_DEPTH_RYTHM      (YM262_L+0xbd)
#define  YM262_TREMOLO_MASK     (1<<7) /* Tremolo depth: 0=1.0dB, 1: 4.8dB */
#define  YM262_VIBRATO_MASK     (1<<6) /* Vibrato depth: 0=7cents, 1=14cents */
#define  YM262_PERCU_MODE_MASK  (1<<5) /* Mode: 0=normal (melodic), 1=percussion */
/* Note: KEY-ON bits of channels 6, 7 and 8 must be clear and their F-Nums, Attack/Decay/Release rates, etc. must be set properly to use percussion mode. */
#define  YM262_PERCU_KICK       (1<<4) /* Key on of the bass drum channel */
#define  YM262_PERCU_SNARE      (1<<3) /* Key on of the snare drum channel */
#define  YM262_PERCU_TOM        (1<<2) /* Key on of the tom-tom channel */
#define  YM262_PERCU_CYMBAL     (1<<1) /* Key on of the cymbal channel */
#define  YM262_PERCU_HIHAT      (1<<0) /* Key on of the hihat channel */

/* Feedback modulation and output selection for 9 channels*/
#define YM262_REG_FBK_OUTSEL   (YM262_L+0xc0)
#define  YM262_OUTSEL_MASK      (3<<4)
#define   YM262_OUTSEL_L         (1<<4)
#define   YM262_OUTSEL_R         (2<<4)
#define   YM262_OUTSEL_LR        (3<<4) /* Convenience */
#define  YM262_FBK_MASK         (3<<1) /* Feedback amount 0: non, 1:n/16, 5:n, 7:4*n  . When in four-operator mode, the FeedBack value is used only by Operator 1, value of Operators 2, 3 and 4 is ignored */
#define  YM262_SYN_MASK         (1<<0) /* Oscillators connection type  */
#define   YM262_SYN_FM           (0<<0) /* 0: FM */
#define   YM262_SYN_ADD          (1<<0) /* 1:Additive */
/*
 In four-operator mode, there are two bits controlling the synthesis type.
 Both are the bit 0 of register C0, one of Operators 1 and 2 and the second of Operators 3 and 4.

		+--------+--------+--------+
		| Op 1&2 | Op 3&4 |  Type  |
		+--------+--------+--------+
		|    0   |  NONE  |   FM   |
		|    1   |  NONE  |   AM   |
		|    0   |    0   | FM-FM  |
		|    1   |    0   | AM-FM  |
		|    0   |    1   | FM-AM  |
		|    1   |    1   | AM-AM  |
		+--------+--------+--------+
*/

/* Waveform selection (9 channels) e0-e8 */
#define YM262_WAVE_SEL        (YM262_L+0xe0)
#define  YM262_WAVE_MASK       (7<<0)
/* Bit 5 of register 01 (YM262_WS_ENABLE_MASK) must be set to use waveforms other than sine. 
 * Waveforms 4-7 are available only on OPL3 (see YM262_REG_OPL3_EN)
 * 0: sine, 1, half-sine (+only), 2:abs-sine, 3:pi/4-sine
 * 4: pi/2-sine, 5 abs(pi/2_sine), 6:square, 7:derived square */ 

/* Public fonction prototypes */
void ym262_write_reg(unsigned long adr, uint8_t value); /* The address is a real address, not some kind of offset */


/* General control */

void ym262_reset(void);


/* Enveloppe and volume control */

void ym262_set_attack_rate(uint16_t channel, uint16_t oscillator, uint16_t rate);
void ym262_set_decay_rate(uint16_t channel, uint16_t oscillator, uint16_t rate);
void ym262_set_slope_rate(uint16_t channel, uint16_t oscillator, uint16_t rate);
void ym262_set_release_rate(uint16_t channel, uint16_t oscillator, uint16_t rate);
void ym262_set_env_type(uint16_t channel, uint16_t oscillator, uint16_t type); /* Type must be 0 or 32 */
void ym262_set_key_scale_rate(uint16_t channel, uint16_t oscillator, bool rate);
void ym262_set_key_scale_level(uint16_t channel, uint16_t oscillator, uint16_t scaling);
void ym262_set_osc_volume(uint16_t channel, uint16_t oscillator, uint16_t volume);


/* Note and pitch control */

void ym262_channel_on(uint16_t channel);
void ym262_channel_off(uint16_t channel);
void ym262_set_block_fnum(uint16_t channel, uint16_t block, uint16_t fnum);
void ym262_set_freq(uint16_t channel, uint32_t freq); /* Convenience. freq is in 10e-3 Hz */
void ym262_get_block_fnum_by_freq(uint32_t freq, uint16_t *block, uint16_t *fnum); /* freq is in 10e-4 Hz (440Hz -> freq=4 400 000) */


/* Wave control */

void ym262_set_tremolo(uint16_t channel, uint16_t oscillator, bool enabled);
void ym262_set_vibrato(uint16_t channel, uint16_t oscillator, bool enabled);
void ym262_set_tremolo_depth(uint16_t depth);
void ym262_set_vibrato_depth(uint16_t depth);
void ym262_set_oscfreqmult(uint16_t channel, uint16_t oscillator, uint16_t mult);
void ym262_set_osc_waveform(uint16_t channel, uint16_t oscillator, uint16_t wave);
void ym262_set_feedback(uint16_t channel, uint16_t fbamount);
void ym262_set_osc_connection(uint16_t channel, uint16_t connection);
void ym262_create_note_to_freq_lut(uint32_t middleA, uint32_t *lut);
void ym262_set_output(uint16_t channel, uint16_t output_sel);

#endif
