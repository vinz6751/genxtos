/* PS/2 controller driver
 * 
 * Done as per https://wiki.osdev.org/%228042%22_PS/2_Controller
 * Authors:
 *	Vincent Barrilliot
 *
 * Limitations:
 * * the driver requires a timer works, because it uses it for timeouts.
 * * cannot have multiple instances of this handler as it uses global vars (but which 68k machine has more than one PS/2 port ?)
 * * ancient AT keyboard with translation enabled in the PS/Controller is not supported.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <stdint.h>
#include <stdbool.h>
#include "ps2.h"

/* #define ENABLE_KDEBUG */
#ifdef ENABLE_KDEBUG
	 void a2560u_debugnl(const char *s,...);
#else
	#define a2560u_debugnl(a,...)
#endif

/* Types */
#define SUCCESS -1
#define ERROR 0;

/* PS/2 Flags */
#define OUTPUT_FULL (1<<0) /* PS/2 output buffer full (that's where we read from) */
#define INPUT_FULL  (1<<1) /* PS/2 input buffer full (that's where we write to) */

/* PS/2 controller commands */
#define CMD_GET_CONFIG		0x20
#define CMD_SET_CONFIG		0x60
#define CMD_PORT1_DISABLE	0xAD
#define CMD_PORT2_DISABLE	0xA7
#define CMD_SELFTEST		0xAA
#define CMD_DEV1_SELFTEST	0xAB
#define CMD_DEV2_SELFTEST	0xA9
#define CMD_DEV1_ENABLE		0xAE
#define CMD_DEV2_ENABLE		0xA8
/* PS/2 device commands */
#define DEVCMD_RESET		0xff
#define DEVCMD_SCAN_ON		0xf4
#define DEVCMD_SCAN_OFF		0xf5
#define DEVCMD_GET_ID		0xf2
#define ACK					0xfa
#define RESEND				0xfe

/* Device flags */
#define STAT_EXISTS			 (1<<0) // Port exists
#define STAT_PORTTEST_PASSED (1<<1) // Port self-test passed
#define STAT_PORT_ENABLED	 (1<<2) // Port is enabled successfully
#define STAT_CONNECTED		 (1<<3) // Device is connected
#define STAT_RESET_OK        (1<<4) // Device reset ok
#define STAT_IDENTIFIED		 (1<<5) // Device identification ok
#define STAT_ATTACHED		 (1<<6) // Driver found and attached
#define STAT_DEV1_ENABLED	 (1<<7) // Device 1 is ready to go
#define STAT_DEV2_ENABLED	 (1<<8) // Device 1 is ready to go


/* Global variables */
/* Target device when sending data */
enum ps2_target
{
	ctrl = 0,
	dev1 = 0,
	dev2 = 1
};


/* Levels of safety checks */
#define ENABLE_DEVICES_RESET_CHECKS    1 /* If you have problems with reseting devices, you can ignore it */
#define ENABLE_DEVICE_IDENTIFICATION   0 /* if you have problems with identifying devices you can make assumptions */
#define ENABLE_SELF_TEST		   	   1 /* Consider the outcome of self tests */
#define RESEND_CONFIG_AFTER_SELFTEST   1 /* OS wiki says that on some keyboard, self test can reset config */
#define FORCE_SELF_TEST_SUCCESS		   1 /* Force the self test results to appear successful */


struct ps2_device_t
{
	enum ps2_target id; /* 0 for device 1, 1 for device 2 */
	uint8_t type[2];    /* PS/2 device type (as the device reports itself) */
	uint16_t status;
	const struct ps2_driver_t *driver; /* Driver currently attached to the device */
	struct ps2_driver_api_t api; /* Interface with the driver */
};

struct ps2_global_t
{
	uint16_t status; /* Status of the PS/2 system */
	bool     dual;    /* Controller is dual channel */
	uint8_t  in_data; /* Last received byte */
	uint8_t  timeout; /* Number of timer ticks to wait before timing out */
	volatile uint32_t *timer; /* Ever increasing counter (from timer) */
	struct ps2_device_t dev1; /* Primary channel / device 1 info */
	struct ps2_device_t dev2; /* Primary channel / device 1 info */
};

/* Local variables */
static struct ps2_global_t L;

/* Global */
struct ps2_api_t ps2_config;
#define P ps2_config /* local convenience */

/* Prototypes */
static bool wait_until_can_write(void);
static bool wait_until_can_read(void);
static bool send_command(uint8_t cmd);
static bool send_command_with_response(uint8_t cmd);
static bool get_data(void);
static uint8_t get_data_no_wait(void);
static bool send_data(enum ps2_target dev, uint8_t data);
static bool send_data1(uint8_t data);
static bool send_data2(uint8_t data);
static bool configure_controller(uint8_t *config);
static bool get_config(uint8_t *config);
static bool send_config(uint8_t config);
static bool selftest(void);
static bool enable_ports(uint8_t *config);
static bool reset_device(struct ps2_device_t *dev);
static void reset_devices(void);
static bool identify_device(struct ps2_device_t *dev);
static bool enable_scanning(void);
static void identify_devices(void);
static void attach_drivers(void);
static bool attach_driver(struct ps2_device_t *dev);
static bool setup_driver_api(struct ps2_device_t *dev);
static bool enable_irqs(uint8_t *config);
static bool disable_irqs(uint8_t *config);


/* Initialise the PS/2 system */
uint16_t ps2_init(void)
{
	uint8_t config = 0;

	/* We get about 960 bytes/s max as the interface is 9600bps 8/1/1 no parity (10 bits).
	 * Period for 1 character is 1/960, we convert to mi
	 * We want to be able to wait 1 character. */
	L.timeout = P.counter_freq / 30;
	if (!L.timeout)
		L.timeout = 1;

	L.status = 0;
	L.dev1.id = dev1;
	L.dev2.id = dev2;
	L.dev1.status = L.dev2.status = 0;

	/* Prevent new incoming data. */
	send_command(CMD_PORT1_DISABLE);
	send_command(CMD_PORT2_DISABLE);
	get_config(&config);
	disable_irqs(&config);

	/* Flush input data */
	while (get_data())
		;

	/* Update config (but don't send yet): enable IRQ, enable set 1 translation */
	if (!configure_controller(&config))
		return L.status;

	/* Test controller and interfaces */
	if (!selftest())
		return L.status;

	/* Enable successfully tested ports and enable IRQs for them */
	if (!enable_ports(&config))
	{
		a2560u_debugnl("Didn't manage to enable ports and IRQs");
		return L.status;
	}

 	send_config(config);

	/* Reset devices attached to enabled ports. This detects whether a device is present. */
	reset_devices();

	/* Flush input data */
	while (get_data())
		;

	identify_devices();

	attach_drivers();

	if (!enable_irqs(&config))
	{
		a2560u_debugnl("Didn't manage to enable IRQs");
		return L.status;
	}

	enable_scanning();

	return L.status;
}


static bool identify_device(struct ps2_device_t *dev)
{
	/* Disable scanning */
	if ((dev->status & STAT_RESET_OK) == 0)
	{
		a2560u_debugnl("Device %d was not reset OK", dev->id);
		return false;
	}

	if (!send_data(dev->id, DEVCMD_SCAN_OFF))
	{
		a2560u_debugnl("Timeout when sending DEVCMD_SCAN_OFF to device %d", dev->id);
		return false;
	}

	if (!get_data())
	{
		a2560u_debugnl("Timeout when getting response to DEVCMD_SCAN_OFF from  device %d", dev->id);
		return false;
	}

	if (L.in_data != ACK)
	{
		a2560u_debugnl("Device %d didn't acknowledge the DEVCMD_SCAN_OFF command, replied 0x%02x", dev->id, L.in_data);
		return false;
	}

	if (!send_data(dev->id, DEVCMD_GET_ID))
	{
		a2560u_debugnl("Timeout when sending DEVCMD_GET_ID to device %d", dev->id);
		return false;
	}

	if (!get_data())
	{
		a2560u_debugnl("Timeout when getting response to DEVCMD_GET_ID from  device %d", dev->id);
		return false;
	}

	if (L.in_data != ACK)
	{
		a2560u_debugnl("Device %d didn't acknowledge the DEVCMD_GET_ID command, replied 0x%02x", dev->id, L.in_data);
		return false;
	}

	/* Read & store device ID */
	if (!get_data())
	{
		a2560u_debugnl("Timeout when getting identification byte 1 for device %d", dev->id);
		return false;
	}
	dev->type[0] = L.in_data;
	if (!get_data())
	{
		dev->type[1] = 0;
		a2560u_debugnl("Timeout when getting identification byte 2 for device %d", dev->id);
	}
	else
		dev->type[1] = L.in_data;

	a2560u_debugnl("Device %d is type 0x%02x 0x%02x", dev->id, dev->type[0], dev->type[1]);

	dev->status |= STAT_IDENTIFIED;
	return SUCCESS;
}


static bool attach_driver(struct ps2_device_t *dev)
{
	if (dev->status & STAT_IDENTIFIED)
	{
		int i;

		for (i = 0 ; i < P.n_drivers ; i++)
		{
			const struct ps2_driver_t *driver = P.drivers[i];
			if (driver->can_drive(dev->type))
			{
				a2560u_debugnl("Found driver %s", driver->name);
				dev->driver = driver;
				dev->status |= STAT_ATTACHED;
				return SUCCESS;
			}
		}
	}

	a2560u_debugnl("Device %d is not identified", dev->id);
	dev->driver = NULL;
	return ERROR;
}


static void on_key_up(uint8_t scancode)
{
	P.os_callbacks.on_key_up(scancode | 0x80);
}


static bool setup_driver_api(struct ps2_device_t *dev)
{
	dev->api.send_data = dev->id == dev1 ? send_data1 : send_data2;
	dev->api.get_data = get_data_no_wait;
	dev->api.malloc = P.malloc;
	dev->api.os_callbacks.on_key_down = P.os_callbacks.on_key_down;
	dev->api.os_callbacks.on_key_up = on_key_up;
	dev->api.os_callbacks.on_mouse = P.os_callbacks.on_mouse;

	while (get_data());

	return dev->driver->init(&dev->api);
}


void ps2_channel1_irq_handler(void)
{
	L.dev1.driver->process(&L.dev1.api, get_data_no_wait());
}


void ps2_channel2_irq_handler(void)
{
	L.dev2.driver->process(&L.dev2.api, get_data_no_wait());
}


static bool wait_until_can_write(void)
{
	uint32_t timeout = *P.counter + L.timeout;

    while ((*P.port_status & INPUT_FULL) == 1)
		if (*P.counter > timeout)
			return ERROR;

    return SUCCESS;
}


static bool wait_until_can_read(void)
{
	uint32_t timeout = *P.counter + L.timeout;

    while ((*P.port_status & OUTPUT_FULL) == 0)
	{
		if (*P.counter > timeout)
			return ERROR;
	}

    return SUCCESS;
}


static bool send_command(uint8_t cmd)
{
	if (wait_until_can_write())
	{
		*P.port_cmd = cmd;
		return SUCCESS;
	}

	return ERROR;
}


static bool get_data(void)
{	
	if (wait_until_can_read())
	{
		L.in_data = get_data_no_wait();
		return SUCCESS;
	}

	return ERROR;
}


static uint8_t get_data_no_wait(void)
{
	return *P.port_data;
}


static bool send_command_with_response(uint8_t cmd)
{
	return send_command(cmd) && get_data();
}


static bool send_data(enum ps2_target dev, uint8_t data)
{
	switch (dev)
	{
		case ctrl:
		/*case dev1: has same value */
			return send_data1(data);
		case dev2:
			return send_data2(data);
		default:
			return false;
	}	
}


static bool send_data1(uint8_t data)
{
	if (wait_until_can_write())
	{
		*P.port_data = data;
		return SUCCESS;
	}

	return ERROR;
}


/* Send data to the second device */
static bool send_data2(uint8_t data)
{
	return send_command(0xd4) && send_data1(data);
}


static bool reset_device(struct ps2_device_t *dev)
{
	uint32_t timeout;
	int tries = 5;

	dev->status &= ~STAT_RESET_OK;

	if ((dev->status & STAT_PORT_ENABLED) == 0)
		return ERROR;

	while (tries--)
	{
		// if (!send_data(dev->id, DEVCMD_SCAN_OFF))
		// {
		// 	a2560u_debugnl("Timeout when sending DEVCMD_SCAN_OFF to device %d", dev->id);
		// 	continue;
		// }

		// if (!get_data() || L.in_data != ACK)
		// {
		// 	a2560u_debugnl("Device %d didn't acknowledge DEVCMD_SCAN_OFF", dev->id);
		// 	continue;
		// }

		a2560u_debugnl("Sending RESET to device %d",dev->id);

		if (!send_data(dev->id, DEVCMD_RESET) && ENABLE_DEVICES_RESET_CHECKS)
			continue;

		if (!get_data())
		{
			a2560u_debugnl("No response");
			continue;
		}

		a2560u_debugnl("Device %d responded to reset with %02x ", dev->id, L.in_data);

		if (L.in_data == RESEND)
			continue;

		if (L.in_data == ACK)
			break;
		else
			a2560u_debugnl("Unexpected response %02x", L.in_data);
	}
	if (tries < 0)
		return ERROR;

	timeout = *P.counter + P.counter_freq * 4 / 5; /* BAT should last 500-750ms  (4/5 = 800ms/1000)*/;
	while (*P.counter < timeout)
	{
		if (get_data())
		{
			if (L.in_data == 0xaa)
			{
				dev->status |= STAT_RESET_OK;
				a2560u_debugnl("Device %d reset successful", dev->id);
				return SUCCESS;
			}
			else
				break;
		}
	}

	a2560u_debugnl("Reset of device %d failed", dev->id);

	/* We can get here because we got no response after RESET ACK, or because the
	 * device reset was not successful */
	return ERROR;
}


static bool enable_scanning(void)
{
	bool res = true;

	if (L.dev1.status & STAT_ATTACHED)
	{
		if (!send_data(dev1, DEVCMD_SCAN_ON))
		{
			a2560u_debugnl("Timeout when sending DEVCMD_SCAN_ON to device 0");
			L.status &= ~STAT_DEV1_ENABLED;
			res = false;
		}
		else
		{
			L.status |= STAT_DEV1_ENABLED;
			get_data(); /* Flush */
			a2560u_debugnl("Device 0 responded to DEVCMD_SCAN_ON with %02x", L.in_data);
		}
	}
	else
		a2560u_debugnl("Device 0 no driver attached");

	if (L.dev2.status & STAT_ATTACHED)
	{
		if (!send_data(dev2, DEVCMD_SCAN_ON))
		{
			a2560u_debugnl("Timeout when sending DEVCMD_SCAN_ON to device 1");
			L.status &= ~STAT_DEV2_ENABLED;
			res = false;
		}
		else
		{
			get_data(); /* Flush */
			L.status |= STAT_DEV2_ENABLED;
			a2560u_debugnl("Device 1 scanning ENABLED");
		}
	}

	return res;
}


static void identify_devices(void)
{
#if ENABLE_DEVICE_IDENTIFICATION
	/* Identify connected devices */
	if (!identify_device(&L.dev1))
	{
		a2560u_debugnl("Assume device 0 is keyboard with set 1 translation");
		L.dev1.type[0] = 0xAB;
		L.dev1.type[1] = 0x41;
		L.dev1.status |= STAT_IDENTIFIED;
	}
	identify_device(&L.dev2);
#else
	/* Checks are disabled, we could be getting rubbish. Assume keyboard for device 1 */
	a2560u_debugnl("Assume device 0 is keyboard with set 1 translation");
	L.dev1.type[0] = 0xAB;
	L.dev1.type[1] = 0x41;
	L.dev1.status |= STAT_IDENTIFIED;

	a2560u_debugnl("Assume device 1 is mouse");
	L.dev2.type[0] = 0x00; /* Useless line. 0x00 is mouse */
	L.dev2.status |= STAT_IDENTIFIED;
#endif
}


static void attach_drivers(void)
{
	if (attach_driver(&L.dev1))
	{
		if (!setup_driver_api(&L.dev1))
		{
			L.status |= ERR_DEV1_DRIVER_ERROR;
			a2560u_debugnl("Error when attaching driver to device 0");
		}
		a2560u_debugnl("Device 0 driver attached: %s", L.dev1.driver->name);
	}
	else
		L.status |= ERR_DEV1_NO_DRIVER_FOUND;

	if (attach_driver(&L.dev2))
	{
		if (!setup_driver_api(&L.dev2))
		{
			L.status |= ERR_DEV2_DRIVER_ERROR;
			a2560u_debugnl("Error when attaching driver to device 1");
		}
		a2560u_debugnl("Device 1 driver attached: %s", L.dev2.driver->name);
	}
	else
		L.status |= ERR_DEV2_NO_DRIVER_FOUND;
}


static bool configure_controller(uint8_t *config)
{
	/* Set controller configuration */
	L.dual = (*config & (1<<5)) ? true : false;
	*config |= 3;	 /* Enable interrupts */
	*config |= 1<<6; /* Enable translation so we have scancode set 1 */

	L.dev1.status = STAT_EXISTS;

	if (L.dual)
	{
		a2560u_debugnl("Dual channel detected");
		L.dev2.status = STAT_EXISTS;
	}

	return true;
}


static bool enable_ports(uint8_t *config)
{
	bool res = true;

	/* Enable ports which have passed self-test, and enable their IRQ on the controller */
	if ((L.dev1.status & STAT_PORTTEST_PASSED) && send_command(CMD_DEV1_ENABLE))
	{
		L.dev1.status |= STAT_PORT_ENABLED;
		*config &= ~0x10;
		a2560u_debugnl("Interface 1 ENABLED");
	}
	else 
		res = false;

	if ((L.dev2.status & STAT_PORTTEST_PASSED) && send_command(CMD_DEV2_ENABLE))
	{
		L.dev2.status |= STAT_PORT_ENABLED;
		*config &= ~0x20;
		a2560u_debugnl("Interface 2 ENABLED");
	}
	else
		res = false;

	if (!res)
		L.status |= ERR_CTRL_ERROR;

	return res;
}


static bool enable_irqs(uint8_t *config)
{
	if (L.dev1.status & STAT_PORT_ENABLED)
		*config |= 1;
	else
		*config &= ~1;

	if (L.dev2.status & STAT_PORT_ENABLED)
		*config |= 2;
	else
		*config &= ~2;

	/* Send updated config with enabled IRQs */
	if (!send_config(*config))
	{
		L.status = ERR_CTRL_ERROR;
		return false;
	}

	return true;
}


static bool disable_irqs(uint8_t *config)
{
	*config &= ~(1|2);

	/* Send updated config with disabled IRQs */
	if (!send_config(*config))
	{
		L.status = ERR_CTRL_ERROR;
		return false;
	}

	return true;
}


static bool selftest(void)
{
	bool res = true;

	if (!(send_command_with_response(CMD_SELFTEST) && L.in_data == 0x55) && ENABLE_SELF_TEST)
	{
		L.status |= ERR_CTRL_SELFTEST_FAILED;
		res = false;
	}

	if (!(send_command_with_response(CMD_DEV1_SELFTEST) || L.in_data == 0x00) && ENABLE_SELF_TEST)
	{
		L.status |= ERR_DEV1_SELFTEST_FAILED;
		res = false;
	}
	else
		L.dev1.status |= STAT_PORTTEST_PASSED;

	if (!(send_command_with_response(CMD_DEV2_SELFTEST) && L.in_data == 0x00) && ENABLE_SELF_TEST)
	{
		L.status |= ERR_DEV2_SELFTEST_FAILED;
		res = false;
	}
	else
		L.dev2.status |= STAT_PORTTEST_PASSED;

	return res;
}


static void reset_devices(void)
{
	if (!reset_device(&L.dev1))
		L.status |= ERR_DEV1_RESET_FAILED;

	if (!reset_device(&L.dev2))
		L.status |= ERR_DEV2_RESET_FAILED;
}


static bool get_config(uint8_t *config)
{
	if (!send_command_with_response(CMD_GET_CONFIG))
	{
		a2560u_debugnl("Timeout when getting config");
		return false;
	}

	*config = L.in_data;
	return true;
}


static bool send_config(uint8_t config)
{
	a2560u_debugnl("Setting new config: 0x%02x", config);
	if (!send_command(CMD_SET_CONFIG))
	{
		a2560u_debugnl("Failed to send CMD_SET_CONFIG to controller");
		return false;
	}

	if (!send_data(ctrl,config))
	{
		a2560u_debugnl("Failed to send config 0x%02x to controller", config);
		return false;
	}

	send_command_with_response(CMD_GET_CONFIG);
	if (L.in_data != config)
	{
		a2560u_debugnl("Unexpected new config 0x%02x", L.in_data);
	}

	return true;
}

