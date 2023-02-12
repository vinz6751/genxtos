#ifndef _FOENIX_CPU_H_
#define _FOENIX_CPU_H_

#include <stdint.h>

extern uint32_t cpu_freq; /* CPU frequency */

typedef void (*vector_t)(void);

vector_t cpu_set_vector(uint16_t num, vector_t vector);
void a2560u_rte(void); /* Actually, just the RTE instruction. Not a function */
void a2560u_rts(void); /* Actually, just the RTS instruction. */

short cpu_disable_interrupts(void);
void cpu_restore_interrupts(short sr);

#endif
