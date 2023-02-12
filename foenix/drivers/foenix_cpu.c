
#include <stdint.h>
#include "foenix_cpu.h"


uint32_t cpu_freq; /* CPU frequency */


// Set the 68k's status register
#if defined(__GNUC__)
    #define set_sr(a)                  \
    __extension__                      \
    ({short _r, _a = (a);              \
    __asm__ volatile                   \
    ("move.w sr,%0\n\t"                \
    "move.w %1,sr"                     \
    : "=&d"(_r)        /* outputs */   \
    : "nd"(_a)         /* inputs  */   \
    : "cc", "memory"   /* clobbered */ \
    );                                 \
    _r;                                \
    })

    short cpu_disable_interrupts(void)
    {
        return set_sr(0x2700);
    }

    void cpu_restore_interrupts(short sr)
    {
        set_sr(sr);
    }

#elif defined (__CALYPSI__)
    #include <intrinsics68000.h>

    short cpu_disable_interrupts(void)
    {
        __interrupt_state_t old_sr = __get_interrupt_state();
        __disable_interrupts();
        return (short)old_sr;
    }


    void cpu_restore_interrupts(short sr)
    {
        __restore_interrupt_state((__interrupt_state_t)sr);
    }
#endif


vector_t cpu_set_vector(uint16_t num, vector_t vector)
{
    vector_t oldvector;
    vector_t *addr = (vector_t *) (4L * num);
    oldvector = *addr;

    if(vector != (vector_t)-1) {
        *addr = vector;
    }
    return oldvector;
}
