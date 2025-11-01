/* Yamaha YM262 (OPL3) FM synthesizer library
 * Author: Vincent Barrilliot, October 2022
 * Public domain
 */
#include "ym262.h"

// Experimental: try to use "session", if a scope of command that we send where we only try to
// send at most one command per register. I
#define YM262_SESSIONS 0

#if !defined(YM262_DEBUG)
# define YM262_DEBUG 0 /* For testing the library without the real thing */
#endif

#if YM262_DEBUG
 #include "a2560_debug.h"
 #include <stdio.h>
 #define a2560_debug(...)
 #define a2560_debugnl(...)
 //#define a2560_debug printf
 //#define a2560_debugnl printf
#else
 #define a2560_debug(...)
 #define a2560_debugnl(...)
#endif

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


/* Minimum rendered frequencies per block, in 10e-4 Hz.
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


/* Shadow registers (so we can operate only on parts of the register) */

#define N_OSC  36
#define N_CHAN 18
static uint8_t shadow_20[N_OSC];
static uint8_t shadow_40[N_OSC];
static uint8_t shadow_60[N_OSC];
static uint8_t shadow_80[N_OSC];
static uint8_t shadow_a0[N_CHAN];
static uint8_t shadow_b0[N_CHAN];
static uint8_t shadow_bd;
static uint8_t shadow_c0[N_CHAN];
static uint8_t shadow_e0[N_OSC];


/* Helper fonctions */

static uint16_t osc_number(uint16_t channel, uint16_t oscillator)
{
	// TODO the lookup to use depends on the mode (2 op, 2 op with drums, 4op) we're in
	return ym262_2op_pairs[channel][oscillator];
}


#ifdef YM262_SESSIONS

#define REG_NOT_USED -1
#define REG_USED 0
static uint16_t sidx;    // Session index
static int16_t *sdata; // Session data
void ym262_session_start(int16_t *data);
void ym262_session_write(uint32_t adr, uint8_t value);
void ym262_session_flush(void);
static int16_t reg_index[512]; // Index: YM262 register

void ym262_session_start(int16_t *data)
{
	sidx = 0;
	sdata = data;

	int i;
	for (i=0; i<sizeof(reg_index)/sizeof(int16_t); i++) {
		reg_index[i] = REG_NOT_USED; // Speedup with memset ? -1 means (not used)
	}
}


void ym262_session_write(uint32_t adr, uint8_t value)
{
	int reg = (uint16_t)(adr-YM262_L);
	//	printf("reg: %03x, %d\n",reg,reg_index[reg]);
	if (reg_index[reg] == REG_NOT_USED) {
		sdata[sidx++] = reg;
		reg_index[reg] = sidx;
		sdata[sidx++] = value;
#if YM262_DEBUG
		printf("*");
#endif
	}
	else {
#if YM262_DEBUG
		printf("-");
#endif
		sdata[reg_index[reg]] = value;
	}
}


void ym262_session_flush(void)
{
	int16_t *d;
	
	if (sidx == 0)
		return;

	sidx /= 2;
	d = sdata;
#if YM262_DEBUG
	printf("sidx: %d\n",sidx);
#endif
	
	while (sidx--) {
		int16_t reg = *d++;
		int16_t val = *d++;
		reg_index[reg] = REG_NOT_USED;
				
#if !YM262_DEBUG
		((uint8_t*)YM262_L)[reg] = val;
		int z;
		for(z=1;z<2000;z++);
#endif
		
#if YM262_DEBUG
		printf("0x%04x, 0x%02x,\n", reg, val);
#endif
	}
#if YM262_DEBUG
	printf("\n");
#endif

}
#endif // SESSIONS



void ym262_write_reg(unsigned long adr, uint8_t value)
{
#if YM262_SESSIONS
	ym262_session_write(adr, value);
#else
	
 #if YM262_DEBUG == 0
	*((uint8_t*)adr) = value;
 #else
	printf("0x%04x, 0x%02x, ", (uint16_t)(adr-YM262_L), value);
 #endif
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

/* General control */

void ym262_reset(void)
{
	int i;
	a2560_debug("ym262_reset");
#if 0
	for (i=0 ; i<N_CHAN; i++)
	 	ym262_channel_off(i);
#endif
	for (i = 0; i<N_CHAN; i++)
	{
		ym262_write_reg(YM262_L+0xa0+i, 0);
		ym262_write_reg(YM262_L+0xb0+i, 0);
		ym262_write_reg(YM262_L+0xc0+i, 0);
		ym262_write_reg(YM262_H+0xa0+i, 0);
		ym262_write_reg(YM262_H+0xb0+i, 0);
		ym262_write_reg(YM262_H+0xc0+i, 0);
	}
	for (i = 0; i<N_OSC; i++)
	{
		ym262_write_reg(YM262_L+0x20+i, 0);
		ym262_write_reg(YM262_L+0x60+i, 0);
		ym262_write_reg(YM262_L+0x80+i, 0);
		ym262_write_reg(YM262_L+0xe0+i, 0);
		ym262_write_reg(YM262_H+0x20+i, 0);
		ym262_write_reg(YM262_H+0x60+i, 0);
		ym262_write_reg(YM262_H+0x80+i, 0);
		ym262_write_reg(YM262_H+0xe0+i, 0);
	}
	ym262_write_reg(YM262_L+1, 0);
	ym262_write_reg(YM262_L+2, 0);
	ym262_write_reg(YM262_L+4, 0);
	ym262_write_reg(YM262_L+5, 0);
	ym262_write_reg(YM262_L+8, 0);

	ym262_write_reg(YM262_REG_TEST, 0);
	ym262_write_reg(YM262_REG_OPL3_EN, YM262_OPL3_EN_MASK);
}



/* Oscillator is 0/1 for 2-osc mode, 0-3 for 4-op mode */

/* Enveloppe and volume control***********************************************/

static const struct ym262_setting_t env_type = { YM252_REG_TONE, YM262_EGT_MASK, shadow_20 };
static const struct ym262_setting_t key_scale_rate = { YM252_REG_TONE, YM262_KSR_MASK, shadow_20 };
static const struct ym262_setting_t key_scale_level = { YM252_REG_KSL_TL, YM262_KSL_MASK, shadow_40 };
static const struct ym262_setting_t osc_level = { YM252_REG_KSL_TL, YM262_TL_MASK, shadow_40 };
static const struct ym262_setting_t attack_rate = { YM252_REG_AR_DR, YM262_AR_MASK, shadow_60 };
static const struct ym262_setting_t decay_rate = { YM252_REG_AR_DR, YM262_DR_MASK, shadow_60 };
static const struct ym262_setting_t sustain = { YM252_REG_SL_RR, YM262_SL_MASK, shadow_80 };
static const struct ym262_setting_t release_rate = { YM252_REG_SL_RR, YM262_RR_MASK, shadow_80 };

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
	set_oscillator(&osc_level, channel, oscillator, 63-(volume&63));
}


/* Wave **********************************************************************/

static const struct ym262_setting_t osc_tremolo = { YM252_REG_TONE, YM262_AM_MASK, shadow_20 };
static const struct ym262_setting_t osc_vibrato = { YM252_REG_TONE, YM262_VIB_MASK, shadow_20 };
static const struct ym262_setting_t osc_freq_multiplier = { YM252_REG_TONE, YM262_MULT_MASK, shadow_20 };
static const struct ym262_setting_t osc_waveform = { YM262_WAVE_SEL, YM262_WAVE_MASK, shadow_e0 };
static const struct ym262_setting_t chan_feedback = { YM262_REG_FBK_OUTSEL, YM262_FBK_MASK, shadow_c0 };
static const struct ym262_setting_t chan_fnum = { YM262_REG_FREQ_NR, -1, shadow_a0 };
static const struct ym262_setting_t chan_note_on_off = { YM262_REG_FNUM, YM262_KEY_ON_MASK, shadow_b0 };
static const struct ym262_setting_t chan_block_fnum = { YM262_REG_FNUM, YM262_FNUM_BLOCK_MASK|YM262_FNUM_FREQ_MASK, shadow_b0 };
static const struct ym262_setting_t chan_osc_connection = { YM262_REG_FBK_OUTSEL, YM262_SYN_MASK, shadow_c0 };
static const struct ym262_setting_t chan_output_sel = { YM262_REG_FBK_OUTSEL, YM262_OUTSEL_MASK, shadow_c0 };
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

/* Connection: YM262_SYN_FM / YM262_SYN_ADD */
void ym262_set_osc_connection(uint16_t channel, uint16_t connection)
{
	set_channel(&chan_osc_connection, channel, connection);
}


/* Note **********************************************************************/

void ym262_channel_on(uint16_t channel)
{
	a2560_debug("ym262_channel_on(channel:%d)", channel);
	set_channel(&chan_note_on_off, channel, YM262_KEY_ON_MASK);
}


void ym262_channel_off(uint16_t channel)
{
	a2560_debug("ym262_channel_off(channel:%d)", channel);
	set_channel(&chan_note_on_off, channel, 0);
}


void ym262_set_block_fnum(uint16_t channel, uint16_t block, uint16_t fnum)
{
	a2560_debug("ym262_set_block_fnum(channel:%d, block:%d, fnum:%d)", channel, block, fnum);
	set_channel(&chan_block_fnum, channel, (block << 2) | (fnum >> 8));
	set_channel(&chan_fnum, channel, fnum & 0xff);
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


/* output_sel should be YM262_OUTSEL_L/YM262_OUTSEL_R/YM262_OUTSEL_LR */
void ym262_set_output(uint16_t channel, uint16_t output_sel)
{
	set_channel(&chan_output_sel, channel, output_sel);
}


/* Make a sound test */
int ym262_test(int argc, char **argv);

#if YM262_DEBUG
int main(int argc, char **argv) {
	return ym262_test(argc, argv);
}
#endif

int ym262_test(int argc, char **argv)
{
	uint16_t i;
	uint32_t scale[128];
	int note_number = 47;
	int channel;
	int16_t session[2048];

#if 0
    for (i=0; i<7; i++)
        ym262_channel_off(i);
	return 0;
#endif
	
#if YM262_SESSIONS
	ym262_session_start(session);
#endif
	
	/* Compute frequencies for the scale */
	ym262_create_note_to_freq_lut(4400, scale);
	//for (int i=0; i<128; i++)
	//	a2560_debugnl("%d: %ld\n", i, scale[i]);

	//	ym262_reset();

	/* Store block/fnum for each note */
	uint16_t blocks[128];
	uint16_t fnums[128];
	for (i=0; i<128; i++)
	{
		ym262_get_block_fnum_by_freq(scale[i], &blocks[i], &fnums[i]);
	}

	/* Global settings*/
	ym262_set_vibrato_depth(1);
	ym262_set_tremolo_depth(1);

	const int n_channels = 7;
	
	for (channel=0;channel<n_channels;channel++) {
		//opl3_test();
		//a2560_debugnl("Set OPL3 mode");
		//ym262_write_reg(YM262_REG_OPL3_EN, YM262_OPL3_EN_MASK);
		/* Program a basic sound */
		// Modulator
		ym262_set_key_scale_level(channel, 1, 0);
		ym262_set_oscfreqmult(channel, 1, 3);
		ym262_set_env_type(channel, 1, YM262_EGT_SUSTAINED);
		ym262_set_attack_rate(channel, 1, 15);
		//a2560_debugnl("Set decay rate");
		ym262_set_decay_rate(channel, 1, 15);
		//a2560_debugnl("Set slope rate");
		ym262_set_slope_rate(channel, 1, 0);
		//a2560_debugnl("Set release rate");
		ym262_set_release_rate(channel, 1, 5);
		//a2560_debugnl("Set volume");
		ym262_set_osc_volume(channel, 1,20);
		ym262_set_vibrato(channel, 1, false);
		ym262_set_tremolo(channel, 1, false);
		ym262_set_osc_connection(0, YM262_SYN_FM);
		//a2560_debugnl("Set oscfreqmult");
		ym262_set_feedback(channel,3);

		ym262_set_key_scale_level(channel, 0, 0);
		ym262_set_oscfreqmult(channel, 0, 2);
		ym262_set_env_type(channel, 0, YM262_EGT_SUSTAINED);
		ym262_set_attack_rate(channel, 0, 15);
		//a2560_debugnl("Set decay rate");
		ym262_set_decay_rate(channel, 0, 2);
		//a2560_debugnl("Set slope rate");
		ym262_set_slope_rate(channel, 0, 4);
		//a2560_debugnl("Set release rate");
		ym262_set_release_rate(channel, 0, 5);
		//a2560_debugnl("Set volume");
		ym262_set_osc_volume(channel, 0,63);
		ym262_set_vibrato(channel, 0, false);
		ym262_set_tremolo(channel, 0, false);
		//a2560_debugnl("Set oscfreqmult");

		ym262_set_block_fnum(0, blocks[note_number], fnums[note_number]);
		ym262_set_block_fnum(1, blocks[note_number+4], fnums[note_number+4]);
		ym262_set_block_fnum(2, blocks[note_number+7], fnums[note_number+7]);
		ym262_set_block_fnum(3, blocks[note_number+12], fnums[note_number+12]);
		ym262_set_block_fnum(4, blocks[note_number-12], fnums[note_number-12]);
		ym262_set_block_fnum(5, blocks[note_number-24], fnums[note_number-24]);

		a2560_debug("ym262_set_output(0, YM262_OUTSEL_LR)");
		ym262_set_output(channel,YM262_OUTSEL_LR);
		a2560_debug("ym262_set_osc_connection(0, FM)");
		ym262_set_osc_connection(channel, YM262_SYN_ADD);

		a2560_debug("channel_on(0)");
		ym262_channel_on(channel);
	}

#if YM262_SESSIONS
	ym262_session_flush();
#endif
	
	return 0;
}

