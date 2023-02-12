#ifdef __CALYPSI_TARGET_68000__

#include <intrinsics68000.h>
#include "drivers/bq4802ly.h"
#include "drivers/foenix_cpu.h"
#include "drivers/gavin_irq.h"
#include "drivers/uart16550.h"

#define IRQ_HANDLER __attribute__((interrupt))

void a2560u_rts(void) {
}

#define PENDING0 ((volatile uint16_t * const)IRQ_PENDING_GRP0)
#define PENDING1 ((volatile uint16_t * const)IRQ_PENDING_GRP1)

/* Vicky interrupt handler */
IRQ_HANDLER void a2560u_irq_vicky(void)
{
    vector_t* vector = gavin_irq_vectors[0];

    for (int8_t mask=1; mask; mask <<= 1, vector++)
    {
        if (*PENDING0 & mask)
        {
            *PENDING0 = mask; // Acknowledge
            (*vector)();
        }
    }
}


/* Real time clock interrupt handler, see bq4802ly.c */
extern vector_t bq4802ly_tick_handler;
IRQ_HANDLER void a2560u_irq_bq4802ly(void)
{
    // Acknowledge interrupt on the bq4802LY by reading the flags
    *((volatile uint8_t * const)BQ4802LY_FLAGS);
    
    // Acknowledge the GAVIN interrupt
    *(PENDING1) = 0x8000;

    bq4802ly_ticks++;

    if (bq4802ly_tick_handler)
        bq4802ly_tick_handler();
}


/* PS/2 interrupt handlers, see ps2.c */
void ps2_channel1_irq_handler(void);
void ps2_channel2_irq_handler(void);

IRQ_HANDLER void a2560u_irq_ps2kbd(void)
{
    // Acknowledge the GAVIN interrupt
    *(PENDING1) = 0x0001;
    ps2_channel1_irq_handler();
}

IRQ_HANDLER void a2560u_irq_ps2mouse(void)
{
    // Acknowledge the GAVIN interrupt
    *(PENDING1) = 0x0004;
    ps2_channel2_irq_handler();
}


 /* UART16550 COM1 interrupt handler */
IRQ_HANDLER void a2560u_irq_com1(void)
{
    // This is not completely working as it should because it seems the interrupts don't get acknowledged correctly
    // when we read the Interrupt Status Register (UART0+UART16750_ISR). As a result we can't acknowledge the
    // interrupts so we always get the highest priority one masking the others.
    // So instead of checking interrupts we examine the Line Status Register to find out the cause of the interrupt.
    
    for (;;)
    {
        // Acknowledge the GAVIN interrupt (if any). Hopefully it's not dangerous to do.
        *(PENDING1) = 0x0008;

        // Reading the LSR should also acknowledge the interrupt, but doesn't (on the A2560U at least)
        if (UART0[5/*LSR*/] & 1)
        {
            // Data received
            uart16550_rx_handler(UART0[0/*RBR*/]);
        }
        else
            return;
    }
}

#endif