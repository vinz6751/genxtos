/* SD Card control for the Gavin chip of the A2560 Foenix computers (except the A2560M)
 * Author: Vincent Barrilliot
 * Licence: MIT
 */

/* #define ENABLE_KDEBUG */

#include <stdint.h>
#include "emutos.h"

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "../foenix/foenix.h"
#include "spi.h"
#include "../foenix/gavin_sdc.h"
#include "../foenix/a2560.h"
#include "../foenix/regutils.h"

/* Nothing needed there, it's all handled by GAVIN */
static void spi_clock_sd(void) { }
static void spi_clock_mmc(void) { }
static void spi_clock_ident(void) { }
static void spi_cs_assert(void) { }
static void spi_cs_unassert(void) { }

static void spi_initialise(void)
{
	if (R16(SDC_STATE) & SDC_STATE_ABSENT)
		KDEBUG(("Carte absente!\n"));
	else
		KDEBUG(("Carte présente.\n"));
	
	gavin_sdc_controller->control = 1; // Reset
	gavin_sdc_controller->control = 0; // Reset
	
    /* We use plain SPI and EmuTOS's SD layer on top of it */
    gavin_sdc_controller->transfer_type = SDC_TRANS_DIRECT;
}

uint8_t clock_byte(uint8_t value);
uint8_t clock_byte(uint8_t value)
{
    gavin_sdc_controller->data = value;
    gavin_sdc_controller->transfer_control = SDC_TRANS_START;
    while (gavin_sdc_controller->transfer_status & SDC_TRANS_BUSY)
		;
    return gavin_sdc_controller->data;
}


static void spi_send_byte(uint8_t c)
{
    (void)clock_byte(c);
}


static uint8_t spi_recv_byte(void)
{
    return clock_byte(0xff);
}


static void led_on(void) {
    a2560_disk_led(true);
}


static void led_off(void) {
    a2560_disk_led(false);
}


const SPI_DRIVER spi_gavin_driver = {
    spi_initialise,
    spi_clock_sd,
    spi_clock_mmc,
    spi_clock_ident,
    spi_cs_assert,
    spi_cs_unassert,
    spi_send_byte,
    spi_recv_byte,
    led_on,
    led_off
};

#endif
