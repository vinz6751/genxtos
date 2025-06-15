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

#if defined(MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX) || defined(MACHINE_A2560M)

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