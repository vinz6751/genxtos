/* MPU-401 MIDI device
 * 
 * Authors:
 *	Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the GNU Public license v2
 * See doc/license.txt for details.
 */

#ifndef MPU_401_H
#define MPU_401_H

#include <stdint.h>
#include <stdbool.h>

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#define MPU401_DATA           ((volatile uint8_t *)SUPERIO_BASE+0x330)
#define MPU401_STAT           ((volatile uint8_t *)SUPERIO_BASE+0x331)
#define MPU401_CMD            ((volatile uint8_t *)SUPERIO_BASE+0x331)

#define MPU401_STAT_TX_BUSY   0x40
#define MPU401_STAT_RX_EMPTY  0x80

/* Handles the incoming MIDI byte (both on the stack and in d0) */
extern void (*mpu401_rx_handler)(uint8_t byte);

int16_t mpu401_init(void);
void mpu401_set_timeout(uint32_t (*timer)(void),uint16_t timeout);
bool mpu401_wait_can_read(void);
bool mpu401_wait_can_write(void);
bool mpu401_can_read(void);
bool mpu401_can_write(void);
void mpu401_write(uint8_t b);
uint8_t mpu401_read(void);

#endif

#endif
