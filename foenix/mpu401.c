/* MPU-401 MIDI device
 * 
 * Authors:
 *	Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the GNU Public license v2
 * See doc/license.txt for details.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef ENABLE_KDEBUG
	void a2560_debugnl(const char *s,...);
#else
	#define a2560_debugnl(a,...)
#endif

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "foenix.h"
#include "a2560_debug.h"
#include "mpu401.h"

/** Timeout for waiting on the MIDI interface */
static uint16_t timeout_ticks; /* Number of ticks of the timer to wait before timing out */
static uint32_t (*get_ticks)(void);
static uint32_t wait_forever(void) { return 0; } /* Default timer provider */

static bool mpu401_send_command(uint8_t cmd);
static uint8_t mpu401_read_status(void);

/**
 * Initilialize the MIDI port
 *
 * @return 0 on success, any other number is an error
 */
int16_t mpu401_init(void) {
    uint8_t dummy;

    get_ticks = wait_forever;
    timeout_ticks = 0;

    /* Switch the MIDI port of the SuperIO into UART mode */
    if (!mpu401_send_command(0x3F)) {
        return -2;
    }

    /* Reset the MIDI port */
    if (!mpu401_send_command(0xFF)) {
        return -1;
    }

    /* Switch the MIDI port of the SuperIO into UART mode */
    if (!mpu401_send_command(0x3F)) {
        return -2;
    }

    /* Wait for the ACK, which may come if the UART3 was not already in MPU-401 mode*/
    do {
        if (mpu401_wait_can_read()) {
            dummy = mpu401_read();
        }
        else {
            return -3;
        }
    } while (dummy != 0xFE);

    return 0;
}

/** Set the info for managing MIDI time outs. Should be called after init,
 * then can be altered "whenever" (but not in the middle of something waiting)
 * @param timer is a function returning ever increasing tick values
 * @param timeout is how many ticks to wait before timing out
 */
void mpu401_set_timeout(uint32_t (*timer)(void),uint16_t timeout) {
    if (timer)
        get_ticks = timer;
    timeout_ticks = timeout;
}

/**
 * Wait for data to be ready to read
 * @return false if there is a timeout
 */
bool mpu401_wait_can_read(void) {
    uint32_t start = get_ticks();
    do {
        if (mpu401_can_read())
            /* There is data waiting */
            return true;
    } while ((get_ticks() - start) < timeout_ticks);

    /* Time out */
    return false;
}

/**
 * Wait for the MIDI transmitter to be empty
 * @return false upon timeout
 */
bool mpu401_wait_can_write(void) {
    uint32_t start = get_ticks();
    do {
        if (mpu401_can_write())
            return true;
    } while ((get_ticks() - start) < timeout_ticks);

    /* Time out */
    return false;
}

/** Returns true if data has been received */
bool mpu401_can_read(void) {
    return !(mpu401_read_status() & MPU401_STAT_RX_EMPTY);
}

/** Returns true if ready to send */
bool mpu401_can_write(void) {
    return ~(mpu401_read_status() & MPU401_STAT_TX_BUSY);
}

void mpu401_write(uint8_t b) {
    a2560_debugnl("mpu401_write %02x",b);
    *MPU401_DATA = b;
}

uint8_t mpu401_read(void) {
    a2560_debugnl("mpu401_read");
    return *MPU401_DATA;
}

uint8_t mpu401_read_status(void) {
    return *MPU401_STAT;
}

/**
 * Send a command to the MPU-401
 */
static bool mpu401_send_command(uint8_t cmd) {
    a2560_debugnl("mpu401_send_command %02x",cmd);
    *MPU401_CMD = cmd;
    return true;
}
#endif
