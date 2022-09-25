/*
 * WM8776 - Functions and defines for the WM8776 Audio codec/mixer
 *
 * Author:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.
 */

#ifndef WM8776_H
#define WM8776_H

#include <stdint.h>

/* Summary (for complete list of feature, see the ):
 * The WM8776 of Wolfson microelectronics can mix 5 stereo analog inputs (you can enable which).
 * The signals are summed and sent an analog attenuator, with has a PGA with different modes to adjust the level,
 * then ADC, is processed then sent to a DAC. 
 * Along the signal path there is a with noise gate, limiter, and auto-level features (a bit like a compressor).
 * Digital signal processing includes high-pass filter, de-emphasis.
 * 
 * Then the mixing is done (and auxiliary input can be mixed in, and the inputs can be directly mixed by bypassing
 * the digital signal path) and the data is output to headphones and line output, each of them having their own
 * volume control.
 */

/* Get register number from WM8776 message */
#define WM8776_REGNR(x) (((uint16_t)x) >> 9)

/* Get data from WM8776 message */
#define WM8776_VALUE(x) (x & 0x1FF)

#define WM8776_R00_HEADPHONE_ATTENUATION_L 0x00 /* Left analog output volume control */
#define   HPLA_MASK               (127<<0) /* Headphones left attenuation in 1db steps. 0x79 = 0dB */
#define   HPLZCEN_MASK            (1<<7)   /* Headphones zero-cross detect enable */
#define    HPLZCEN_DISABLED       (0<<7)
#define    HPLZCEN_ENABLED        (1<<7)
#define   UPDATEL                 (1<<8)   /* Simulaneous update of headphone attenuation latches */
#define    UPDATEL_DISABLED       (0<<8)
#define    UPDATEL_ENABLED        (1<<8)

#define WM8776_R01_HEADPHONE_ATTENUATION_R (0x01<<9) /* Right analog output volume control */
#define   HPRA_MASK               (127<<0) /* Headphones right attenuation in 1db steps. 0x79 = 0dB */
#define   HPRZCEN_MASK            (1<<7)   /* Headphones zero-cross detect enable */
#define    HPRZCEN_DISABLED       (0<<7)
#define    HPRZCEN_ENABLED        (1<<7)
#define   UPDATER                 (1<<8)   /* Simulaneous update of headphone attenuation latches */
#define    UPDATER_DISABLED       (0<<8)
#define    UPDATER_ENABLED        (1<<8)

#define WM8776_R02_HEADPHONE_ATTENUATION_ALL (0x02<<9) /* Both headphone channels output volume control */
#define   HPRMASTA_MASK           (127<<0) /* Headphones right attenuation in 1db steps. 0x79 = 0dB */
#define   MZCEN_MASK              (1<<7)   /* Headphones zero-cross detect enable */
#define    MZCEN_DISABLED         (0<<7)
#define    MZCEN_ENABLED          (1<<7)
#define   UPDATEA                 (1<<8)   /* Simulaneous update of headphone attenuation latches */
#define    UPDATEA_DISABLED       (0<<8)
#define    UPDATEA_ENABLED        (1<<8)

#define WM8776_R03_DAC_VOLUME_L (0x03<<9) /* Left DAC digital volume control (DACL) */
#define   LDA_MASK                (255<<0) /* Left digital attenuation. 0xff: 0dB */
#define   LDA_UPDATED_MASK        (1<<8)   /* Simultaneous update of attenuation latches */
#define    LDA_NOT_LATCHED        (0<<8)
#define    LDA_LATCHED            (1<<8)

#define WM8776_R04_DAC_VOLUME_R (0x04<<9)/* Right DAC digital volume control */
#define   RDA_MASK                (255<<0) /* Right digital attenuation. 0xff: 0dB  */
#define   RDA_UPDATED_MASK        (1<<8)   /* Simultaneous update of attenuation latches */
#define    RDA_NOT_LATCHED        (0<<8)
#define    RDA_LATCHED            (1<<8)

#define WM8776_R05_DAC_VOLUME_ALL (0x05<<9)/* Both left&right DAC digital volume control */
#define   MASTDA_MASK             (255<<0) /* Right digital attenuation. 0xff: 0dB */
#define   MASTDA_UPDATED_MASK     (1<<8)   /* Simultaneous update of attenuation latches */
#define    MASTDA_NOT_LATCHED     (0<<8)
#define    MASTDA_LATCHED         (1<<8)

#define WM8776_R06_DAC_PHASE (0x06<<9) /* DAC phase */
#define   PH_MASK                 (3<<0) /* DAC output inversion */
#define    DACL_NORMAL            (0<<0) /* Left DAC normal */
#define    DACL_INVERT            (1<<0) /* Left DAC value inverted */
#define    RACL_NORMAL            (0<<1) /* Right DAC normal */
#define    RACL_INVERT            (1<<1) /* Right DAC value inverted */

#define WM8776_R07_DAC_CONTROL (0x07<<9) /* Attenuator control mode */
#define   DCZEN_MASK              (1<<0) /* DAC digital volume zero cross enable */
#define    DCZEN_DISABLED         (0<<0)
#define    DCZEN_ENABLED          (0<<0
#define   ATC_MASK                (1<<1)
#define    ATC_NORMAL             (0<<1) /* Right channel use right channel attenuation */
#define    ATC_BOTH               (1<<1) /* Left channel attenuation controls both channels */
#define   IZD_MASK                (1<<2) /* Infinite zero mute enable */
#define    IZD_DISABLED           (0<<2)
#define    IZD_ENABLED            (1<<2) /* Disable DACs if 1024 consecutive 0s. Will be renabled automatically */
#define   TOD_MASK                (1<<3) /* DAC/ADC analogue zero cross detect timeout */
#define    TOD_ENABLED            (0<<3)
#define    TOD_DISABLED           (1<<3)
#define   PL30_MASK               (15<<4) /* Routing of audio interface to DACs */
/* Not all values are really useful, I only code the ones that are realistic */
#define    PL30_MUTE              (0<<4)  /* Left->mute, Right->mute */
#define    PL30_L_ONLY            (1<<4)  /* Left->left, Right->mute */
#define    PL30_R_ONLY            (8<<4)  /* Left->mute, Right->right */
#define    PL30_NORMAL            (9<<4)  /* Left->left, Right->left */
#define    PL30_STEREO2MONO       (15<<4) /* (Left+Right/2)->left,right */
#define    PL30_L2BOTH            (5<<4)  /* Left->left,right */
#define    PL30_R2BOTH            (10<<4) /* Right->left,right */

#define WM8776_R08_DAC_MUTE (0x08<<9) /* Mute modes */
/* Prefered way of muting the WM8776 as it does a "fade out" */
#define   DMUTE_MASK              (1<<0) /* DAC mute */
#define    DMUTE_NORMAL           (0<<0)
#define    DMUTE_MUTED            (1<<0)

#define WM8776_R09_DAC_CONTROL (0x09<<9) /* De-emphasis mode */
#define   DEEMPTH_MASK            (1<<0)
#define    DEEMPTH_NORMAL         (0<<0)
#define    DEEMPTH_ENABLED        (1<<0)

#define WM8776_R10_DAC_INTERFACE_CONTROL (0x0a<<9) /* DAC interface control ******/
#define   DACFMT_MASK             (3<<0) /* Interface format */
#define    DACFMT_RIGHT_JUSTIFIED (0<<0)
#define    DACFMT_LEFT_JUSTIFIED  (1<<0)
#define    DACFMT_I2S             (2<<0)
#define    DACFMT_DSP             (3<<0)
#define   DACLRP_MASK 4 /* Left/right polarity */
/* In left/right/i2s modes */ 
#define    DACLRP_NORMAL          (0<<2)
#define    DACLRP_INVERTED        (1<<2)
/* In DSP mode */ 
#define    DACLRP_DSP_EARLY       (0<<2)          
#define    DACLRP_DSP_LATE        (1<<2)
#define   DACBCP_MASK 8 /* BCLK polarity (DSP modes) */
#define    DACBCP_NORMAL          (0<<3)
#define    DACBCP_INVERTED        (1<<3)
#define   DACWL_MASK 0x30 /* DAC word length */
#define    DACWL_16BITS           (0<<4)
#define    DACWL_20BITS           (1<<4)
#define    DACWL_24BITS           (2<<4)
#define    DACWL_32BITS           (3<<4)

#define WM8776_R11_ADC_INTERFACE_CONTROL (0x0b<<9) /* ADC interface control ******/
#define   ADCFMT_MASK             (3<<0) /* Interface format */
#define    ADCFMT_RIGHT_JUSTIFIED (0<<0)
#define    ADCFMT_LEFT_JUSTIFIED  (1<<0)
#define    ADCFMT_I2S             (2<<0)
#define    ADCFMT_DSP             (3<<0)
#define   ADCLRP_MASK 4 /* Left/right polarity */
/* In left/right/i2s modes */ 
#define    ADCLRP_NORMAL          (0<<2)
#define    ADCLRP_INVERTED        (1<<2)
/* In DSP mode */ 
#define    ADCLRP_DSP_EARLY       (0<<2)
#define    ADCLRP_DSP_LATE        (1<<2)
#define   ADCBCP_MASK 8 /* BCLK polarity (DSP modes) */
#define    ADCBCP_NORMAL          (0<<3)
#define    ADCBCP_INVERTED        (1<<3)
#define   ADCWL_MASK 0x30 /* ADC word length */
#define    ADCWL_16BITS           (0<<4)
#define    ADCWL_20BITS           (1<<4)
#define    ADCWL_24BITS           (2<<4)
#define    ADCWL_32BITS           (3<<4)
/* Everything from R10 applies here too */
#define   ADCMCL_MASK             (1<<6)
#define    ADCMCLK_NORMAL         (0<<6)
#define    ADCMCLK_INVERTED       (1<<6)
/* ADC high pass filter */
#define   ADCHPD_MASK             (1<<8) 
#define    ADCHPD_ENABLED         (0<<8)
#define    ADCHPD_DISABLED        (1<<8)

#define WM8776_R12_MASTER_MODE_CONTROL (0x0c<<9) /* Master modes */
/* ADC clock rate */
#define    ADCRATE_MASK           7
#define     ADCRATE_256           (2<<0)
#define     ADCRATE_384           (3<<0)
#define     ADCRATE_512           (4<<0)
#define     ADCRATE_768           (5<<0)
/* ADC oversampling */
#define    ADCOSR_MASK            (1<<3)
#define     ADCOSR_128x           (0<<3)
#define     ADCOSR_64x            (1<<3)
/* DAC clock rate */
#define    DACRATE_MASK           (7<<4)
#define     DACRATE_256           (2<<4)
#define     DACRATE_384           (3<<4)
#define     DACRATE_512           (4<<4)
#define     DACRATE_768           (5<<4)
/* ADC interface control ******/
#define   DACMS_MASK              (1<<8)
#define    DACMS_SLAVE            (0<<8)
#define    DACMS_MASTER           (1<<8)
/* DAC interface control ******/
#define   ADCMS_MASK              (1<<9)
#define    ADCMS_SLAVE            (0<<9)
#define    ADCMS_MASTER           (1<<9)

#define WM8776_R13_POWER_DOWN_CONTROL (0x0d<<9) /* Powerdown mode and ADC/DAC disable */
#define   PDWN_MASK               (1<<0) /* Power down control */
#define    PDWN_NORMAL            (0<<0)
#define    PDWN_POWER_DOWN        (1<<0)
#define   ADCPD_MASK              (1<<1) /* ADC power down */
#define    ADCPD_NORMAL           (0<<1)
#define    ADCPD_POWER_DOWN       (1<<1)
#define   DACPD_MASK              (1<<2) /* DAC power down */
#define    DACPD_NORMAL           (0<<2)
#define    DACPD_POWER_DOWN       (1<<2)
#define   HPPD_MASK               (1<<3) /* Headphones output power down */
#define    HPPD_NORMAL            (0<<3)
#define    HPPD_POWER_DOWN        (1<<3)
#define   AINPD_MASK              (1<<6) /* Analog input power down */
#define    AINPD_NORMAL           (0<<6)
#define    AINPD_POWER_DOWN       (1<<6)

#define WM8776_R14_ADC_ATTENUATION_L (0x0e<<9) /* Left ADC channel gain */
#define   LAG_MASK                (255<<0) /* Left channel attenuation. 0xCF = 0dB */
#define   ZCLA_MASK               (1<<8)   /* ADC zero cross enable */
#define    ZCLA_DISABLED          (0<<8)
#define    ZCLA_ENABLED           (1<<8)

#define WM8776_R15_ADC_ATTENUATION_R (0x0f<<9) /* Right ADC channel gain */
#define   RAG_MASK                (255<<0) /* Right channel attenuation. 0xCF = 0dB */
#define   ZCRA_MASK               (1<<8)   /* ADC zero cross enable */
#define    ZCRA_DISABLED          (0<<8)
#define    ZCRA_ENABLED           (1<<8)

#define WM8776_R16_ADC_ANALOG_AUTO_LEVEL_CONTROL (0x10<<9) /* ADC analog gain auto control */
#define   LCT_MASK                (15<<0)  /* target level in 1db steps, default is 11 (-5db) */
#define   MAXGAIN_MASK            (3<<4)   /* Max ALC PGA gain. Default 111: +24dB, -4dB increments */
#define   LCSEL_MASK              (3<<7)   /* ADC PGA level control mode */
#define    LCSEL_LIMITER          (0<<7)
#define    LCSEL_ALC_RIGHT        (1<<7)
#define    LCSEL_ALC_LEFT         (2<<7)
#define    LCSEL_ALC_STEREO       (3<<7)

/* ADC's automatic analog attenuation (limiter) */
#define WM8776_R17_ADC_ANALOG_AUTO_LEVEL_CONTROL (0x11<<9)
#define   HLD_MASK                (15<<0)  /* Auto Level Control hold time, default 0 */
#define   ALCZC_MASK              (1<<7)   /* PGA zero cross enable */
#define    ALCZC_DISABLED         (0<<7)
#define    ALCZC_ENABLED          (1<<7)
#define   LCEN_MASK               (1<<8)   /* Enable DAC PGA auto gain (compressor/limiter) control */
#define    LCEN_DISABLED          (0<<8)
#define    LCEN_ENABLED           (1<<8)

/* Automatic level control attack/decay times. Be they have a slightly different
 * meanings in automatic gain and limiter modes. */
#define WM8776_R18_ADC_AUTOLEVEL_ATTACK_DECAY (0x12<<9)
#define   ATK_MASK                (15<<0) /* Attack time. Default is 2 (33.6ms in ALC, 1ms in Limiter) */
#define   DCY_MASK                (15<<4) /* Decay time. Default is 3, (269ms in ALC, 9.6ms in limiter) */
 
 /* Noise gate */
#define WM8776_R19_NOISE_GATE (0x13<<9)
#define   NGAT_MASK               (1<<0) /* Noise gate */
#define    NGAT_DISABLE           (0<<0)
#define    NGAT_ENABLE            (1<<0)
#define   NGTH_MASK               (7<<2) /* Default 0, -78dbFS */

 /* ADC Limiter transient window length */
#define WM8776_R20_ADC_LIMITER_TRANSIENT_WINDOW (0x14<<9)
#define   MAXATTEN_MASK           (15<<0) /* Max ALC PGA attenuation. */ 
#define   TRANWIN_MASK            (7<<4)  /* Length of transcient window. Default is 2, 125us */

/* ADC input mixer and powerdown channel mute */
#define WM8776_R21_ADC_INPUT_MIXER_AND_MUTE (0x15<<9)
#define   AMX_MASK                (31<<0) /* ADC left channel input mixer control bits */
#define    AMX_AIN1               (1<<0)  /* Analog input 1 */
#define    AMX_AIN2               (1<<1)  /* Analog input 2 */
#define    AMX_AIN3               (1<<2)  /* Analog input 3 */
#define    AMX_AIN4               (1<<3)  /* Analog input 4 */
#define    AMX_AIN5               (1<<4)  /* Analog input 5 */
#define   MUTERA_MASK             (1<<6)  /* Mute right channel */
#define    MUTERA_NORMAL          (0<<6)
#define    MUTERA_MUTED           (1<<6)
#define   MUTELA_MASK             (1<<7)  /* Mute left channel */
#define    MUTELA_NORMAL          (0<<7)
#define    MUTELA_MUTED           (1<<7)
#define   LRBOTH_MASK             (1<<8)  /* Mute both channels */
#define    LRBOTH_NORMAL          (0<<8)
#define    LRBOTH_MUTED           (1<<8)

/* Output mux */
#define WM8776_R22_OUTPUT_MIXER (0x16<<9)
#define   MX_MASK                 (7<<0)
#define    MX_DAC                 (1<<0) /* DAC output (combination of analog inputs) */
#define    MX_AUX                 (2<<0) /* Auxiliary input */
#define    MX_BYPASS              (4<<0) /* Bypass */

/* WM8776 Reset */
#define WM8776_R23_RESET (0x17<<9) /* Writting any value causes a reset */


/* Functions *****************************************************************/

void wm8776_init(void);
void wm8776_deinit(void);
void wm8776_send(uint16_t data);
void wm8776_set_digital_volume(uint8_t volume);
uint8_t wm8776_get_digital_volume(void);

#endif /* WM8776_H */
