/*
 * spi.h - header for SPI functions used by SD/MMC driver
 *
 * Copyright (C) 2013-2019 The EmuTOS development team
 *
 * Authors:
 *  RFB   Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */
#ifndef _SPI_H
#define _SPI_H

#include "emutos.h"

typedef struct {
    void (*initialise)(void);   /* Initialise for reading a card */
    void (*clock_sd)(void);     /* Configure clock for SD */
    void (*clock_mmc)(void);    /* Configure clock for MMC */
    void (*clock_ident)(void);
    void (*cs_assert)(void);    /* Assert chip select*/
    void (*cs_unassert)(void);  /* Unassert chip select*/
    void (*send_byte)(UBYTE c);
    UBYTE (*recv_byte)(void);
    void (*led_on)(void);       /* Turn on the led of the associated slot */
    void (*led_off)(void);      /* Turn off the led of the associated slot */
    ULONG data;                 /* Free for the driver's use */
} SPI_DRIVER;

#if CONF_WITH_VAMPIRE_SPI
extern const SPI_DRIVER spi_vamp_driver;
#elif defined(__mcoldfire__)
extern const SPI_DRIVER spi_coldfire_driver;
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
extern const SPI_DRIVER spi_gavin_driver;
#elif defined(MACHINE_A2560M)
extern const SPI_DRIVER spi_a2560m_sd0;
extern const SPI_DRIVER spi_a2560m_sd1;
#endif

#endif /* _SPI_H */
