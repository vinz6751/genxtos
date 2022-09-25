/*
 * WM8776 - Functions for the WM8776 Audio wm8776/mixer
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *  PJW  Peter Weingartner
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.
 */

#include <stdint.h>
#include "foenix.h"
#include "wm8776.h"


/* We can't read from the WM8776, so we shadow the registers. */
static uint16_t wm8776_regs[23];

/* Convenience */
static volatile uint16_t * const wm8776 = WM8776_PORT;

/* Initialisation data for the WM8776 CODEC */
static uint16_t wm8776_init_data[] = {
    /* Master reset */
    WM8776_R23_RESET,
    /* Power options */
    WM8776_R13_POWER_DOWN_CONTROL | PDWN_NORMAL | ADCPD_NORMAL | DACPD_NORMAL | HPPD_NORMAL | AINPD_NORMAL,
    /* Input mixer, ADC mute */
    WM8776_R21_ADC_INPUT_MIXER_AND_MUTE | AMX_AIN1 | AMX_AIN2 | AMX_AIN3 | AMX_AIN4 | AMX_AIN5 | MUTERA_NORMAL | MUTELA_NORMAL,
    /* Auto-level control */
    WM8776_R17_ADC_ANALOG_AUTO_LEVEL_CONTROL | LCEN_ENABLED | HLD_MASK,
    /* Output mixer */
    WM8776_R22_OUTPUT_MIXER | MX_DAC | MX_AUX | MX_BYPASS,
    /* DAC interface control */
    WM8776_R10_DAC_INTERFACE_CONTROL | DACFMT_I2S,
    /* ADC interface control */
    WM8776_R11_ADC_INTERFACE_CONTROL | ADCFMT_I2S,
    /* Master mode control */
    WM8776_R12_MASTER_MODE_CONTROL | ADCRATE_768 | DACRATE_512,
 };


void wm8776_init(void)
{
    int i;

    /* TODO: initalize wm8776_regs with documented default values */

    for (i = 0; i < sizeof(wm8776_init_data)/sizeof(uint16_t); i++)
    {
        wm8776_send(wm8776_init_data[i]);
    }
}


void wm8776_deinit(void)
{
    // TODO: fade out using DMUTE then power off, but it may require timer etc.
}


void wm8776_send(uint16_t data)
{
    wm8776_regs[WM8776_REGNR(data)] = WM8776_VALUE(data); 
    
    /* Gavin/Beatrix take care of the communication with the WM8776 (how nice).
     * We only need to write the 16 bits (14 of them are really relevant).
     * MSB of the port tells us weither the line is busy, if so we have to wait.
     *
     * Possible enhancement: queue messages and use interrupts. */ 
    while (*wm8776 & 0x8000)
        ;
    *wm8776 = data;
}


void wm8776_set_digital_volume(uint8_t volume)
{
    wm8776_send(WM8776_R05_DAC_VOLUME_ALL | (0xFF - (volume & MASTDA_MASK)));

    /* Is this really correct ? above 0x79 we'll be amplifying */
    wm8776_send(WM8776_R02_HEADPHONE_ATTENUATION_ALL | ((volume >> 1) & HPRMASTA_MASK));
}


uint8_t wm8776_get_digital_volume(void)
{
    return 255- (wm8776_regs[5/*WM8776_R05_DAC_VOLUME_ALL*/] && MASTDA_MASK);
}
