/* Definitions for the Yamaha 2612 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> /* for debug */


#define YM262_L              (0xB20300) /* Base address of "Low" registers */
#define YM262_H              (0xB20200) /* Base address of "High" registers */

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
#define  YM262_OPL3_EN_MASK      (1<<1) /* Set to 1 to enable OPL3 extensions. If 0, behave like OPL2 */

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

/* Pairs of the operator making up a channel */
struct op_pair { uint16_t op0; uint16_t op1; };

/* In 2 operator mode */
const uint16_t ym262_2op_pairs[18][2] = {
	{0, 3},   /* 0 */
	{1, 4},   /* 1 */
	{2, 5},   /* 2 */
	{6, 9},   /* 3 */
	{7, 10},  /* 4 */
	{8, 11},  /* 5 */
	{12, 15}, /* 6 */
	{13, 16}, /* 7 */
	{14, 17}, /* 8 */
	{18, 21}, /* 9 */
	{19, 22}, /* 10 */
	{20, 23}, /* 11 */ 
	{24, 27}, /* 12 */
	{25, 28}, /* 13 */
	{26, 29}, /* 14 */
	{30, 33}, /* 15 */
	{31, 34}, /* 16 */
	{32, 35}, /* 17 */
};

/* In 2 operator with drums */
static const struct op_pair ym262_2op_with_drums_pairs[] = {
	{0, 3},   /* 0 */
	{1, 4},   /* 1 */
	{2, 5},   /* 2 */
	{6, 9},   /* 3 */
	{7, 10},  /* 4 */
	{8, 11},  /* 5 */
	{12, 15}, /* Bass drum */
	{13, -1}, /* Hi-hat */
	{14, -1}, /* Tom tom */
	{16, -1}, /* Snare drum */
	{17, -1}, /* Cymbal */
	{18, 21}, /* 9 */
	{19, 22}, /* 10 */
	{20, 23}, /* 11 */ 
	{24, 27}, /* 12 */
	{25, 28}, /* 13 */
	{26, 29}, /* 14 */
	{30, 33}, /* 15 */
	{31, 34}, /* 16 */
	{32, 35}, /* 17 */
};


/* Operators address, relative to "low" */
#define OPOFST(set,address) (set?(YM262_H-YM262_L):0)+address
const uint16_t op_offset[] = {
	OPOFST(0, 0), /* 0 */
	OPOFST(0, 1), /* 1 */
	OPOFST(0, 2), /* 2 */
	OPOFST(0, 3), /* 3 */
	OPOFST(0, 4), /* 4 */
	OPOFST(0, 5), /* 5 */
	OPOFST(0, 8), /* 6 */
	OPOFST(0, 9), /* 7 */
	OPOFST(0, 0xa), /* 8 */
	OPOFST(0, 0xb), /* 9 */
	OPOFST(0, 0xc), /* 10 */
	OPOFST(0, 0xd), /* 11 */
	OPOFST(0, 0x10), /* 12 */
	OPOFST(0, 0x11), /* 13 */
	OPOFST(0, 0x12), /* 14 */
	OPOFST(0, 0x13), /* 15 */
	OPOFST(0, 0x14), /* 16 */
	OPOFST(0, 0x15), /* 17 */
	/* Same as above, with set 1, starting at 18 */
	OPOFST(1, 0), /* 18 */
	OPOFST(1, 1), /* 19 */
	OPOFST(1, 2), /* 20 */
	OPOFST(1, 3), /* 21 */
	OPOFST(1, 4), /* 22 */
	OPOFST(1, 5), /* 23 */
	OPOFST(1, 8), /* 24 */
	OPOFST(1, 9), /* 25 */
	OPOFST(1, 0xa), /* 26 */
	OPOFST(1, 0xb), /* 27 */
	OPOFST(1, 0xc), /* 28 */
	OPOFST(1, 0xd), /* 29 */
	OPOFST(1, 0x10), /* 30 */
	OPOFST(1, 0x11), /* 31 */
	OPOFST(1, 0x12), /* 32 */
	OPOFST(1, 0x13), /* 33 */
	OPOFST(1, 0x14), /* 34 */
	OPOFST(1, 0x15), /* 35 */
};

/* Channel offsets for register sets indexed by channel number (e.g. b0-b8, c0-c8), */
const uint16_t ym_chan_offset[] = {
	OPOFST(0,0), OPOFST(0,1), OPOFST(0,2), OPOFST(0,3),
	OPOFST(0,4), OPOFST(0,5), OPOFST(0,6), OPOFST(0,7), OPOFST(0,8),
	OPOFST(1,0), OPOFST(1,1), OPOFST(1,2), OPOFST(1,3),
	OPOFST(1,4), OPOFST(1,5), OPOFST(1,6), OPOFST(1,7), OPOFST(1,8)
};


/* Minimum rendered frequencies per block, in 10e-3 Hz.
 * The fmin is also the resolution (i.e. the freq difference between 2 consecutive values in the block) */
static const struct block_freqs_t {
	uint16_t fmin;
	uint32_t fmax;
} ym262_freqs_by_block[8] = {
#if 1
        {474, 485033},
        {948, 970067},
        {1896, 1940134},
        {3793, 3880269},
        {7586, 7760538},
        {15172, 15521077},
        {30344, 31042155},
        {60688, 62084311},
#else
        {47, 48503},
        {94, 97006},
        {189, 194013},
        {379, 388026},
        {758, 776053},
        {1517, 1552107},
        {3034, 3104215},
        {6068, 6208431}
#endif
};


/* Public fonction prototypes */
void ym262_write_reg(unsigned long adr, uint8_t value); /* The address is a real address, not some kind of offset */


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


/* Shadow registers (so we can operate only on parts of the register) */

static uint8_t shadow_20[18];
static uint8_t shadow_40[18];
static uint8_t shadow_60[18];
static uint8_t shadow_80[18];
static uint8_t shadow_b0[18];
static uint8_t shadow_bd;
static uint8_t shadow_c0[18];
static uint8_t shadow_e0[18];


/* Helper fonctions */

static uint16_t osc_number(uint16_t channel, uint16_t oscillator)
{
	// TODO the lookup to use depends on the mode (2 op, 2 op with drums, 4op) we're in
	return ym262_2op_pairs[channel][oscillator];
}

static uint16_t osc_offset(uint16_t channel, uint16_t oscillator)
{
	return op_offset[osc_number(channel, oscillator)];
}


void ym262_write_reg(unsigned long adr, uint8_t value)
{
	printf("Write %02x to %p\n", value, adr);
#if 0
	*((uint8_t*)adr) = value;
#endif
}

struct ym262_setting_t {
	unsigned long regset;
	uint8_t mask;
	uint8_t *shadow;
};


static void set_oscillator(const struct ym262_setting_t* setting, uint16_t channel, uint16_t oscillator, uint16_t value)
{
	uint16_t osc = osc_number(channel, oscillator);
	setting->shadow[osc] &= ~setting->mask;
	setting->shadow[osc] |= value & setting->mask;
	ym262_write_reg(setting->regset + op_offset[osc], setting->shadow[osc]);
}


static void set_channel(const struct ym262_setting_t* setting, uint16_t channel, uint16_t value)
{
	setting->shadow[channel] &= ~setting->mask;
	setting->shadow[channel] |= value & setting->mask;
	ym262_write_reg(setting->regset + ym_chan_offset[channel], setting->shadow[channel]);
}


static void set_global(const struct ym262_setting_t* setting, uint16_t value)
{
	*(setting->shadow) &= ~setting->mask;
	*(setting->shadow) |= value & setting->mask;
	ym262_write_reg(setting->regset, *(setting->shadow));
}

/* Oscillator is 0/1 for 2-osc mode, 0-3 for 4-op mode */

/* Enveloppe and volume control***********************************************/

static const struct ym262_setting_t env_type = { YM252_REG_TONE, YM262_EGT_MASK, shadow_20 };
static const struct ym262_setting_t key_scale_rate = { YM252_REG_TONE, YM262_KSR_MASK, shadow_20 };
static const struct ym262_setting_t key_scale_level = { YM252_REG_KSL_TL, YM262_KSL_MASK, shadow_40 };
static const struct ym262_setting_t osc_level = { YM252_REG_KSL_TL, YM262_TL_MASK, shadow_40 };
static const struct ym262_setting_t attack_rate = { YM252_REG_AR_DR, YM262_AR_MASK, shadow_60 };
static const struct ym262_setting_t decay_rate = { YM252_REG_AR_DR, YM262_DR_MASK, shadow_60 };
static const struct ym262_setting_t sustain = { YM252_REG_AR_DR, YM262_SL_MASK, shadow_80 };
static const struct ym262_setting_t release_rate = { YM252_REG_AR_DR, YM262_RR_MASK, shadow_80 };

/* Rate: 0-15 */
void ym262_set_attack_rate(uint16_t channel, uint16_t oscillator, uint16_t rate)
{
	set_oscillator(&attack_rate, channel, oscillator, rate << 4);
}

/* Rate: 0-15 */
void ym262_set_decay_rate(uint16_t channel, uint16_t oscillator, uint16_t rate)
{
	set_oscillator(&decay_rate, channel, oscillator, rate);
}

/* Rate: 0-15 */
void ym262_set_slope_rate(uint16_t channel, uint16_t oscillator, uint16_t rate)
{
	set_oscillator(&sustain, channel, oscillator, rate << 4);
}

/* Rate: 0-15 */
void ym262_set_release_rate(uint16_t channel, uint16_t oscillator, uint16_t rate)
{
	set_oscillator(&release_rate, channel, oscillator, rate);
}

/* Type should be YM262_EGT_SUSTAINED / YM262_EGT_DECAY */
void ym262_set_env_type(uint16_t channel, uint16_t oscillator, uint16_t type)
{
	set_oscillator(&env_type, channel, oscillator, type);
}

/* Rate: 0-1 */
void ym262_set_key_scale_rate(uint16_t channel, uint16_t oscillator, bool rate)
{
	set_oscillator(&key_scale_rate, channel, oscillator, rate << 4);
}

/* Level: 0-3 */
void ym262_set_key_scale_level(uint16_t channel, uint16_t oscillator, uint16_t level)
{
	set_oscillator(&key_scale_level, channel, oscillator, level << 6);
}


void ym262_set_osc_volume(uint16_t channel, uint16_t oscillator, uint16_t volume)
{
	set_oscillator(&osc_level, channel, oscillator, 63-volume);
}


/* Wave **********************************************************************/

static const struct ym262_setting_t osc_tremolo = { YM252_REG_TONE, YM262_AM_MASK, shadow_20 };
static const struct ym262_setting_t osc_vibrato = { YM252_REG_TONE, YM262_VIB_MASK, shadow_20 };
static const struct ym262_setting_t osc_freq_multiplier = { YM252_REG_TONE, YM262_VIB_MASK, shadow_20 };
static const struct ym262_setting_t osc_waveform = { YM262_WAVE_SEL, YM262_WAVE_MASK, shadow_e0 };
static const struct ym262_setting_t chan_feedback = { YM262_REG_FBK_OUTSEL, YM262_FBK_MASK, shadow_c0 };
static const struct ym262_setting_t chan_osc_connection = { YM262_REG_FBK_OUTSEL, YM262_SYN_MASK, shadow_c0 };
static const struct ym262_setting_t tremolo_depth = { YM262_DEPTH_RYTHM, YM262_TREMOLO_MASK, &shadow_bd };
static const struct ym262_setting_t vibrato_depth = { YM262_DEPTH_RYTHM, YM262_VIBRATO_MASK, &shadow_bd };

void ym262_set_tremolo(uint16_t channel, uint16_t oscillator, bool enabled)
{
	set_oscillator(&osc_tremolo, channel, oscillator, enabled ? YM262_AM_MASK : 0);
}


void ym262_set_vibrato(uint16_t channel, uint16_t oscillator, bool enabled)
{
	set_oscillator(&osc_vibrato, channel, oscillator, enabled ? YM262_VIB_MASK : 0);
}

/* Depth: 0-1 */
void ym262_set_tremolo_depth(uint16_t depth)
{
	set_global(&tremolo_depth, depth << 7);
}

/* Depth: 0-1 */
void ym262_set_vibrato_depth(uint16_t depth)
{
	set_global(&vibrato_depth, depth << 6);
}

/* Fbamount: 0-7 */
void ym262_set_feedback(uint16_t channel, uint16_t fbamount)
{
	set_channel(&chan_feedback, channel, fbamount << 1);
}

/* Mult: 0-15 */
void ym262_set_oscfreqmult(uint16_t channel, uint16_t oscillator, uint16_t mult)
{
	set_oscillator(&osc_freq_multiplier, channel, oscillator, mult);
}

/* Wave: 0-7 */
void ym262_set_osc_waveform(uint16_t channel, uint16_t oscillator, uint16_t wave)
{
	set_oscillator(&osc_waveform, channel, oscillator, wave);
}

/* Connectiono: YM262_SYN_FM / YM262_SYN_ADD */
void ym262_set_osc_connection(uint16_t channel, uint16_t connection)
{
	set_channel(&chan_osc_connection, channel, connection);
}


/* Note **********************************************************************/

void ym262_channel_on(uint16_t channel)
{
	shadow_b0[channel] |= YM262_KEY_ON_MASK;
	ym262_write_reg(YM262_REG_FNUM+ym_chan_offset[channel], shadow_b0[channel]);
}


void ym262_channel_off(uint16_t channel)
{
	shadow_b0[channel] &= ~YM262_KEY_ON_MASK;
	ym262_write_reg(YM262_REG_FNUM+ym_chan_offset[channel], shadow_b0[channel]);
}


void ym262_set_block_fnum(uint16_t channel, uint16_t block, uint16_t fnum)
{
	shadow_b0[channel] &= YM262_FNUM_BLOCK_MASK | YM262_FNUM_FREQ_MASK;
	shadow_b0[channel] |= (block << 2) | (fnum >> 8);

	ym262_write_reg(YM262_REG_FNUM+ym_chan_offset[channel], shadow_b0[channel]);
	ym262_write_reg(YM262_REG_FREQ_NR+ym_chan_offset[channel], fnum & 0xff);
}


/* Convenience */
void ym262_set_freq(uint16_t channel, uint32_t freq)
{
	uint16_t block, fnum;
	
	ym262_get_block_fnum_by_freq(freq, &block, &fnum);
	ym262_set_block_fnum(channel, block, fnum);
}


/* Compute the block and fnum for a given frequency (in 1/10000Hz) */
void ym262_get_block_fnum_by_freq(uint32_t freq, uint16_t *block, uint16_t *fnum)
{
	uint16_t i;

	/* Find the block. We use the lowest as it, in theory, provides the best resolution */
	for (i = 0; i < 8; i++)
	{
		if (ym262_freqs_by_block[i].fmin <= freq && freq <= ym262_freqs_by_block[i].fmax)
		{
			*block = i;
			*fnum = freq / ym262_freqs_by_block[i].fmin;
			return;
		}
	}
	/* If the frequency is really out of range, return 0 for all */
	*block = *fnum = 0;
	return;
}


/* This is 2^((note_nr - 69)/12) * 1000 for note_nr 0..127
 * Multiply that by the frequency of the middle A (normally 440) and
 * you'll get the frequency of notes from C-1 TO G9 note, in 10e-3 Hz, eg 
 * middle A will be 440*1000 */
static const unsigned long tune_table[] = {
	19, 20, 21, 23, 24, 25, 27, 28, 30, 32, 34, 36,
	38, 40, 42, 45, 47, 50, 53, 56, 59, 63, 67, 71,
	75, 79, 84, 89, 94, 100, 106, 112, 118, 125, 133, 141,
	149, 158, 167, 177, 188, 199, 211, 223, 236, 250, 265, 281,
	298, 315, 334, 354, 375, 397, 421, 446, 472, 500, 530, 562,
	595, 630, 668, 708, 750, 794, 841, 891, 944, 1000, 1060, 1123,
	1190, 1260, 1335, 1415, 1499, 1588, 1682, 1782, 1888, 2000, 2119, 2245,
	2379, 2520, 2670, 2829, 2997, 3175, 3364, 3564, 3776, 4000, 4238, 4490,
	4757, 5040, 5340, 5657, 5994, 6350, 6728, 7128, 7551, 8000, 8476, 8980,
	9514, 10080, 10679, 11314, 11987, 12700, 13455, 14255, 15102, 16000, 16952, 17960,
	19028, 20159, 21358, 22628, 23973, 25399, 26909, 28509 };


/* Initialise the given lut (note to freq) table for the A4 of given frequency,
 * with a resolution of 10e-4Hz
 * middleA is a frequency in 10th of Hz, e.g. for A4=440Hz, middleA=4400
 * So you can tune the chip with a 1/10 semi-tone resolution */
void ym262_create_note_to_freq_lut(uint32_t middleA, uint32_t *lut)
{
	int i;
	for (i = 0; i < 128; i++)
		lut[i] = tune_table[i] * middleA;
}


uint16_t main(void)
{
	uint16_t i;
	uint32_t scale[128];
#if 0
	/* Compute frequencies for the scale */
	ym262_create_note_to_freq_lut(4400, scale);
	for (int i=0; i<128; i++)
		printf("%d: %ld\n", i, scale[i]);
#endif

	/* Store block/fnum for each note */
	uint16_t blocks[128];
	uint16_t fnums[128];
	for (int i=0; i<128; i++)
	{
		ym262_get_block_fnum_by_freq(scale[i], &blocks[i], &fnums[i]);
//		printf("%d: freq:%ld block:%d fnum:%d\n", i, scale[i], blocks[i], fnums[i]);
	}

	/* Program a basic sound */
	ym262_set_attack_rate(0, 0, 15);
	ym262_set_decay_rate(0, 0, 2);
	ym262_set_release_rate(0, 0, 8);
	ym262_set_osc_volume(0, 0, 63);
	ym262_set_vibrato(0, 0, true);

	ym262_set_attack_rate(0, 1, 8);
	ym262_set_decay_rate(0, 1, 8);
	ym262_set_release_rate(0, 1, 8);
	ym262_set_osc_volume(0, 1, 40);
	ym262_set_tremolo(0, 1, true);
	ym262_set_oscfreqmult(0, 1, 2);

	ym262_set_block_fnum(0, blocks[69], fnums[69]);
	ym262_channel_on(0);

#if 0

	/* * 2^(20-Block) / 49716 Hz */ 
	uint16_t block, fnum;
	ym262_get_block_fnum_by_freq(4396*100L, &block, &fnum);
	printf("440Hz: block %d fnum %d\n", block, fnum);
#endif
}
