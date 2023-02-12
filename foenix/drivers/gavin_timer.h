#ifndef _GAVIN_TIMER_H_
#define _GAVIN_TIMER_H_

#include <stdint.h>

void gavin_timer_init(void);
void gavin_timer_set(uint16_t timer, uint32_t frequency, bool repeat, void *handler);
void gavin_timer_enable(uint16_t timer, bool enable);

#endif
