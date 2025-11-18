#include <stdint.h>
#include "foenix.h"
#include "a2560.h"
#include "a2560_debug.h"
#include "cpu.h"
#include "interrupts.h"
#include "regutils.h"


/* Interrupt handlers for each of the IRQ groups */
void *a2560_irq_vectors[IRQ_GROUPS][16];

void a2560_irq_init(void)
{
    int i, j;

    volatile uint16_t *pending = (uint16_t*)IRQ_PENDING_GRP0;
    volatile uint16_t *polarity = (uint16_t*)IRQ_POL_GRP0;
    volatile uint16_t *edge = (uint16_t*)IRQ_EDGE_GRP0;
    volatile uint16_t *mask = (uint16_t*)IRQ_MASK_GRP0;

    for (i = 0; i < IRQ_GROUPS; i++)
    {
        mask[i] = edge[i] = 0xffff;
        pending[i] = 0xffff; /* Acknowledge any pending interrupt */
        polarity[i] = 0;

        for (j=0; j<16; j++)
            a2560_irq_vectors[i][j] = a2560_rts;
    }

    for (i=0x40; i<0x60; i++)
        cpu_set_vector(i,(uint32_t)a2560_rte); /* That's not even correct because it doesn't acknowledge interrupts */
}


void a2560_irq_mask_all(uint16_t *save)
{
    int i;
    uint16_t sr = m68k_set_sr(0x2700);

    for (i = 0; i < IRQ_GROUPS; i++)
    {
        save[i] = ((volatile uint16_t*)IRQ_MASK_GRP0)[i];
        ((volatile uint16_t*)IRQ_MASK_GRP0)[i] = 0xffff;
    }

    m68k_set_sr(sr);
}


void a2560_irq_restore(const uint16_t *save)
{
    int i;

    for (i = 0; i < IRQ_GROUPS; i++)
        ((volatile uint16_t*)IRQ_MASK_GRP0)[i] = save[i];
}

/* Utility functions, don't use directly */
static inline uint16_t irq_group(uint16_t irq_id) { return irq_id >> 4; }
static inline uint16_t irq_number(uint16_t irq_id) { return irq_id & 0xf; }
static inline uint16_t irq_mask(uint16_t irq_id) { return 1 << irq_number(irq_id); }
static inline uint16_t *irq_mask_reg(uint16_t irq_id) { return &((uint16_t*)IRQ_MASK_GRP0)[irq_group(irq_id)]; }
static inline uint16_t *irq_pending_reg(uint16_t irq_id) { return &((uint16_t*)IRQ_PENDING_GRP0)[irq_group(irq_id)]; }
#define irq_handler(irqid) (a2560_irq_vectors[irq_group(irqid)][irq_number(irqid)])


/* Enable an interruption. First byte is group, second byte is bit */
void a2560_irq_enable(uint16_t irq_id)
{
    a2560_debugnl("a2560_irq_enable(0x%02x)", irq_id);
    a2560_irq_acknowledge(irq_id);
    R16(irq_mask_reg(irq_id)) &= ~irq_mask(irq_id);
    a2560_debugnl("a2560_irq_enable: Mask %p=%04x", irq_mask_reg(irq_id), R16(irq_mask_reg(irq_id)));
}


/* Disable an interruption. First byte is group, second byte is bit */
void a2560_irq_disable(uint16_t irq_id)
{
    a2560_debugnl("a2560_irq_disable(0x%02x)", irq_id);
    R16(irq_mask_reg(irq_id)) |= irq_mask(irq_id);
    a2560_debugnl("a2560_irq_disable: Mask %p=%04x", irq_mask_reg(irq_id), R16(irq_mask_reg(irq_id)));
}


void a2560_irq_acknowledge(uint8_t irq_id)
{
    R16(irq_pending_reg(irq_id)) = irq_mask(irq_id);
}


/* Set an interrupt handler for IRQ managed through GAVIN interrupt registers */
void *a2560_irq_set_handler(uint16_t irq_id, void *handler)
{
    void *old_handler = irq_handler(irq_id);
    irq_handler(irq_id) = (void*)handler;

    a2560_debugnl("a2560_irq_set_handler(%04x,%p)", irq_id, handler);
    return old_handler;
}