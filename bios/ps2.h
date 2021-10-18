/* PS/2 system 
 * 
 * Authors:
 *	Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 #ifndef _PS2_H_
 #define _PS2_H_

 #include <stdint.h>
 #include <stdio.h>
 
 #define bool int
 #define SUCCESS -1
 #define ERROR 0;

 /* ps2_init() return codes */
#define ERR_CTRL_ERROR			 1<<0 /* Generic controller error (pretty bad) */
#define ERR_CTRL_SELFTEST_FAILED 1<<1 /* Controller self test failed (pretty bad) */
#define ERR_DEV1_SELFTEST_FAILED 1<<2 /* Device 1 self test failed */
#define ERR_DEV2_SELFTEST_FAILED 1<<3 /* Device 2 self test failed */
#define ERR_DEV1_RESET_FAILED	 1<<4 /* Device 1 reset failed */
#define ERR_DEV2_RESET_FAILED	 1<<5 /* Device 2 reset failed */
#define ERR_DEV1_NO_DRIVER_FOUND 1<<6 /* No driver found for device 1 */
#define ERR_DEV2_NO_DRIVER_FOUND 1<<7 /* No driver found for device 2*/
#define ERR_DEV1_DRIVER_ERROR	 1<<8 /* Device 1 driver errored */
#define ERR_DEV2_DRIVER_ERROR	 1<<9 /* Device 2 driver errored */

struct ps2_driver_api_t;

/* This is what a driver needs to implement */
struct ps2_driver_t
{
	/* Provided by the driver to the PS/2 system */
	/* Initialisation function, must be called before calling anything else */
	bool	(*init)(struct ps2_driver_api_t *this);
	/* Whether the driver can manage the given PS/2 device type (2-byte ID). */
	bool	(*can_drive)(const uint8_t *ps2_device_type);
	/* Process the data received during interrupts and fire up OS callbacks */
	void	(*process)(const struct ps2_driver_api_t *this, uint8_t byte);
};

/* This is the interface between the driver and the rest */
struct ps2_driver_api_t
{
	/* Provided by the PS/2 system to the driver */
	bool    (*send_data)(uint8_t b);
	uint8_t (*get_data)(void);
	void*   (*malloc)(size_t size);
	void    (*free)(void *address);

	/* OS callbacks that the driver will call as it does its job */
	void	(*on_key_down)(uint8_t scancode);
	void	(*on_key_up)(uint8_t scancode);

	struct ps2_driver_t *driver;

	/* State of the driver, driver does what it wants with this, nobody cares. */
	void *driver_data; 
};


/* This is the interface between the OS and the PS/2 system */
struct ps2_api_t
{
	/* --- INPUT parameters (provided by OS) --- */
	/* Ever increasing counter and its frequency in Hz */
	volatile uint32_t	*counter;
	uint16_t			counter_freq;
	/* PS/2 port addresses */
	volatile uint8_t *port_data;
	volatile uint8_t *port_status;
	uint8_t			 *port_cmd;
	/* Available drivers */
	uint8_t			    n_drivers;
	struct ps2_driver_t *drivers;
	/* OS functions that we can use */
	//void* (*malloc)(size_t size);
	//void  (*free)(void *address);

	/* Events OS -> PS/2 */
	/* These are called by the system interrupt handler when a PS/2 device interrupt
	 * occurs. The handler gets the data as quickly as possible, stores it then return control. */
	void (*on_device1_irq)(void);
	void (*on_device2_irq)(void);
	/* The OS instructs the devices to process the data they have received and act 
	 * accordingly by calling OS events */
	void (*on_do_events)(void);

	/* Events PS/2 -> OS. Ultimately, calling these are the purpose in life of drivers */
	void (*on_key_down)(uint8_t scancode);
	void (*on_key_up)(uint8_t scancode);
};


/* Initialises the PS/2 system (to be called by OS) */
uint16_t ps2_init(struct ps2_api_t *p);

#endif
