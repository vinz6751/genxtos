#include <stdio.h>
#include "ym262.h"

uint16_t blocks[128];
uint16_t fnums[128];
uint32_t scale[128];

int main(void)
{
	uint16_t i;

	int note_number = 47;

	/* Compute frequencies for the scale */
	ym262_create_note_to_freq_lut(4400, scale);
	//for (int i=0; i<128; i++)
	//	printf("%d: %ld\n", i, scale[i]);

	ym262_reset();

	/* Store block/fnum for each note */

	for (i=0; i<128; i++)
	{
		ym262_get_block_fnum_by_freq(scale[i], &blocks[i], &fnums[i]);
#if YM262_DEBUG
		printf("%d: freq:%d block:%d fnum:%d\n", i, scale[i], blocks[i], fnums[i]);
#else		
		// printf("%d: freq:%lu block:%d fnum:%d\n", i, scale[i], blocks[i], fnums[i]);
#endif		
	}

	ym262_reset();
	//opl3_test();
	ym262_channel_off(0);
	//printf("Set OPL3 mode");
	//ym262_write_reg(YM262_REG_OPL3_EN, YM262_OPL3_EN_MASK);

	/* Global settings*/
	ym262_set_vibrato_depth(1);
	ym262_set_tremolo_depth(1);

	/* Program a basic sound */
	// Modulator
	ym262_set_key_scale_level(0, 1, 0);
	ym262_set_oscfreqmult(0, 1, 3);
	ym262_set_env_type(0, 1, YM262_EGT_SUSTAINED);
	ym262_set_attack_rate(0, 1, 15);
	//printf("Set decay rate");
	ym262_set_decay_rate(0, 1, 15);
	//printf("Set slope rate");
	ym262_set_slope_rate(0, 1, 0);
	//printf("Set release rate");
	ym262_set_release_rate(0, 1, 5);
	//printf("Set volume");
	ym262_set_osc_volume(0, 1,20);
	ym262_set_vibrato(0, 1, false);
	ym262_set_tremolo(0, 1, false);
	ym262_set_osc_connection(0, YM262_SYN_FM);
	//printf("Set oscfreqmult");
	ym262_set_feedback(0,3);

#if 1
	ym262_set_key_scale_level(0, 0, 0);
	ym262_set_oscfreqmult(0, 0, 2);
	ym262_set_env_type(0, 0, YM262_EGT_SUSTAINED);
	ym262_set_attack_rate(0, 0, 15);
	//printf("Set decay rate");
	ym262_set_decay_rate(0, 0, 2);
	//printf("Set slope rate");
	ym262_set_slope_rate(0, 0, 4);
	//printf("Set release rate");
	ym262_set_release_rate(0, 0, 5);
	//printf("Set volume");
	ym262_set_osc_volume(0, 0,63);
	ym262_set_vibrato(0, 0, false);
	ym262_set_tremolo(0, 0, false);
	//printf("Set oscfreqmult");
#endif

	ym262_set_block_fnum(0, blocks[note_number], fnums[note_number]);
	//ym262_set_block_fnum(0, 4, 512+65);
	printf("ym262_set_output(0, YM262_OUTSEL_LR)");
	ym262_set_output(0,YM262_OUTSEL_LR);
	printf("ym262_set_osc_connection(0, FM)");
	ym262_set_osc_connection(0, YM262_SYN_ADD);

	printf("channel_on(0)");
	ym262_channel_on(0);
    printf("Done !");
	return 0;
}


