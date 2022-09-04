/*
 * bq4802LY.c - bq4802LY real time clock handling routines
 *
 *
 * Copyright (C) 2022 The EmuTOS development team
 *
 * Authors:
 *  Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the BSD licence.
 * See doc/license.txt for details.
 */

#include <stdint.h>
#include "bq4802ly.h"
#include "a2560u.h"

typedef void (*tick_handler_t)(void);

uint32_t bq4802ly_ticks;
void (*bq4802ly_tick_handler)(void);


/* Private stuff */ 
static struct bq4802ly_t* const bq4802ly = (struct bq4802ly_t *)BQ4802LY_BASE;
static void stop(uint8_t *control);
static void start(uint8_t control);
static uint8_t i_to_bcd(uint16_t n);
static uint16_t bcd_to_i(uint8_t bcd);


void bq4802ly_init(void)
{
    a2560u_irq_disable(INT_RTC);

    /* Make sure the RTC is on. Yes the way this works is awkward :) */
    bq4802ly->control = BQ4802LY_STOP;

    bq4802ly_set_tick_rate(BQ4802LY_RATE_500ms);
    bq4802ly_set_tick_handler(a2560u_rte);
    bq4802ly_enable_ticks(false);
}


/* Set the frequency of the ticking interrupt. Possible values are BQ4802LY_RATE_xxx */
void bq4802ly_set_tick_rate(uint16_t rate)
{
    bq4802ly->rates = rate;
}


/* Enable the RTC interrupts ticking. This doesn't unmask GAVIN interrupts. */
void bq4802ly_enable_ticks(bool enable)
{
    if (enable)
    {
        /* Acknowledge any previous pending interrupt */
        bq4802ly->flags;
        bq4802ly->enables |= BQ4802LY_PIE;
        a2560u_irq_enable(INT_RTC);
    }
    else
    {
        bq4802ly->enables &= BQ4802LY_PIE;
        a2560u_irq_disable(INT_RTC);
    }
}


uint32_t bq4802ly_get_ticks(void) {
    return bq4802ly_ticks;
}


tick_handler_t bq4802ly_get_tick_handler(void) {
    return bq4802ly_tick_handler;
}

void bq4802ly_set_tick_handler(tick_handler_t handler) {
    bq4802ly_tick_handler = handler;
}


void bq4802ly_set_datetime(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t minute, uint8_t second)
{
    uint8_t  control;
    uint8_t  century_bcd, year_bcd, month_bcd, day_bcd;
    uint16_t century;
    uint8_t  hour_bcd, minute_bcd, second_bcd;
    

    century = year / 100;
    year = year - (century * 100);

    /* Compute the BCD values for the time */
    century_bcd = i_to_bcd(century);
    year_bcd = i_to_bcd(year);
    month_bcd = i_to_bcd(month);
    day_bcd = i_to_bcd(day);
    hour_bcd = i_to_bcd(hour);
//    if (!time->is_24hours && time->is_pm)
//        hour_bcd = hour_bcd | 0x80;
    minute_bcd = i_to_bcd(minute);
    second_bcd = i_to_bcd(second);

    /* Temporarily disable updates to the clock */
    stop(&control);

    /* Set the date in the RTC */
    bq4802ly->century = century_bcd;
    bq4802ly->year = year_bcd;
    bq4802ly->month = month_bcd;
    bq4802ly->day = day_bcd;
    bq4802ly->hours = hour_bcd;
    bq4802ly->minutes = minute_bcd;
    bq4802ly->seconds = second_bcd;

//    if (time->is_24hours)
        control = control | BQ4802LY_2412;

    /* Re-enable updates to the clock */
    start(control);
}


void bq4802ly_get_datetime(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    uint8_t control;
    uint8_t century_bcd, year_bcd, month_bcd, day_bcd;
    uint8_t hour_bcd, minute_bcd, second_bcd;

    stop(&control);
    
    century_bcd = bq4802ly->century;
    year_bcd = bq4802ly->year;
    month_bcd = bq4802ly->month;
    day_bcd = bq4802ly->day;
    hour_bcd = bq4802ly->hours;
    minute_bcd = bq4802ly->minutes;
    second_bcd = bq4802ly->seconds;

    /* Re-enable updates to the clock */
    start(control);
    
    *year = bcd_to_i(century_bcd) * 100 + bcd_to_i(year_bcd);
    *month = bcd_to_i(month_bcd);
    *day = bcd_to_i(day_bcd);
    *hour = bcd_to_i(hour_bcd & 0x7f);
    *minute = bcd_to_i(minute_bcd);
    *second = bcd_to_i(second_bcd);    
}


/* Convert a number from 0 to 99 to BCD
 * n = a binary number from 0 to 99
 * Returns a byte containing n as a BCD number, 0 if error. */
static uint8_t i_to_bcd(uint16_t n)
{
    if (n > 99) 
        return 0; /* Input was out of range */
    
    uint8_t tens = n / 10;
    uint8_t ones = n - (tens * 10);

    return tens << 4 | ones;
}


static uint16_t bcd_to_i(uint8_t bcd)
{
    uint16_t tens = (bcd >> 4) & 0x0f;
    uint16_t ones = bcd & 0x0f;

    if ((ones > 9) || (tens > 9))
    {
        /* Byte was not in BCD... just return a 0 */
        return 0;
    }
    else
    {
        return tens * 10 + ones;
    }
}


static void stop(uint8_t *control)
{
    *control = bq4802ly->control;
    bq4802ly->control = *control | BQ4802LY_UTI;
}


static void start(uint8_t control)
{
    bq4802ly->control = (control & 0x7f) | BQ4802LY_STOP;
}
