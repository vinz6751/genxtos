#ifndef A2560U_DEBUG_H
#define A2560U_DEBUG_H

#include "../include/config.h"

#ifndef FOENIX_CHANNEL_A_DEBUG_PRINT
//#define FOENIX_CHANNEL_A_DEBUG_PRINT 1
#endif

void a2560_debugnl(const char* __restrict__ s, ...);
void a2560_debug(const char* __restrict__ s, ...);
void a2560u_here(void);

#endif