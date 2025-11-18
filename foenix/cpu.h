/* m68k-specific functions
 * Author: Vincent Barrilliot
 */

 #ifndef FOENIX_CPU_H
#define FOENIX_CPU_H

#include <stdint.h>

/* Indicates if the processor uses a long exceptions frames or not */
extern uint16_t cpu_has_long_frames;


#define m68k_set_sr(a)                         \
__extension__                             \
({int16_t _r, _a = (a);                     \
  __asm__ volatile                        \
  ("move.w sr,%0\n\t"                     \
   "move.w %1,sr"                         \
  : "=&d"(_r)        /* outputs */        \
  : "nd"(_a)         /* inputs  */        \
  : "cc", "memory"   /* clobbered */      \
  );                                      \
  _r;                                     \
})


/*
 * WORD m68k_get_sr(void);
 *   returns the current value of sr.
 */

#define m68k_get_sr()                          \
__extension__                             \
({int16_t _r;                               \
  __asm__ volatile                        \
  ("move.w sr,%0"                         \
  : "=dm"(_r)        /* outputs */        \
  :                  /* inputs  */        \
  : "cc", "memory"   /* clobbered */      \
  );                                      \
  _r;                                     \
})


/*
 * ULONG get_usp(void);
 *   returns the current value of the user stack pointer.
 */

#define get_usp()                         \
__extension__                             \
({uint32_t _r;                            \
  __asm__ volatile                        \
  ("move.l usp,%0"                        \
  : "=a"(_r)         /* outputs */        \
  :                  /* inputs  */        \
  : "memory"         /* clobbered */      \
  );                                      \
  _r;                                     \
})


/*
 * Set the given vector number (not address !) if 'vector' != -1
 * Returns the previous.
 */

uint32_t cpu_set_vector(uint16_t num, uint32_t vector);

/*
 * Setup the CPU for basic use
 */
void m68k_cpu_init(void);


#endif
