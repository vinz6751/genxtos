/*
 * bq4802LY.c - bq4802LY real time clock handling routines
 *
 *
 * Copyright (C) 2021 The EmuTOS development team
 *
 * Authors:
 *  Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the BSD licence.
 * See doc/license.txt for details.
 */

#ifndef BQ4802LY_H
#define BQ4802LY_H

#include <stdint.h>
#include <stdbool.h>


struct __attribute__((aligned(16))) bq4802ly_t 
{
    volatile uint8_t seconds;
    uint8_t dummy1;
    volatile uint8_t alarm_seconds;
    uint8_t dummy3;
    volatile uint8_t minutes;
    uint8_t dummy5;
    volatile uint8_t alarm_minutes;
    uint8_t dummy7;
    volatile uint8_t hours;
    uint8_t dummy9;
    volatile uint8_t alarm_hours;
    uint8_t dummy11;
    volatile uint8_t day;
    uint8_t dummy13;
    volatile uint8_t alarm_day;
    uint8_t dummy15;
    volatile uint8_t day_of_week;
    uint8_t dummy17;
    volatile uint8_t month;
    uint8_t dummy19;
    volatile uint8_t year;
    uint8_t dummy21;
    volatile uint8_t rates;
    uint8_t dummy23;
    volatile uint8_t enables;
    uint8_t dummy25;
    volatile uint8_t flags;
    uint8_t dummy27;
    volatile uint8_t control;
    uint8_t dummy29;
    volatile uint8_t century;
};
extern struct bq4802ly_t bq4802;

/* Rate fields and settings */
#define BQ4802LY_RATES_WD    0xf0
#define BQ4802LY_RATES_RS    0x0f
#define BQ4802LY_RATE_976us  0x06
#define BQ4802LY_RATE_4ms    0x08
#define BQ4802LY_RATE_15ms   0x0A
#define BQ4802LY_RATE_500ms  0x0F


/* Enable bits */
#define BQ4802LY_AIE         0x08
#define BQ4802LY_PIE         0x04
#define BQ4802LY_PWRIE       0x02
#define BQ4802LY_ABE         0x01

/* Flag bits */
#define BQ4802LY_AF          0x08
#define BQ4802LY_PF          0x04
#define BQ4802LY_PWRF        0x02
#define BQ4802LY_BVF         0x01

/* Control bits */
#define BQ4802LY_UTI         0x08
#define BQ4802LY_STOP        0x04
#define BQ4802LY_2412        0x02
#define BQ4802LY_DSE         0x01

extern uint32_t rtc_ticks;
typedef void (*tick_handler_t)(void);

void bq4802ly_init(void);
void bq4802ly_set_tick_rate(uint16_t rate);
void bq4802ly_enable_ticks(bool enable);
tick_handler_t bq4802ly_get_tick_handler(void);
/* The handler must save all the register it uses and terminate with rte */
void bq4802ly_set_tick_handler(tick_handler_t handler);
uint32_t bq4802ly_get_ticks(void);
void bq4802ly_set_datetime(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t minute, uint8_t second);
void bq4802ly_get_datetime(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second);

#endif /* BQ4802LY_H */