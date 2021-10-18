/* PS/2 controller driver
 * 
 * Done as per https://wiki.osdev.org/%228042%22_PS/2_Controller
 * Authors:
 *	Vincent Barrilliot
 *
 * Limitations:
 * * the driver requires that the 200Hz timer works, because it uses it for timeouts. Any other timer should work too.
 * * cannot have multiple instances of this handler as it uses global vars (but which 68k machine has more than one PS/2 port ?)
 * * ancient AT keyboard with translation enabled in the PS/Controller is not supported.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 #include "stdint.h"
 #include "ps2.h"
 
 /* Types */
 #define bool int
 #define SUCCESS -1
 #define ERROR 0;

 /* PS/2 Flags */
 #define OUTPUT_FULL 1<<0 /* PS/2 output buffer full (that's where we read from) */
 #define INPUT_FULL  1<<1 /* PS/2 input buffer full (that's where we write to) */

 /* PS/2 controller commands */
 #define CMD_GET_CONFIG		0x20
 #define CMD_SET_CONFIG		0x60
 #define CMD_PORT1_DISABLE	0xAD
 #define CMD_PORT2_DISABLE	0xA7
 #define CMD_SELFTEST		0xAA
 #define CMD_DEV1_SELFTEST	0xAB
 #define CMD_DEV2_SELFTEST	0xA9
 #define CMD_DEV1_ENABLE	0xAE
 #define CMD_DEV2_ENABLE	0xA8
 /* PS/2 device commands */
 #define DEVCMD_RESET		0xff
 #define DEVCMD_SCAN_ON		0xf4
 #define DEVCMD_SCAN_OFF	0xf5
 #define DEVCMD_GET_ID		0xf2
 #define ACK				0xfa

/* Device flags */
#define STAT_EXISTS			 1<<0 // Port exists
#define STAT_PORTTEST_PASSED 1<<1 // Port self-test passed
#define STAT_PORT_ENABLED	 1<<2 // Port is enabled successfully
#define STAT_CONNECTED		 1<<3 // Device is connected
#define STAT_OK			     1<<4 // Device reset ok
#define STAT_IDENTIFIED		 1<<5 // Device identification ok
#define STAT_ATTACHED		 1<<6 // Driver found and attached



/* Global variables */
/* Target device when sending data */
enum ps2_target
{
	ctrl = 0,
	dev1 = 0,
	dev2 = 1
};

/* Must be a power of 2 and no greater than 256, which should be large enough if the buffer
 * is processed (emptied) on VBL, ie at least 50 times per second. */
#define IN_BUFFER_SIZE 128

struct ps2_device_t
{
	enum ps2_target id; /* 0 for device 1, 1 for device 2 */
	uint8_t type[2];    /* PS/2 device type (as the device reports itself) */
	uint8_t status;
	struct ps2_driver_t *driver; /* Driver currently attached to the device */
	struct ps2_driver_api_t api; /* Interface with the driver */
	/* Reception buffer. Bytes received from the device are stored here */
	uint16_t in_read;
	uint16_t in_write;
	uint8_t  in_buffer[IN_BUFFER_SIZE];
};

struct ps2_global_t
{
	uint8_t status;  /* Status of the PS/2 system */
	uint8_t dual;    /* Controller is dual channel */
	uint8_t in_data; /* Last received byte */
	uint8_t timeout; /* Number of timer ticks to wait before timing out */
	volatile uint32_t *timer; /* Ever increasing counter (from timer) */
	struct ps2_device_t dev1; /* Primary channel / device 1 info */
	struct ps2_device_t dev2; /* Primary channel / device 1 info */
};

/* Local variables */
static struct ps2_global_t L;
static struct ps2_api_t P;

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
static bool reset_device(struct ps2_device_t *dev);
static bool identify_device(struct ps2_device_t *dev);	
static bool attach_driver(struct ps2_device_t *dev);
static bool setup_driver_api(struct ps2_device_t *dev);
static void on_irq(struct ps2_device_t *dev);



/* Initialise the PS/2 system */
uint16_t ps2_init(struct ps2_api_t *p)
{
	uint8_t config;
	bool ok;

	*(&P) = *p;
	L.timeout = 960/P.counter_freq + 1; /* we get about 960 bytes/s max as the interface is 9600bps. we want to be able to wait 1 character*/
	L.status = 0;
	L.dev1.id = dev1;
	L.dev2.id = dev2;

	/* Disable ports, don't care about the result */
	send_command(CMD_PORT1_DISABLE); 
	send_command(CMD_PORT2_DISABLE);

	/* Flush input data */
	while (get_data())
		;

	/* Step 1: set controller configuration */
	ok = ERROR;
	if (send_command_with_response(CMD_GET_CONFIG))
	{
		config = L.in_data;
		L.dual = config & 1<<5;
		config &= 3;	/* Disable interrupts */
		config |= 1<<6; /* Enable translation so we have scancode set 1 */
		if (send_command(CMD_SET_CONFIG) && send_data(ctrl,config) && get_data()) 
		{
			L.dev1.status = STAT_EXISTS;
			if (L.dual)
				L.dev2.status = STAT_EXISTS;
		}
		ok = SUCCESS; 
	}
	if (!ok)
		return ERR_CTRL_ERROR;

	/* Step 2: controller self-test */
	if (send_command_with_response(CMD_SELFTEST) && L.in_data == 0x55)
	{
		/* Resend config in case it got corrupt by self-test. Is this really common? */
		send_command(CMD_SET_CONFIG);
		send_data(ctrl, config);
		get_data();
	}
	else
		return ERR_CTRL_SELFTEST_FAILED;
	

	/* Step 3: interface self-tests */
	if (send_command_with_response(CMD_DEV1_SELFTEST) && L.in_data != 0x00)
		L.dev1.status = STAT_PORTTEST_PASSED;
	else
		L.status |= ERR_CTRL_SELFTEST_FAILED;

	if (send_command_with_response(CMD_DEV2_SELFTEST) && L.in_data != 0x00)
		L.dev2.status = STAT_PORTTEST_PASSED;
	else
		L.status |= ERR_CTRL_SELFTEST_FAILED;

	if (!((L.dev1.status | L.dev1.status) & STAT_PORTTEST_PASSED))
		return L.status;

	/* Step 4: enable ports which have passed self-test, and their IRQ on the PS/2 controller */
	if (L.dev1.status)
	{
		if (send_command(CMD_DEV1_ENABLE))
		{
			config |= 1<<0; /* Enable IRQ */
			L.dev1.status &= ~STAT_PORT_ENABLED;
		}
	}
	if (L.dev2.status)
	{
		if (send_command(CMD_DEV2_ENABLE))
		{
			config |= 1<<1; /* Enable IRQ */
			L.dev2.status &= ~STAT_PORT_ENABLED;
		}
	}
	/* Send updated config */
	/* If no port is operational, abort init */
	if (!(send_command(CMD_SET_CONFIG) && send_data(ctrl, config)))
		return ERR_CTRL_ERROR;

	/* Step 4: reset devices */
	if (!reset_device(&L.dev1))
		L.status |= ERR_DEV1_RESET_FAILED;
	if (!reset_device(&L.dev2))
		L.status |= ERR_DEV2_RESET_FAILED;

	/* Step 5: identify connected devices */
	identify_device(&L.dev1);
	identify_device(&L.dev2);

	/* Step 6: Attach driver */
	if (attach_driver(&L.dev1))
	{
		if (!setup_driver_api(&L.dev1))
			L.status |= ERR_DEV1_DRIVER_ERROR;
	}
	else
		L.status |= ERR_DEV1_NO_DRIVER_FOUND;

	if (attach_driver(&L.dev2))
	{
		if (!setup_driver_api(&L.dev2))
			L.status |= ERR_DEV2_DRIVER_ERROR;
	}
	else
		L.status |= ERR_DEV2_NO_DRIVER_FOUND;

	/* We're all set ! */
	/* Step 7: enable scanning and start processing IRQs */

	return L.status;
}

static bool identify_device(struct ps2_device_t *dev)
{
	/* Disable scanning */
	if (dev->status & STAT_OK
		&& send_data(dev->id, DEVCMD_SCAN_OFF) && L.in_data == ACK
		&& send_command_with_response(DEVCMD_GET_ID) && L.in_data == ACK
		&& get_data())
	{
		/* Read & store device ID */
		dev->type[0] = L.in_data;
		dev->type[1] = get_data() ? L.in_data : 0;
		dev->status |= STAT_IDENTIFIED;

		return SUCCESS;
	}

	return ERROR;	
}	

static bool attach_driver(struct ps2_device_t *dev)
{
	if (dev->status & STAT_IDENTIFIED)
	{
		int i;

		for (i = 0 ; i < P.n_drivers ; i++)
		{
			if (P.drivers[i].can_drive(dev->type))
			{
				dev->driver = &P.drivers[i];
				return SUCCESS;
			}
		}
	}

	dev->driver = NULL;
	return ERROR;
}

static bool setup_driver_api(struct ps2_device_t *dev)
{
	dev->api.send_data = dev == dev1 ? send_data1 : send_data2;
	dev->api.get_data = get_data_no_wait;
	dev->api.malloc = P.malloc;
	dev->api.free = P.free;
	dev->api.on_key_down = P.on_key_down;
	dev->api.on_key_up = P.on_key_up;
	dev->in_read = dev->in_write = 0;

	return dev->driver->init(&dev->api);
}

static void on_irq1(void)
{
	on_irq(&L.dev1);
}

static void on_irq2(void)
{
	on_irq(&L.dev2);
}

static void on_irq(struct ps2_device_t *dev)
{
    uint8_t b;
    int     new_write;

    b = get_data_no_wait();

    /* Put the byte into the circular buffer */
    new_write = (dev->in_write + 1) & (IN_BUFFER_SIZE - 1);
    if (new_write == dev->in_read)
        return; /* buffer full, we loose data */

    dev->in_buffer[new_write] = b;
    dev->in_write = new_write;
}

static void process(struct ps2_device_t *dev)
{
    uint8_t read;
    uint8_t b;

    read = dev->in_read;
    while (read != dev->in_write)
    {
        /* Pull from circular buffer */
        b = dev->in_buffer[read++];
        dev->in_read = (read + 1) & (IN_BUFFER_SIZE - 1);
        read = dev->in_read;
        
        dev->driver->process(&dev->api, b);
    }
}

static bool wait_until_can_write(void)
{
	uint32_t timeout = *P.counter + L.timeout;

    while ((*P.port_status & INPUT_FULL))
		if (*P.counter >= timeout)
			return SUCCESS;

    return ERROR;
}

static bool wait_until_can_read(void)
{
	uint32_t timeout = *P.counter + L.timeout;

    while (!(*P.port_status & OUTPUT_FULL))
		if (*P.counter >= timeout)
			return SUCCESS;

    return ERROR;
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
		L.in_data = *P.port_data;
		return SUCCESS;
	}

	return ERROR;
}

/* This one is meant to be called from IRQ. We know something arrived, so need to wait */
static uint8_t get_data_no_wait(void)
{
	return  *P.port_data;
}

static bool send_command_with_response(uint8_t cmd)
{
	return send_command(cmd) && get_data();
}

static bool send_data(enum ps2_target dev, uint8_t data)
{
	return dev
		? send_data1(data)
		: send_data2(data);
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
	return (send_data1(0xd4) && send_data1(data))
		? SUCCESS
		: ERROR;
}

static bool reset_device(struct ps2_device_t *dev)
{
	if (dev->status & STAT_PORT_ENABLED)
	{
		if (send_data(dev->id, DEVCMD_RESET))
		{
			dev->status |= STAT_CONNECTED;
			if (L.in_data == ACK)
			{
				dev->status |= STAT_OK;
				return SUCCESS;
			}
		}	
	}

	return ERROR;
}