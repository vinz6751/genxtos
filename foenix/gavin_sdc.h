/* Support of GAVIN's handling of SD Card for the A2560U Foenix */

#include <stdint.h>
#include "foenix.h"

/* SD card support */
struct gavin_sdc_controller_t 
{
    uint8_t version;
    uint8_t control;
    uint8_t transfer_type;
    uint8_t transfer_control;
    uint8_t transfer_status;
    uint8_t transfer_error;
    uint8_t data;
    uint8_t sd_xxxa;
    uint8_t sd_xxax;
    uint8_t sd_xaxx;
    uint8_t sd_axxx;
    uint8_t spi_clock;
    uint8_t dummy0c[4];
    uint8_t rx_fifo_data;
    uint8_t dummy11[1];
    uint8_t rx_fifo_h;
    uint8_t rx_fifo_l;
    uint8_t rx_fifo_control;
    uint8_t dummy15[11];
    uint8_t tx_fifo;
    uint8_t dummy21[3];
    uint8_t tx_fifo_control;
};
#define gavin_sdc_controller ((volatile struct gavin_sdc_controller_t* const)SDC_BASE)

