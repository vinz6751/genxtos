/* Initialisation of the SuperIO chip.
 * This file is copied from FoenixMCP
 */

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "foenix.h"
#include "a2560_debug.h"
#include "regutils.h"
#include "superio.h"

static void configure_zones(void);
static void UnReset_LPC(void);

/*
 * Initialize the SuperIO registers.
 */
void superio_init(void) {
  /* This initialization used to be done by the FPGA.
   * Other machines A2560K/C256FMX will have this change soon too. */

	UnReset_LPC();

	configure_zones();

  *GP10_REG = 0x01;
  *GP11_REG = 0x01;
  *GP12_REG = 0x01;
  *GP13_REG = 0x01;
  *GP14_REG = 0x05;
  *GP15_REG = 0x05;
  *GP16_REG = 0x05;
  *GP17_REG = 0x05;

  *GP20_REG = 0x00;
  *GP24_REG = 0x01;
  *GP25_REG = 0x05;
  *GP26_REG = 0x84;

  *GP30_REG = 0x01;
  *GP31_REG = 0x01;
  *GP32_REG = 0x01;
  *GP33_REG = 0x04; /* FAN1 GPIO Config */
  *GP34_REG = 0x01;
  *GP35_REG = 0x01;
  *GP36_REG = 0x01;
  *GP37_REG = 0x01;

  *GP42_REG = 0x01;
  *GP43_REG = 0x01;

  *GP50_REG = 0x05;
  *GP51_REG = 0x05;
  *GP52_REG = 0x05;
  *GP53_REG = 0x04;
  *GP54_REG = 0x05;
  *GP55_REG = 0x04;
  *GP56_REG = 0x05;
  *GP57_REG = 0x04;

  *GP60_REG = 0x84;
  *GP61_REG = 0x84;

  *GP1_REG = 0x00;
  *GP2_REG = 0x01;
  *GP3_REG = 0x00;
  *GP4_REG = 0x00;
  *GP5_REG = 0x00;
  *GP6_REG = 0x00;

  *LED1_REG = 0x01;
  *LED2_REG = 0x02;

  *FAN1_REG = 0xE0;       /* <= Value to change to Get the Fan running.
                           * See doc for more options, need to set $80 to get it started and use other bits to change the PWN... */
  *FAN_CTRL_REG = 0x01;
}


static void UnReset_LPC(void) {
  unsigned long i;
  /* TODO: use timers clocked at CPU speed to do the waits */
  
  R32(GAVIN_CTRL) |= GAVIN_CTRL_LPC_RESET; /* We start in normal mode */
  for (i = 0; i< 1000; i++) /* Not sure if this is necessary */
	  DONT_OPTIMIZE_OUT; 

  R32(GAVIN_CTRL) &= ~GAVIN_CTRL_LPC_RESET; /* Enter reset mode */
  for (i = 0; i< 400000; i++) /* Wait at least 0x325aa0 cycles at 33MHz */
	  DONT_OPTIMIZE_OUT;

  R32(GAVIN_CTRL) |= GAVIN_CTRL_LPC_RESET; /* Go back to normal mode */
  for (i = 0; i< 2000000; i++) /* Wait 0xfc0000 cycles at 33MHz */
	  DONT_OPTIMIZE_OUT;
}

/* Logical Device Numbers */
#define SUPERIO_LDN_FLOPPY   0
#define SUPERIO_LDN_LPT      3
#define SUPERIO_LDN_COM1     4
#define SUPERIO_LDN_COM2     5
#define SUPERIO_LDN_KEYBOARD 7
#define SUPERIO_LDN_GAMEPORT 9
#define SUPERIO_LDN_PWRMGMT  10
#define SUPERIO_LDN_MIDI     11

static void select_logical_device(uint8_t n) {
  R8(CONFIG_0x2E_REG) = 0x07;
  R8(CONFIG_0x2F_REG) = n;
}

static void set_base_address(uint16_t adr) {
  R8(CONFIG_0x2E_REG) = 0x60; /* High */
  R8(CONFIG_0x2F_REG) = (adr>>8);
  R8(CONFIG_0x2E_REG) = 0x61; /* Low*/
  R8(CONFIG_0x2F_REG) = (uint8_t)(adr&0xff);
}

static void set_irq(uint8_t n) {
  *CONFIG_0x2E_REG = 0x70;
  *CONFIG_0x2F_REG = n;
}

static void activate(void) {
  *CONFIG_0x2E_REG = 0x30;
  *CONFIG_0x2F_REG = 0x01;
}

static void configure_zones(void) {
	DONT_OPTIMIZE_OUT;

  /* Get into the Configuration Mode */
  *CONFIG_0x2E_REG = 0x55;  /* <- Cette ligne suffit à planter la machine */

  /* Setting Up Device 0 - Floppy */
  /* {8'h06,16'h03F0,8'h00}; */
  select_logical_device(SUPERIO_LDN_FLOPPY);
  set_base_address(0x3f0);
  set_irq(6);
  activate();

  /* Setting Up Device 3 - Parallel Port */
  /* {8'h07,16'h0378,8'h03}; */
  select_logical_device(SUPERIO_LDN_LPT);
  set_base_address(0x378);
  set_irq(7);
  /* Parallel Mode */
  *CONFIG_0x2E_REG = 0xF0;
  *CONFIG_0x2F_REG = 0x3A;
  activate();

  /* Setting Up Device 4 - Serial Port 1 */
  /* {8'h04,16'h03F8,8'h04}; */
  select_logical_device(SUPERIO_LDN_COM1);
  set_base_address(0x3f8);
  set_irq(4);
  activate();

  /* Setting Up Device 5 - Serial Port 2 */
  /* {8'h03,16'h02F8,8'h05}; */
  select_logical_device(SUPERIO_LDN_COM2);
  set_base_address(0x2f8);
  set_irq(3);
  activate();

  /* Setting Up Device 7 - Keyboard */
  /* {8'h01, 16'h0060,8'h07}; */
  select_logical_device(SUPERIO_LDN_KEYBOARD);
  set_base_address(0x060);
  set_irq(1);
  /* INT1 (mouse) */
  *CONFIG_0x2E_REG = 0x72;
  *CONFIG_0x2F_REG = 0x02;
  activate();

  /* Setting Up Device 9 - Game Port */
  /* {8'h00, 16'h0200,8'h09}; */
  select_logical_device(SUPERIO_LDN_GAMEPORT);
  set_base_address(0x200);
  set_irq(0);
  activate();

  /* Setting Up Device 10 - PME (Power Management) */
  /* {8'h00, 16'h0100,8'h0A}; */
  select_logical_device(SUPERIO_LDN_PWRMGMT);
  set_base_address(0x100);
  set_irq(0);
  activate();

  /* Setting Up Device 11 - MPU-401 (MIDI) */
  /* {8'h05,16'h0330,8'h0B}; */
  select_logical_device(SUPERIO_LDN_MIDI);
  set_base_address(0x330);
  set_irq(5);
  activate();

  /* Supplemental Settings */
  /* Power On Device */
  *CONFIG_0x2E_REG = 0x22;
  *CONFIG_0x2F_REG = 0xFF;

/* We are done with config. */
  *CONFIG_0x2E_REG = 0xAA;    /* We need to Get into the Config Mode with 0x55 */

  *GP60_REG = 0x84;           /* This is to replicate the FPGA behavior when it did the config. */
  *LED1_REG = 0x01;           /* This is to replace the FPGA behavior when it did the config in hardware. */
}

#endif
