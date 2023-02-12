#ifndef _GAVIN_IRQ_H
#define _GAVIN_IRQ_H

#include <stdint.h>
#include "../foenix.h"
#include "foenix_cpu.h"

extern vector_t gavin_irq_vectors[IRQ_GROUPS][16];

void gavin_irq_init(void);

/* Backup all IRQ mask registers and mask all interrupts */
void gavin_irq_mask_all(uint16_t *save);
/* Restore interrupts backed up with gavin_irq_mask_all */
void gavin_irq_restore(const uint16_t *save);
void gavin_irq_enable(uint16_t irq_id);
void gavin_irq_disable(uint16_t irq_id);
void gavin_irq_acknowledge(uint8_t irq_id);
void *gavin_irq_set_handler(uint16_t irq_id, void *handler);

#endif