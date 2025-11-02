#ifndef FOENIX_IRQ_H
#define FOENIX_IRQ_H

#include <stdint.h>

void a2560_irq_init(void);
void a2560_irq_mask_all(uint16_t *save);
void a2560_irq_restore(const uint16_t *save);
void a2560_irq_enable(uint16_t irq_id);
void a2560_irq_disable(uint16_t irq_id);
void a2560_irq_acknowledge(uint8_t irq_id);
void *a2560_irq_set_handler(uint16_t irq_id, void *handler);

#endif