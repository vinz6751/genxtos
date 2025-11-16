#ifndef FOENIX_TIMER_H
#define FOENIX_TIMER_H

#include <stdint.h>
#include <stdbool.h>

void a2560_timer_init(void);
void a2560_set_timer(uint16_t timer, uint32_t frequency, bool repeat, void *handler);
void a2560_timer_enable(uint16_t timer, bool enable);
uint32_t a2560_run_calibration(uint32_t time);

#endif
