/*
 * sd.c - SD/MMC card routines
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  RFB   Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "disk.h"
#include "asm.h"
#include "blkdev.h"
#include "delay.h"
#include "gemerror.h"
#include "sd.h"
#include "spi.h"
#include "string.h"
#include "tosvars.h"
#include "a2560u_bios.h"
#include "coldfire.h"

#include "../foenix/regutils.h"

#if CONF_WITH_SDMMC

/* Max number of SD Card readers connected to the system */
#if defined(MACHINE_A2560M)
#define MAX_SD_CARDS 2
#else
#define MAX_SD_CARDS 1
#endif

/*
 *  SD card commands
 */
                            /* Basic command class */
#define CMD0        0           /* GO_IDLE_STATE: response type R1 */
#define CMD1        1           /* SEND_OP_COND: response type R1 */
#define CMD8        8           /* SEND_IF_COND: response type R7 */
#define CMD9        9           /* SEND_CSD: response type R1 */
#define CMD10       10          /* SEND_CID: response type R1 */
#define CMD12       12          /* STOP_TRANSMISSION: response type R1B */
#define CMD13       13          /* SEND_STATUS: response type R2 */
#define CMD58       58          /* READ_OCR: response type R3 */
                            /* Block read command class */
#define CMD16       16          /* SET_BLOCKLEN: response type R1 */
#define CMD17       17          /* READ_SINGLE_BLOCK: response type R1 */
#define CMD18       18          /* READ_MULTIPLE_BLOCK: response type R1 */
                            /* Block write command class */
#define CMD24       24          /* WRITE_BLOCK: response type R1 */
#define CMD25       25          /* WRITE_MULTIPLE_BLOCK: response type R1 */
                            /* Application-specific command class */
#define CMD55       55          /* APP_CMD: response type R1 */
#define ACMD13      13          /* SD_STATUS: response type R2 (in SPI mode only!) */
#define ACMD41      41          /* SD_SEND_OP_COND: response type R1 */
#define ACMD51      51          /* SEND_SCR: response type R1 */

/*
 *  SD card response types
 */
#define R1          1
#define R1B         2
#define R2          3
#define R3          4
#define R7          5

/*
 *  SD error code bits
 */
#define SD_ERR_IDLE_STATE       0x01
#define SD_ERR_ILLEGAL_CMD      0x04

/*
 *  SD timeouts
 */
                            /* a useful macro */
#define msec_to_ticks(msec)     ((msec*CLOCKS_PER_SEC+999)/1000)
                            /* these are byte-count timeout values */
#define SD_CMD_TIMEOUT          8       /* between sending crc & receiving response */
#define SD_CSD_TIMEOUT          8       /* between sending SEND_CSD cmd & receiving data */
                            /* these are millisecond timeout values (see SD specifications v4.10) */
#define SD_POWERUP_DELAY_MSEC   1       /* minimum power-up time */
#define SD_INIT_TIMEOUT_MSEC    1000    /* waiting for card to become ready */
#define SD_READ_TIMEOUT_MSEC    100     /* waiting for start bit of data block */
#define SD_WRITE_TIMEOUT_MSEC   500     /* waiting for end of busy */
                            /* these are derived timeout values in TOS ticks */
#define SD_POWERUP_DELAY_TICKS  msec_to_ticks(SD_POWERUP_DELAY_MSEC)
#define SD_INIT_TIMEOUT_TICKS   msec_to_ticks(SD_INIT_TIMEOUT_MSEC)
#define SD_READ_TIMEOUT_TICKS   msec_to_ticks(SD_READ_TIMEOUT_MSEC)
#define SD_WRITE_TIMEOUT_TICKS  msec_to_ticks(SD_WRITE_TIMEOUT_MSEC)

/*
 *  SD data tokens
 */
#define DATAERROR_TOKEN_MASK    0xf0    /* for reads: error token is 0000EEEE */
#define DATARESPONSE_TOKEN_MASK 0x1f    /* for writes: data response token is xxx0sss1 */
#define START_MULTI_WRITE_TOKEN 0xfc
#define STOP_TRANSMISSION_TOKEN 0xfd
#define START_BLOCK_TOKEN       0xfe

/*
 *  miscellaneous
 */
#define SDV2_CSIZE_MULTIPLIER   1024    /* converts C_SIZE to sectors */
#define DELAY_1_MSEC            delay_loop(loopcount_1_msec)

/*
 *  structures
 */
struct cardinfo {
    UBYTE type;
#define CARDTYPE_UNKNOWN    0
#define CARDTYPE_MMC        1
#define CARDTYPE_SD         2
    UBYTE version;
    UBYTE features;
#define BLOCK_ADDRESSING    0x02
#define MULTIBLOCK_IO       0x01
    SPI_DRIVER *spi_driver; /* Associated driver */
};

/*
 *  globals
 */
static struct cardinfo cards[MAX_SD_CARDS];
static UBYTE response[5];

/*
 *  function prototypes
 */
static void sd_cardtype(struct cardinfo *info);
static int sd_command(UBYTE cmd,ULONG argument,UBYTE crc,UBYTE resp_type,UBYTE *resp, const SPI_DRIVER *spi_driver);
static ULONG sd_calc_capacity(UBYTE *csd, const struct cardinfo *card);
static LONG sd_check(struct cardinfo *card);
static void sd_features(struct cardinfo *info);
static UBYTE sd_get_dataresponse(const SPI_DRIVER *spi_driver);
static int sd_mbtest(struct cardinfo *card);
static LONG sd_read(struct cardinfo *card,ULONG sector,UWORD count,UBYTE *buf);
static int sd_receive_data(UBYTE *buf,UWORD len,UWORD special, const SPI_DRIVER *spi_driver);
static int sd_send_data(UBYTE *buf,UWORD len,UBYTE token,const SPI_DRIVER *spi_driver);
static int sd_special_read(UBYTE cmd,UBYTE *data,const SPI_DRIVER *spi_driver);
static int sd_wait_for_not_busy(LONG timeout,const SPI_DRIVER *spi_driver);
static int sd_wait_for_not_idle(UBYTE cmd,ULONG arg, const SPI_DRIVER *spi_driver);
static int sd_wait_for_ready(LONG timeout,const SPI_DRIVER *spi_driver);
static LONG sd_write(ULONG sector,UWORD count,UBYTE *buf,const struct cardinfo *card);

static const SPI_DRIVER *get_spi_driver_for_drive(UWORD drv);

#if defined(MACHINE_A2560M)
#define LED_ON a2560_sdc1_led(true)
#define LED_OFF a2560_sdc1_led(false)
#endif

/*
 *  initialise sd/mmc bus
 */
void sd_init(void)
{
    SPI_DRIVER *spi_driver;
    UWORD i;

    for (i=0; i<MAX_SD_CARDS; i++) {
        KDEBUG(("sd_init for card %d\n",i));
        struct cardinfo *card = &cards[i];
        spi_driver = (SPI_DRIVER *)get_spi_driver_for_drive(i);
        card->spi_driver = spi_driver;
        KDEBUG(("sd_init for card %d, driver:%p\n",i,spi_driver));
        if (spi_driver) {
            spi_driver->led_on();
            sd_check(card);
            spi_driver->led_off();
        }
    }
}

/*
 *  read/write interface
 */
LONG sd_rw(WORD rw,LONG sector,WORD count,UBYTE *buf,WORD dev)
{
LONG ret;
UBYTE *p = buf;
int i;
SPI_DRIVER *spi_driver;
struct cardinfo *card = &cards[dev];

    spi_driver = card->spi_driver;
    if (spi_driver == NULL)
        return EUNDEV;

    if (count == 0)
        return 0;

    spi_driver->led_on();

    /*
     * retry at most once (to handle reinitialisation)
     */
    for (i = 0; i < 2; i++) {
        /* see if we need to (re)initialise the card after a previous error */
        if (card->type == CARDTYPE_UNKNOWN) {
            if (sd_check(card)) {
                ret = EDRVNR;
                break;
            }
            if (!(rw & RW_NOMEDIACH)) {
                ret = E_CHNG;
                break;
            }
        }

        ret = (rw&RW_RW) ? sd_write(sector,count,p,card) : sd_read(card,sector,count,p);

        if (ret == 0L)
            break;

        if (ret > 0L) {         /* error but not timeout */
            ret = (rw&RW_RW) ? EWRITF : EREADF;
            break;
        }

        /* at this point, it must be a timeout.  we set the return
         * code in case the retry has failed
         */
        ret = (rw & RW_NOMEDIACH) ? EDRVNR : E_CHNG;

        /* for timeouts, card.type should already be set to
         * CARDTYPE_UNKNOWN, but we'll set it here anyway
         * (this is what forces the reinitialisation on retry)
         */
        card->type = CARDTYPE_UNKNOWN;
    }

    if (ret < 0)
        KDEBUG(("sd_rw(%d,%ld,%d,%p,%d) rc=%ld\n",rw,sector,count,p,dev,ret));

    spi_driver->led_off();

    return ret;
}

/*
 *  perform miscellaneous non-data-transfer functions
 */
LONG sd_ioctl(UWORD drv,UWORD ctrl,void *arg)
{
LONG rc = 0;
ULONG *info = arg;
UBYTE cardreg[16];
struct cardinfo *card;
SPI_DRIVER *spi_driver;

    KDEBUG(("sd_ioctl(%d,%d)\n",drv,ctrl));

    card = &cards[drv];
    spi_driver = card->spi_driver;

    if (spi_driver == NULL || drv >= ARRAY_SIZE(cards)) {
        KDEBUG(("Invalid device %d (driver:%p)\n", drv, spi_driver));
        return EUNDEV;
    }

    spi_driver->led_on();

    switch(ctrl) {
    case GET_DISKINFO:
        KDEBUG(("GET_DISKINFO\n"));
        if (sd_special_read(CMD9,cardreg,spi_driver)) {    /* medium could have changed */
            sd_check(card);   /* so try to reset & retry */
            if (sd_special_read(CMD9,cardreg,spi_driver)) {
                card->type = CARDTYPE_UNKNOWN;
                rc = E_CHNG;
                break;
            }
        }
        info[0] = sd_calc_capacity(cardreg, card);
        info[1] = SECTOR_SIZE;
        KDEBUG(("GET_DISKINFO returns size=%ld and sector_size=%ld\n",info[0],info[1]));
        break;
    case GET_DISKNAME:
        KDEBUG(("GET_DISKNAME\n"));
        if (sd_special_read(CMD10,cardreg,spi_driver)) {   /* medium could have changed */
            KDEBUG(("CMD10 failed 1st try\n"));
            sd_check(card);   /* so try to reset & retry */
            if (sd_special_read(CMD10,cardreg,spi_driver)) {
                KDEBUG(("CMD10 failed 2nd try\n"));
                card->type = CARDTYPE_UNKNOWN;
                rc = E_CHNG;
                break;
            }
        }
        if (card->type == CARDTYPE_SD) {
            cardreg[8] = '\0';
            strcpy(arg,(const char *)cardreg+1);
        } else {    /* must be MMC */
            cardreg[9] = '\0';
            strcpy(arg,(const char *)cardreg+3);
        }
        KDEBUG(("GET_DISKNAME returns '%s'\n",cardreg));
        break;
    case GET_MEDIACHANGE:
        KDEBUG(("GET_MEDIACHANGE\n"));
        if (sd_special_read(CMD9,cardreg,spi_driver) == 0)
            rc = MEDIANOCHANGE;
        else {
            if (sd_check(card))  /*  attempt to reset device  */
                card->type = CARDTYPE_UNKNOWN;
            rc = MEDIACHANGE;
        }
        KDEBUG(("GET_MEDIACHANGE returns %ld\n",rc));
        break;
    default:
        rc = ERR;
    }
    KDEBUG(("Return code: %ld\n", rc));
    spi_driver->led_off();

    return rc;
}

/*
 *  perform special read commands for ioctl
 */
static int sd_special_read(UBYTE cmd,UBYTE *data,const SPI_DRIVER *spi_driver)
{
UWORD special = (cmd==CMD9) ? 1 : 0;
int rc = ERR;

    spi_driver->cs_assert();

    if (sd_command(cmd,0L,0,R1,response, spi_driver) == 0)
        if (sd_receive_data(data,16,special, spi_driver) == 0)
            rc = 0;

    spi_driver->cs_unassert();

    return rc;
}

/*
 *  check drive for card present and re-initialise to handle it
 */
static LONG sd_check(struct cardinfo *card)
{
int i, rc;
SPI_DRIVER *spi_driver;

    spi_driver  = card->spi_driver;
    if (spi_driver == NULL)
        return EUNDEV;

    KDEBUG(("Initialising the SPI driver\n"));
    spi_driver->initialise();
    spi_driver->clock_ident();

    /* wait at least 1msec */
    for (i=0;i<10;i++) /* FatFS waits 10ms but EmuTOS only waits 1ms */
        DELAY_1_MSEC;

    /* send at least 74 dummy clocks with CS unasserted (high) */
    spi_driver->cs_unassert();
    for (i = 0; i < 10; i++)
        spi_driver->send_byte(0xff);

    spi_driver->cs_assert();

    /*
     *  if CMD0 doesn't cause a switch to idle state, there's
     *  probably no card inserted, so exit with error
     */
    rc = sd_command(CMD0,0L,0x95,R1,response, spi_driver);
    if ((rc < 0) || !(rc&SD_ERR_IDLE_STATE)) {
        KDEBUG(("CMD0 failed, rc=%d, response=0x%02x\n",rc,response[0]));
        card->type = CARDTYPE_UNKNOWN;
        spi_driver->cs_unassert();
        KDEBUG(("Exiting sd_check()\n"));
        return EDRVNR;
    }

    /*
     *  determine card type, version, features
     */
    sd_cardtype(card);
	sd_features(card);
    KDEBUG(("sd_features: Card info: type %d, version %d, features 0x%02x\n", card->type,card->version,card->features));

    /*
     *  force block length to SECTOR_SIZE if byte addressing
     */
    if (card->type != CARDTYPE_UNKNOWN)
        if (!(card->features&BLOCK_ADDRESSING))
            KDEBUG(("CMD16: set block length to %ld bytes\n", SECTOR_SIZE));
            if (sd_command(CMD16,SECTOR_SIZE,0,R1,response,spi_driver) != 0) {
                KDEBUG(("CMD16 problem"));
                card->type = CARDTYPE_UNKNOWN;
            }

    spi_driver->cs_unassert();

    KDEBUG(("Card info: type %d, version %d, features 0x%02x\n", card->type,card->version,card->features));

    switch(card->type) {
    case CARDTYPE_SD:
        spi_driver->clock_sd();
        break;
    case CARDTYPE_MMC:
        spi_driver->clock_mmc();
        break;
    default:
		KDEBUG(("sd_check: bad drive number\n"));
        return EDRVNR;
    }

    KDEBUG(("sd_check existing successfully\n"));
    return 0L;
}

/*
 *  read one or more blocks
 */
static LONG sd_read(struct cardinfo *card,ULONG sector,UWORD count,UBYTE *buf)
{
LONG i, rc, rc2;
LONG posn, incr;
SPI_DRIVER *spi_driver = card->spi_driver;

    spi_driver->cs_assert();

    /*
     *  handle byte/block addressing
     */
    if (card->features&BLOCK_ADDRESSING) {
        posn = sector;
        incr = 1;
    } else {
        posn = sector * SECTOR_SIZE;
        incr = SECTOR_SIZE;
    }

    KDEBUG(("sd_read(%ld,%d) => sd_read(%ld,%d)\n",sector,count,posn,count));

    rc = 0L;

    /*
     *  can we use multi sector reads?
     */
    if ((count > 1) && (card->features&MULTIBLOCK_IO)) {
        KDEBUG(("sd_read CMD18 reading block %ld (%d)\n",sector,card->features&BLOCK_ADDRESSING ));
        rc = sd_command(CMD18,posn,0,R1,response,spi_driver);
        if (rc == 0L) {
            for (i = 0; i < count; i++, buf += SECTOR_SIZE) {
                KDEBUG(("sd_read reading sector %ld\n",sector+i));
                rc = sd_receive_data(buf,SECTOR_SIZE,0,spi_driver);
                if (rc)
                    break;
            }
            rc2 = sd_command(CMD12,0L,0,R1B,response,spi_driver);
            if (rc == 0)
                rc = rc2;
        }
        else {
            KDEBUG(("sd_read failed to read multiple sectors\n"));
        }
    } else {            /* use single sector */
        KDEBUG(("sd_read reading sector %ld\n",sector));
        for (i = 0; i < count; i++, posn += incr, buf += SECTOR_SIZE) {
            rc = sd_command(CMD17,posn,0,R1,response,spi_driver);
            if (rc == 0L)
                rc = sd_receive_data(buf,SECTOR_SIZE,0,spi_driver);
            if (rc)
                break;
        }
    }

    spi_driver->cs_unassert();

    return rc;
}

/*
 *  write one or more blocks
 *
 *  note: we don't use the pre-erase function, since
 *  it doesn't seem to improve performance
 */
static LONG sd_write(ULONG sector,UWORD count,UBYTE *buf,const struct cardinfo *card)
{
LONG i, rc, rc2;
LONG posn, incr;
SPI_DRIVER *spi_driver = card->spi_driver;

    spi_driver->cs_assert();

    /*
     *  handle byte/block addressing
     */
    if (card->features&BLOCK_ADDRESSING) {
        posn = sector;
        incr = 1;
    } else {
        posn = sector * SECTOR_SIZE;
        incr = SECTOR_SIZE;
    }

    rc = 0L;

    /*
     *  can we use multi sector writes?
     */
    if ((count > 1) && (card->features&MULTIBLOCK_IO)) {
        rc = sd_command(CMD25,posn,0,R1,response,spi_driver);
        if (rc == 0L) {
            for (i = 0; i < count; i++, buf += SECTOR_SIZE) {
                rc = sd_send_data(buf,SECTOR_SIZE,START_MULTI_WRITE_TOKEN,spi_driver);
                if (rc)
                    break;
            }
            rc2 = sd_send_data(NULL,0,STOP_TRANSMISSION_TOKEN,spi_driver);
            if (rc == 0)
                rc = rc2;
        }
    } else {            /* use single sector write */
        for (i = 0; i < count; i++, posn += incr, buf += SECTOR_SIZE) {
            rc = sd_command(CMD24,posn,0,R1,response,spi_driver);
            if (rc == 0L)
                rc = sd_send_data(buf,SECTOR_SIZE,START_BLOCK_TOKEN,spi_driver);
            if (rc)
                break;
        }
    }

    spi_driver->cs_unassert();

    return rc;
}

/*
 *  send a command to the SD card in SPI mode
 *
 *  returns -1  timeout or bad response type
 *          0   OK
 *          >0  error status from response[0]
 */
static int sd_command(UBYTE cmd,ULONG argument,UBYTE crc,UBYTE resp_type,UBYTE *resp, const SPI_DRIVER *spi_driver)
{
    int i, resp_length;

    /*
     *  set up response length
     */
    switch(resp_type) {
    case R1:
    case R1B:
        resp_length = 1;
        break;
    case R2:
        resp_length = 2;
        break;
    case R3:
    case R7:
        resp_length = 5;
        break;
    default:
        return -1;
    }

    /*
     *  the following test serves two functions:
     *  1. it ensures that at least one byte is clocked out before sending
     *     the command.  some cards seem to require this, at least during
     *     the initialisation sequence.
     *  2. it cleans up any residual data that the card may be sending as
     *     a result of a previous command that experienced problems.
     */
    if (sd_wait_for_ready(SD_READ_TIMEOUT_TICKS,spi_driver) < 0)
        return -1;

    /* Send the command byte, argument, crc */
    spi_driver->send_byte((cmd & 0x3f) | 0x40);
    spi_driver->send_byte((argument>>24)&0xff);
    spi_driver->send_byte((argument>>16)&0xff);
    spi_driver->send_byte((argument>>8)&0xff);
    spi_driver->send_byte(argument&0xff);

    /* CRC is ignored by default in SPI mode ... but we always need a stop bit! */
    spi_driver->send_byte(crc|0x01);

    KDEBUG(("cmd:%d %p\n", cmd, argument));

    if (cmd == CMD12)                   /* stop transmission: */
        spi_driver->recv_byte();                /* always discard first byte */

    /* now we look for the response, which starts with a byte with the 0x80 bit clear */
    for (i = 0; i < SD_CMD_TIMEOUT; i++) {
        resp[0] = spi_driver->recv_byte();
        if ((resp[0]&0x80) == 0)
            break;
    }
    if (i >= SD_CMD_TIMEOUT)            /* timed out */
        return -1;

    /*
     *  retrieve remainder of response iff command is legal
     *  (if it's illegal, it's effectively an R1 response type)
     */
    if ((resp[0] & SD_ERR_ILLEGAL_CMD) == 0) {
        for (i = 1; i < resp_length; i++)
            resp[i] = spi_driver->recv_byte();
    }

    /*
     *  for R1B responses, we need to wait for the end of the busy state.
     *  R1B is only set by write-type commands (CMD12, CMD28, CMD29, CMD38)
     *  so we use the write timeout here.
     */
    if (resp_type == R1B)
        if (sd_wait_for_not_busy(SD_WRITE_TIMEOUT_TICKS,spi_driver) < 0)
            return -1;

    return resp[0];
}

/*
 *  receive data block
 *
 *  notes:
 *  1. if 'buf' is NULL, we throw away the received data
 *  2. if 'special' is non-zero, we use the special SD_CSD_TIMEOUT
 *     instead of the standard read timeout
 *
 *  returns -1 timeout or unexpected start token
 *          0   ok
 */
static int sd_receive_data(UBYTE *buf,UWORD len,UWORD special, const SPI_DRIVER *spi_driver)
{
LONG i;
UBYTE token;

    //KDEBUG(("sd_receive_data len:%d\n", len));
    /* wait for the token */
    if (special) {
        for (i = 0; i < SD_CSD_TIMEOUT; i++) {
            token = spi_driver->recv_byte();
            if (token != 0xff)
                break;
        }
    } else {
        ULONG end = hz_200 + SD_READ_TIMEOUT_TICKS;
        while(hz_200 < end) {
            token = spi_driver->recv_byte();
            if (token != 0xff)
                break;
        }
    }
    if (token == 0xff) {
        KDEBUG(("sd_receive_data error\n"));
        return -1;
    }

    /* check for valid token */
    if (token != START_BLOCK_TOKEN) {
        KDEBUG(("sd_receive_data() bad startblock token 0x%02x\n",token));
        return -1;
    }

    /*
     *  transfer data
     */
    if (buf) {
        for (i = 0; i < len; i++)
            *buf++ = spi_driver->recv_byte();
    } else {
        for (i = 0; i < len; i++)
            spi_driver->recv_byte();
    }

    spi_driver->recv_byte();        /* discard crc */
    spi_driver->recv_byte();

    //KDEBUG(("sd_receive_data OK\n"));
    return 0;
}

/*
 *  send data block
 *
 *  returns -1  timeout or bad response token
 *          0   ok
 */
static int sd_send_data(UBYTE *buf,UWORD len,UBYTE token,const SPI_DRIVER *spi_driver)
{
LONG i;
UBYTE rtoken;

    spi_driver->send_byte(token);
    if (token == STOP_TRANSMISSION_TOKEN) {
        spi_driver->recv_byte();    /* skip a byte before testing for busy */
    } else {
        /* send the data */
        for (i = 0; i < len; i++)
            spi_driver->send_byte(*buf++);
        spi_driver->send_byte(0xff);        /* send dummy crc */
        spi_driver->send_byte(0xff);

        /* check the data response token */
        rtoken = sd_get_dataresponse(spi_driver);
        if ((rtoken & DATARESPONSE_TOKEN_MASK) != 0x05) {
            KDEBUG(("sd_send_data() response token 0x%02x\n",rtoken));
            return -1;
        }
    }

    return sd_wait_for_not_busy(SD_WRITE_TIMEOUT_TICKS,spi_driver);
}

/*
 *  determine card type & version
 */
static void sd_cardtype(struct cardinfo *card)
{
int rc;
UBYTE csd[16];
SPI_DRIVER *spi_driver = card->spi_driver;

    card->type = CARDTYPE_UNKNOWN;          /* defaults */
    card->version = 0;

    /*
     *  check first for SDv2
     */
    if ((sd_command(CMD8,0x000001aaL,0x87,R7,response,spi_driver) >= 0)
     && ((response[0]&SD_ERR_ILLEGAL_CMD) == 0)) {
        if ((response[3]&0x0f) != 0x01) {    /* voltage not accepted */
            KDEBUG(("Voltage not accepted\n"));
            return;
        }
        if (response[4] != 0xaa) {           /* check pattern mismatch */
            KDEBUG(("Check pattern mismatch\n"));
            return;
        }
        if (sd_wait_for_not_idle(ACMD41,0x40000000L,spi_driver) != 0) {
            KDEBUG(("ACMD41 failed\n"));
            return;
        }
        card->type = CARDTYPE_SD;
        card->version = 2;
        KDEBUG(("Detected SDv2\n"));
        return;
    }

    /*
     *  check for SDv1
     */
    rc = sd_wait_for_not_idle(ACMD41,0L,spi_driver);
    if (rc == 0) {
        card->type = CARDTYPE_SD;
        card->version = 1;
        return;
    }

    /*
     *  check for MMC
     */
    rc = sd_wait_for_not_idle(CMD1,0L,spi_driver);
    if (rc) {
        card->type = CARDTYPE_UNKNOWN;
        return;
    }
    card->type = CARDTYPE_MMC;

    /*
     *  determine MMC version from CSD
     */
    if (sd_command(CMD9,0L,0,R1,response,spi_driver) == 0)
        if (sd_receive_data(csd,16,1,spi_driver) == 0)
            card->version = (csd[0] >> 2) & 0x0f;
}

/*
 *  determine card features
 */
static void sd_features(struct cardinfo *card)
{
    card->features = 0;

    KDEBUG(("sd_features\n"));

    /*
     *  check SDv2 for block addressing
     */
    if ((card->type == CARDTYPE_SD) && (card->version == 2)) {
        KDEBUG(("Read Operation Code Register\n"));
        if (sd_command(CMD58,0L,0,R3,response,card->spi_driver) != 0) {  /* shouldn't happen */
            KDEBUG(("unexpected condition\n"));
            card->type = CARDTYPE_UNKNOWN;
            card->version = 0;
            return;
        }

        // High Capacity (SDHC/SDXC) ? If so the addressing is done in sectors rather than bytes
        if (response[1] & 0x40)
            card->features |= BLOCK_ADDRESSING;
        KDEBUG(("card is %s high capacity\n", card->features & BLOCK_ADDRESSING ? "" : "not"));
    }

    /*
     *  all SD cards support multiple block I/O
     */
    if (card->type == CARDTYPE_SD) {
        card->features |= MULTIBLOCK_IO;
        return;
    }

    /*
     *  check MMC for multiple block I/O support
     *  v3 cards always have it ... but so do some v2 & v1 cards
     */
    if (card->type == CARDTYPE_MMC) {
        if (card->version == 3)
            card->features |= MULTIBLOCK_IO;
        else if (sd_mbtest(card) == 0)
            card->features |= MULTIBLOCK_IO;
    }
}

/*
 *  test if multiple block i/o works
 *  returns 0 iff true
 */
static int sd_mbtest(struct cardinfo *card)
{
    SPI_DRIVER *spi_driver = card->spi_driver;

    /*
     *  see if READ_MULTIPLE_BLOCK/STOP_TRANSMISSION work
     *  if they do, we assume the write stuff works too
     */
    if (sd_command(CMD18,0L,0,R1,response,spi_driver))
        return -1;

    sd_receive_data(NULL,SECTOR_SIZE,0,spi_driver);
    if (sd_command(CMD12,0L,0,R1B,response,spi_driver))
        return -1;

    return 0;
}

/*
 *  calculate card capacity in sectors
 */
static ULONG sd_calc_capacity(UBYTE *csd, const struct cardinfo *card)
{
UBYTE read_bl_len, c_size_mult;
ULONG c_size;

    if ((card->type == CARDTYPE_SD) && ((csd[0]>>6) == 0x01)) {
        c_size = csd[7] & 0x3f;
        c_size = (c_size << 8) + csd[8];
        c_size = (c_size << 8) + csd[9];
        return c_size * SDV2_CSIZE_MULTIPLIER;
    }

    read_bl_len = csd[5] & 0x0f;
    c_size = csd[6] & 0x03;
    c_size = (c_size << 8) + csd[7];
    c_size = (c_size << 2) + ((csd[8] & 0xc0) >> 6);
    c_size_mult = ((csd[9] & 0x03) << 1) + ((csd[10] & 0x80) >> 7);

    /*
     * according to the specs, size (bytes)
     *   = (C_SIZE+1) * 2**(C_SIZE_MULT+2) * 2**READ_BL_LEN
     *
     * therefore, size in sectors is calculated as:
     *   (C_SIZE+1) * 2**(C_SIZE_MULT+2) * 2**READ_BL_LEN / 512
     */
    return (c_size+1) << (c_size_mult+2 + read_bl_len - 9);
}

/*
 *  get the data response.  although it *should* be the byte
 *  immediately after the data transfer, some cards miss the
 *  time frame by one or more bits, so we check bit-by-bit.
 *
 *  idea stolen from the linux driver mmc_spi.c
 */
static UBYTE sd_get_dataresponse(const SPI_DRIVER *spi_driver)
{
ULONG pattern;

        pattern = (ULONG)spi_driver->recv_byte() << 24;    /* accumulate 4 bytes */
        pattern |= (ULONG)spi_driver->recv_byte() << 16;
        pattern |= (ULONG)spi_driver->recv_byte() << 8;
        pattern |= (ULONG)spi_driver->recv_byte();

        /* the first 3 bits are undefined */
        pattern |= 0xe0000000L;             /* first 3 bits are undefined */

        /* left-adjust to leading 0 bit */
        while(pattern & 0x80000000L)
            pattern <<= 1;

        /* right-adjust to put code into bits 4-0 */
        pattern >>= 27;

        return LOBYTE(pattern);
}

/*
 *  initialisation function:
 *      loops, issuing command & waiting for card to become "un-idle"
 *
 *  assumes that input cmd is 1 or 41 and, if it's 41,
 *  it's ACMD41 and so must be preceded by CMD55
 *
 *  returns 0   ok
 *          -1  timeout
 */
static int sd_wait_for_not_idle(UBYTE cmd,ULONG arg, const SPI_DRIVER *spi_driver)
{
ULONG end = hz_200 + SD_INIT_TIMEOUT_TICKS;

    while(hz_200 < end) {
        if (cmd == ACMD41)
            if (sd_command(CMD55,0L,0,R1,response,spi_driver) < 0)
                break;
        if (sd_command(cmd,arg,0,R1,response,spi_driver) < 0)
            break;
            DELAY_1_MSEC;
        if ((response[0] & SD_ERR_IDLE_STATE) == 0)
            return 0;
    }

    return -1;
}

/*
 *  wait for not busy indication
 *
 *  note: timeout value is in ticks
 *
 *  returns -1  timeout
 *          0   ok
 */
static int sd_wait_for_not_busy(LONG timeout,const SPI_DRIVER *spi_driver)
{
ULONG end = hz_200 + timeout;
UBYTE c;

    while(hz_200 < end) {
        c = spi_driver->recv_byte();
        if (c != 0x00)
            return 0;
    }

    return -1;
}

/*
 *  wait for ready indication
 *
 *  note: timeout value is in ticks
 *
 *  returns -1  timeout
 *          0   ok
 */
extern uint16_t recvbufi;
static int sd_wait_for_ready(LONG timeout,const SPI_DRIVER *spi_driver)
{
ULONG end = hz_200 + timeout;
UBYTE c;

    while(hz_200 < end) {
        c = spi_driver->recv_byte();
        if (c == 0xff) {
            return 0;
        }
    }

    return -1;
}

/* drv: device number (0=first cart reader) */
static const SPI_DRIVER *get_spi_driver_for_drive(UWORD drv)
{
#if CONF_WITH_VAMPIRE_SPI
    return &spi_vamp_driver;

#elif defined(__mcoldfire__)
# ifdef __mcoldfire__
    /* FIXME: Add and use HAS_SDMMC instead */
    if (cf_spi_chip_select == MCF_VALUE_UNKNOWN)
        return EUNDEV;
# endif
    return &spi_coldfire_driver;
#elif defined(MACHINE_A2560U)
    return &spi_gavin_driver;
#elif defined(MACHINE_A2560M)
    switch (drv) {
        // The onboard SD card is scanned first because we prefer booting from it as it
        // is considered the "hard drive".
        // TODO: check what's present and "prefer" something ?
        case 0: return &spi_a2560m_sd1;
        case 1: return &spi_a2560m_sd0;
        default: return NULL; /* Not supposed to happen */
    }
#else
 #error Must provide a SD card driver
#endif
}

#endif /* CONF_WITH_SDMMC */
