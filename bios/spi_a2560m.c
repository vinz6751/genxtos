/* Controller of the A2560M's SD Card */

#if defined(MACHINE_A2560M)

#include <stdint.h>
#include "asm.h" /* just_rts */
#include "delay.h"
#include "spi.h"
#include "../foenix/foenix.h"
#include "../foenix/regutils.h"
#include "../foenix/a2560_debug.h"

//#define a2560_debugnl(...)

/* #define ENABLE_KDEBUG */

#define SDx_CS				0x01		// 1 = Enable 
#define SDx_SLOW 			0x02		// 1 = Slow 400Khz, 0 = 25Mhz
#define SDx_BUSY			0x80		// 1 = Busy

#define SD_CTRL R8(SDC1_BASE)
#define SD_DATA R8(SDC1_BASE+1)

typedef struct {
    volatile int8_t * const ctrl;
    volatile int8_t * const data;
} SD;

static void SD_Wait_SDx_Busy(const SD *sd) {
	while (*sd->ctrl & SDx_BUSY);
}

uint8_t recvbuf[1024];
uint16_t recvbufi;
void dump_spi(void);
void dump_spi(void) {
    int i;
    for (i=0; i<recvbufi; i++) {
        //a2560_debug("%02x ", recvbuf[i]);
        recvbuf[i] = 0;
    }
    recvbufi = 0;
    //a2560_debugnl("--");
}

static void spi_send_byte(const SD *sd, uint8_t data) {
    //a2560_debugnl("spi_send_byte");
    *sd->data = data; // Set the Data in the Transmit Register
    SD_Wait_SDx_Busy(sd); // Wait for the transmit to be over with 
}


static uint8_t spi_recv_byte(const SD *sd) {
    //a2560_debugnl("spi_recv_byte");
    *sd->data = 0xff; // Set the Data in the Transmit Register
    SD_Wait_SDx_Busy(sd); // Wait for the transmit to be over with 
    uint8_t ret = *sd->data;
    //recvbuf[recvbufi++] = ret;
    return ret;			/* Store a received byte */
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready Using - SPI Controler 0                           */
/*-----------------------------------------------------------------------*/
static int SD_wait_ready(const SD *sd) {
    //a2560_debugnl("SD_wait_ready");
	uint8_t d;
	int tmr;
	for (tmr = 500; tmr; tmr--) {	/* Wait for ready in timeout of 500ms */
		d = spi_recv_byte(sd);
		if (d == 0xFF)
            break;
        delay_loop(loopcount_1_msec);
	}
    if (0)
        a2560_debugnl("SD_wait_ready TIMEOUT");

	return tmr ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus - SPI Controler 0               */
/*-----------------------------------------------------------------------*/
static void spi_cs_unassert(const SD *sd) {
//    a2560_debugnl("spi_cs_unassert");
    *sd->ctrl &= ~SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready - SPI Controler 0                  */
/*-----------------------------------------------------------------------*/
static void spi_cs_assert(const SD *sd) {
    //a2560_debugnl("spi_cs_assert");
    *sd->ctrl |= SDx_CS; // SDx_CS = 0 ( Disabled ), SDx = 1 (Active)
    SD_wait_ready(sd);
	return;
}

static void spi_clock_sd(const SD *sd)
{
    a2560_debugnl("spi_clock_sd");
    *sd->ctrl &= ~SDx_SLOW;
}

static void spi_clock_mmc(const SD *sd)
{
    a2560_debugnl("spi_clock_mmc");
    *sd->ctrl |= SDx_SLOW;
}

static void spi_clock_ident(const SD *sd)
{
    a2560_debugnl("spi_clock_ident");
    *sd->ctrl |= SDx_SLOW;
}

static void spi_initialise(const SD *sd)
{
    int i;
    a2560_debugnl("spi_initialise");
    for (i=0 ; i < sizeof(recvbuf); recvbuf[i++] = 0);
    recvbufi = 0;
}


/* SD Card slot 0 (front panel)*/
const SD sd0 = {
    (volatile int8_t * const)SDC_BASE,
    (volatile int8_t * const)SDC_BASE+1
};
static void spi_initialise0(void) { spi_initialise(&sd0); }
static void spi_clock_sd0(void) { spi_clock_sd(&sd0); }
static void spi_clock_mmc0(void) { spi_clock_mmc(&sd0); }
static void spi_clock_ident0(void) { spi_clock_ident(&sd0); }
static void spi_cs_assert0(void) { spi_cs_assert(&sd0); }
static void spi_cs_unassert0(void) { spi_cs_unassert(&sd0); }
static void spi_send_byte0(uint8_t b) { spi_send_byte(&sd0, b); }
static uint8_t spi_recv_byte0(void) { return spi_recv_byte(&sd0); }
const SPI_DRIVER spi_a2560m_sd0 = {
    spi_initialise0,
    spi_clock_sd0,
    spi_clock_mmc0,
    spi_clock_ident0,
    spi_cs_assert0,
    spi_cs_unassert0,
    spi_send_byte0,
    spi_recv_byte0,
    just_rts,
    just_rts
};

/* SD Card slot 1 (on main board)*/
const SD sd1 = {
    (volatile int8_t * const)SDC1_BASE,
    (volatile int8_t * const)SDC1_BASE+1
};
static void spi_initialise1(void) { spi_initialise(&sd1); }
static void spi_clock_sd1(void) { spi_clock_sd(&sd1); }
static void spi_clock_mmc1(void) { spi_clock_mmc(&sd1); }
static void spi_clock_ident1(void) { spi_clock_ident(&sd1); }
static void spi_cs_assert1(void) { spi_cs_assert(&sd1); }
static void spi_cs_unassert1(void) { spi_cs_unassert(&sd1); }
static void spi_send_byte1(uint8_t b) { spi_send_byte(&sd1, b); }
static uint8_t spi_recv_byte1(void) { return spi_recv_byte(&sd1); }
const SPI_DRIVER spi_a2560m_sd1 = {
    spi_initialise1,
    spi_clock_sd1,
    spi_clock_mmc1,
    spi_clock_ident1,
    spi_cs_assert1,
    spi_cs_unassert1,
    spi_send_byte1,
    spi_recv_byte1,
    just_rts,
    just_rts
};

#endif
