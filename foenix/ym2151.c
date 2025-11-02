/* Small library to help program the Yamaha 2151 OPM synthesizer
 * Author: Vincent Barrilliot
 */

#include <stdint.h>

#define KEY_ON       0x08
#define NE_NFRQ      0x0f
#define LFRQ         0x18
#define PMD_AMD      0x19
#define CT_W         0x1b
#define RL_FB_CON    0x20
#define KEY_CODE     0x21
#define KEY_FRACTION 0x22
#define PMS_AMS      0x3f
#define DT1_MUL      0x40 /* to 0x5f */
#define TL           0x60 /* to 0x7f */
#define KS_AR        0x80 /* to 0x9f */
#define AMS_D1R      0xa0 /* to 0xbf */
#define DT2_D2R      0xc0 /* to 0xdf */
#define D1L_RR       0xe0 /* to 0xff */

/* Return the slot offset for carrier/modulators of a channel.
 * Add the channel number to get the final slot number */
#define C1(c) (c+16)
#define M1(c) c
#define C2(c) (c+24)
#define M2(c) (c+8)

/* Device clock frequency */
#define YM2151_CLOCK 3579545

/* Base register address */
uint8_t *ym2151_base;


static void ym2151_write(uint16_t reg, uint8_t value) {
  ym2151_base[reg] = value;
}

/* Per-slot settings */
struct ym2151_slot_t {
 /* Operator specific */
  uint8_t  total_level;     /* 0-127 (TL) */
  /* Frequency */
  uint8_t  key_follow;      /* 0-63 (KF)  */
  uint8_t  freq_multiplier; /* 0-15 (MUL) */
  uint8_t  detune_coarse;   /* 0-3 (DT2)  */
  uint8_t  deune_fine;     /* 0-7 (DT1)  */
  /* Volume envelope */
  uint8_t  attack_rate;     /* 0-31 (AR)  */
  uint8_t  decay1_rate;     /* 0-31 (DT1) */
  uint8_t  decay1_level;    /* 0-15 (D1L) */
  uint8_t  decay2_rate;     /* 0-31 (DT2) */
  uint8_t  release_rate;    /* 0-15 (FF)  */
  uint8_t  rate_scaling;    /* 0-3 (EG key scaling) */
  /* Modulation */
  uint8_t  mod_enable;      /* 0 or not ANS EN */
};


/* Per-channel (a.k.a tone) settings */
struct ym2151_voice_t {
  /* Voice common */
  uint16_t op_mask;        /* (M1 C1 M2 C2) << 3 : on/off flags for operators */
  uint8_t  output_select;  /* Right - Left (2 bits) */

  uint8_t  algorithm;      /* 0-7 */
  uint8_t  feedback;       /* 0-7 */
  /* Modulation */
  uint8_t  pitch_mod_sens; /* 0-7 */
  uint8_t  amp_mod_sens;   /* 0-3 */

  struct ym2151_slot_t op[4];
};

/* Global settings */
struct ym2151_global_t {
  uint8_t noise_enable;    /* 0-1 */
  uint8_t noise_frequency; /* 0-31 */
  uint8_t lfo_frequency;   /* 0-255 */
  uint8_t lfo_waveform;    /* 0-3 (saw,square,triangle,noise) */
  uint8_t pitch_mod_depth; /* 0-127 */
  uint8_t amp_mod_depth;   /* 0-127 */
};

/* Algorithms:
 * 0:
 *   M1(F)-C1-M2-C2-OUT
 * 1:
 *   M1(F)-+-M2-C2-OUT
 *      C1-+
 * 2:
 *   C1-M2-+-C2-OUT
 *   M1(F)-+
 * 3:
 *   M1(F)-C1-+-C2-OUT
 *         M2-+
 * 4:
 *   M1(F)-C1-+-OUT
 *   M2----C2-+
 * 5:
 *   M1(F)-C1-+-OUT  (no sure what the FB source is)
 *         M2-+
 *         C2-+
 * 6:
 *   M1(F)-C1-+-OUT
 *         M2-+
 *         C2-+
 * 7:
 *   M1(F)-+-OUT
 *   C1--- +
 *   M2----+
 *   C2----+
 */

struct ym2151_voice_t voice_per_channel[8];


void ym2151_key_on(uint16_t channel) {
  uint8_t value;
  
  /* Safe check */
  channel &= 7;
  value = (0x80 | voice_per_channel[channel].op_mask | channel);
  ym2151_write(KEY_ON,value);
}

void ym2151_key_off(uint16_t channel) {
  /* Safe check */
  channel &= 7;
  ym2151_write(KEY_ON,channel);
}

void ym2151_key_code(uint8_t oct, uint8_t note) {
  ym2151_write(KEY_CODE, (oct<<4) | (note&15));
}

void ym2161_key_fraction(uint8_t fraction) {
  ym2151_write(KEY_FRACTION, fraction<<2);
}

void ym2151_detune1_phase_multiply(uint8_t slot, uint8_t dt1, uint8_t mul) {
  ym2151_write(DT1_MUL+slot, (dt1&7) | (mul&15));
}

void ym2151_detune2_d2r(uint8_t slot, uint8_t detune, uint8_t release_rate) {
  ym2151_write(DT2_D2R, (detune<<5) | (release_rate&63));
}

void ym2151_modulation_sensitivity(uint8_t slot, uint8_t phase, uint8_t amplitude) {
  ym2151_write(PMS_AMS, (phase<<4) | (amplitude&3));
}

void ym2151_connection(uint8_t out_select, uint8_t feedback, uint8_t connection) {
  ym2151_write(RL_FB_CON, ((out_select&3)<<6) | ((feedback&7)<<3) | (connection&7));
}

void ym2151_attack_rate(uint8_t slot, int8_t rate) {
  ym2151_write(KS_AR+slot, rate&31);
}

void ym2151_ams_en_decay1r(uint8_t slot, uint8_t ams_enable, uint8_t rate) {
  uint8_t value = rate & 31;
  if (ams_enable)
    value |= 0x80;
  ym2151_write(AMS_D1R+slot, value);
}

void ym2151_ams_decay2r(uint8_t slot, uint8_t rate) {
  ym2151_write(DT2_D2R+slot, rate&31);
}

void ym2151_decay1l_release_rate(uint8_t slot, uint8_t decay1_level, uint8_t release_rate) {
  ym2151_write(D1L_RR+slot, (decay1_level<<4) | (release_rate&15));
}

void ym2151_total_level(uint8_t slot, uint8_t level) {
  ym2151_write(TL+slot, level & 127);
}

void ym2151_noise(uint8_t enable, uint8_t frequency) {
  uint8_t value = frequency & 31;
  if (enable)
    value |= 0x80;
  ym2151_write(NE_NFRQ, value); 
}

void ym2151_lfo_freq(uint8_t frequency) {
  ym2151_write(LFRQ, frequency);
}

void ym2151_lfo_wave(uint8_t lfo_waveform) {
  ym2151_write(CT_W, lfo_waveform&3);
}

void ym2151_pitch_mod_depth(uint8_t depth) {
  ym2151_write(PMD_AMD, 0x80 | depth&7);
}

void ym2151_amplitude_mod_depth(uint8_t depth) {
  ym2151_write(PMD_AMD, depth&7);
}
