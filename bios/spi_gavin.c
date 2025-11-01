/* SD Card control for the Gavin chip of the Foenix computer (used by A2560U) 
 * Author: Vincent Barrilliot
 * Licence: MIT
 */

#include "../foenix/foenix.h"

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include <stdint.h>
#include "spi.h"
#include "../foenix/gavin_sdc.h"
#include "../foenix/a2560.h"

/* Nothing needed there, it's all handled by GAVIN */
static void spi_clock_sd(void) { }
static void spi_clock_mmc(void) { }
static void spi_clock_ident(void) { }
static void spi_cs_assert(void) { }
static void spi_cs_unassert(void) { }

static void spi_initialise(void)
{
    /* We use plain SPI and EmuTOS's SD layer on top of it */
    gavin_sdc_controller->transfer_type = SDC_TRANS_DIRECT;
}


static uint8_t clock_byte(uint8_t value)
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
