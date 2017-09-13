/*
 * ubicom32vfb_lcd.c
 *	Ubicom32 LCD driver
 *
 * (C) Copyright 2009, Ubicom, Inc.
 *
 * This Ubicom32 library is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Ubicom32 library is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Ubicom32 Linux Kernel Port.  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "ubicom32vfb.h"

#define RC 0x02002000
#define RD 0x02003000
#define RI 0x02008000

static void ubicom32vfb_lcd_update_region_16(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h);
static void ubicom32vfb_lcd_update_region_8h(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h);
static void ubicom32vfb_lcd_update_region_8split(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h);
static void ubicom32vfb_lcd_update_16(const struct ubicom32vfb_lcd_panel *panel, u16 *data);
static void ubicom32vfb_lcd_update_8h(const struct ubicom32vfb_lcd_panel *panel, u16 *data);
static void ubicom32vfb_lcd_update_8split(const struct ubicom32vfb_lcd_panel *panel, u16 *data);
static u16 ubicom32vfb_lcd_read_data_16(void);
static u16 ubicom32vfb_lcd_read_data_8h(void);
static void ubicom32vfb_lcd_write_16(u16 data);
static void ubicom32vfb_lcd_write_8h(u16 data);
static void ubicom32vfb_lcd_write_8split(u16 data);

static const struct ubicom32vfb_lcd_bus ubicom32vfb_lcd_busses[] = {
	/*
	 * 16 is the default entry do not move!
	 */
	{
		name:			"16",
		data_port:		RI,
		data_mask:		0x0000FFFF,
		rs_port:		RD,
		rs_pin:			3,
		rd_port:		RD,
		rd_pin:			5,
		wr_port:		RD,
		wr_pin:			2,
		cs_port:		RD,
		cs_pin:			4,
		reset_port:		RD,
		reset_pin:		7,
		update_fn:		ubicom32vfb_lcd_update_16,
		update_region_fn:	ubicom32vfb_lcd_update_region_16,
		read_fn:		ubicom32vfb_lcd_read_data_16,
		write_fn:		ubicom32vfb_lcd_write_16,
		flags:			UBICOM32VFB_LCD_BUS_16BIT,
	},
	{
		name:			"8HIGH",
		data_port:		RI,
		data_mask:		0x0000FF00,
		rs_port:		RD,
		rs_pin:			3,
		rd_port:		RD,
		rd_pin:			5,
		wr_port:		RD,
		wr_pin:			2,
		cs_port:		RD,
		cs_pin:			4,
		reset_port:		RD,
		reset_pin:		7,
		update_fn:		ubicom32vfb_lcd_update_8h,
		update_region_fn:	ubicom32vfb_lcd_update_region_8h,
		read_fn:		ubicom32vfb_lcd_read_data_8h,
		write_fn:		ubicom32vfb_lcd_write_8h,
		flags:			UBICOM32VFB_LCD_BUS_8BIT,
	},
	{
		name:			"8LOW",
		data_port:		RI,
		data_mask:		0x000000FF,
		rs_port:		RD,
		rs_pin:			3,
		rd_port:		RD,
		rd_pin:			5,
		wr_port:		RD,
		wr_pin:			2,
		cs_port:		RD,
		cs_pin:			4,
		reset_port:		RD,
		reset_pin:		7,
		flags:			UBICOM32VFB_LCD_BUS_8BIT,
	},
	{
		name:			"8SPLIT",
		data_port:		RI,
		data_mask:		0x00000F0F,
		rs_port:		RI,
		rs_pin:			5,
		wr_port:		RI,
		wr_pin:			7,
		update_fn:		ubicom32vfb_lcd_update_8split,
		update_region_fn:	ubicom32vfb_lcd_update_region_8split,
		write_fn:		ubicom32vfb_lcd_write_8split,
		flags:			UBICOM32VFB_LCD_BUS_8BIT,
	},

	/*
	* 16 is default but with Cameo mapping
	*/
	{
		name:			"cameo16",
		data_port:		RC,
		data_mask:		0x0000FFFF,
		rs_port:		RC,
		rs_pin:			25,
		rd_port:		RD,
		rd_pin:			6,
		wr_port:		RD,
		wr_pin:			5,
		cs_port:		RD,
		cs_pin:			4,
		reset_port:		RC,
		reset_pin:		24,
		update_fn:		ubicom32vfb_lcd_update_16,
		update_region_fn:	ubicom32vfb_lcd_update_region_16,
		read_fn:		ubicom32vfb_lcd_read_data_16,
		write_fn:		ubicom32vfb_lcd_write_16,
		flags:			UBICOM32VFB_LCD_BUS_16BIT,
	},
};
static const struct ubicom32vfb_lcd_bus *bustype;

/*
 * Configure this for the hardware
 */
#define LCD_DATA_CTL_OFFSET	0x06		// offset of GPIO ctl register
#define LCD_DATA_OUT_OFFSET	0x0A		// offset of GPIO out register
#define LCD_DATA_IN_OFFSET	0x0E		// offset of GPIO in register
#define LCD_DATA_MASK_OFFSET	0x52		// offset of GPIO mask register

#define LCD_DATA8H_CTL_OFFSET	0x06		// offset of GPIO ctl register
#define LCD_DATA8H_OUT_OFFSET	0x0A		// offset of GPIO out register
#define LCD_DATA8H_IN_OFFSET	0x0E		// offset of GPIO in register
#define LCD_DATA8H_MASK_OFFSET	0x52		// offset of GPIO mask register

/*
 * Should not need to change anything below this line
 */
#define S(arg) #arg
#define D(arg) S(arg)

#define SYSTEM_FREQ	600000000
#define DELAY_HZ_TO_CYC(hz) (SYSTEM_FREQ / (hz))
#define DELAY_NS_TO_CYC(ns) (SYSTEM_FREQ / 1000000 * (ns) / 1000)
#define DELAY_US_TO_CYC(us) (SYSTEM_FREQ / 1000000 * (us))

#define ubicom32vfb_lcd_wait_ns(ns)	delay_cyc_const(DELAY_NS_TO_CYC(ns))
#define ubicom32vfb_lcd_wait_ms(ms)	usleep(ms * 1000) //delay_cyc_const(DELAY_US_TO_CYC(ms * 1000))

#define JMPT_PENALTY 3
#define JMPF_PENALTY 7

#define IO_GPIO_CTL 0x04
#define IO_GPIO_OUT 0x08
#define IO_GPIO_IN 0x0C
#define IO_GPIO_MASK 0x50

#define delay_cyc(dly) \
	{ \
	u32 __delay = (dly); \
	u32 __tmp; \
	asm volatile ( \
		"add.4		%[tmp], 0x14(%[timer_base]), %[delay]	\n\t" \
		"sub.4		#0, 0x14(%[timer_base]), %[tmp]		\n\t" \
		"jmpmi.f	.-4					\n\t" \
		: [tmp] "=d" (__tmp) \
		: [timer_base] "a" (0x01000100), [delay] "d" (__delay) \
		: "cc" \
		); \
	}

#define delay_cyc_const(dly) \
	{ \
		u32 __delayc = (dly); \
		if (__delayc <= 1) { \
			asm volatile ("nop"); \
		} else if (__delayc <= JMPT_PENALTY + 1) { \
			asm volatile ("jmpt.t   . + 4"); \
		} else if (__delayc <= JMPF_PENALTY + 1) { \
			asm volatile ("jmpt.f   . + 4"); \
		} else if (__delayc <= JMPT_PENALTY + 1 + JMPF_PENALTY + 1) { \
			asm volatile ("jmpt.f   . + 4"); \
			asm volatile ("jmpt.t   . + 4"); \
		} else if (__delayc <= JMPF_PENALTY + 1 + JMPF_PENALTY + 1) { \
			asm volatile ("jmpt.f   . + 4"); \
			asm volatile ("jmpt.f   . + 4"); \
		} else { \
			delay_cyc(__delayc); \
		} \
	}

#define ubicom32_set_port_pin_dir_out(port, pin) \
	asm volatile (	"or.4 "D(IO_GPIO_MASK)"(%[_port]), "D(IO_GPIO_MASK)"(%[_port]), %[mask]	\n\t" \
			"or.4 "D(IO_GPIO_CTL)"(%[_port]), "D(IO_GPIO_CTL)"(%[_port]), %[mask]	\n\t" \
			: : [_port] "a" (port), [mask] "d" (1 << pin) : "cc");

#define ubicom32_write_port_pin_high(port, pin) \
	asm volatile ( "or.4 "D(IO_GPIO_OUT)"(%0), "D(IO_GPIO_OUT)"(%0), %1\n\t" : : "a" (port), "d" (1 << pin) : "cc");

#define ubicom32_write_port_pin_low(port, pin) \
	asm volatile ( "and.4 "D(IO_GPIO_OUT)"(%0), "D(IO_GPIO_OUT)"(%0), %1\n\t" : : "a" (port), "d" (~(1 << pin)) : "cc");

/*
 * ubicom32vfb_lcd_write_8split
 *	Performs a write data/command cycle on the bus (assumes CS asserted, RD & WR set)
 */
static void ubicom32vfb_lcd_write_8split(u16 data)
{
	u32 data_mask = ~bustype->data_mask;
	u32 datah = data >> 8;
	asm volatile (
		"and.4	"D(IO_GPIO_OUT)"(%[port]), "D(IO_GPIO_OUT)"(%[port]), %[mask]	\n\t"
		"or.4	"D(IO_GPIO_OUT)"(%[port]), "D(IO_GPIO_OUT)"(%[port]), %[data]	\n\t"
		:
		: [port] "a" (bustype->data_port),
		  [mask] "d" (data_mask),
		  [data] "d" (((datah << 4) & 0x0F00) | (datah & 0x000F))
		: "cc"
	);

	ubicom32_write_port_pin_low(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);

	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(5);

	u32 datal = data & 0xFF;
	asm volatile (
		"and.4	"D(IO_GPIO_OUT)"(%[port]), "D(IO_GPIO_OUT)"(%[port]), %[mask]	\n\t"
		"or.4	"D(IO_GPIO_OUT)"(%[port]), "D(IO_GPIO_OUT)"(%[port]), %[data]	\n\t"
		:
		: [port] "a" (bustype->data_port),
		  [mask] "d" (data_mask),
		  [data] "d" (((datal << 4) & 0x0F00) | (datal & 0x000F))
		: "cc"
	);

	ubicom32_write_port_pin_low(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);

	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);
}

/*
 * ubicom32vfb_lcd_write_8h
 *	Performs a write data/command cycle on the bus (assumes CS asserted, RD & WR set)
 */
static void ubicom32vfb_lcd_write_8h(u16 data)
{
	asm volatile (
		"move.1 "D(LCD_DATA8H_CTL_OFFSET)"(%[port]), #0xFF	\n\t"
		"move.1 "D(LCD_DATA8H_OUT_OFFSET)"(%[port]), %[data]	\n\t"
		:
		: [port] "a" (bustype->data_port), 
		  [data] "d" (data >> 8)
	);

	ubicom32_write_port_pin_low(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);

	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(5);

	asm volatile ("move.1 "D(LCD_DATA8H_OUT_OFFSET)"(%[port]), %[data]" :: [port] "a" (bustype->data_port), [data] "d" (data));

	ubicom32_write_port_pin_low(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);

	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(5);

	asm volatile ("move.1 "D(LCD_DATA8H_CTL_OFFSET)"(%[port]), #0\n\t" :: [port] "a" (bustype->data_port));

	ubicom32vfb_lcd_wait_ns(45);
}

/*
 * ubicom32vfb_lcd_write_16
 *	Performs a write data/command cycle on the bus (assumes CS asserted, RD & WR set)
 */
static void ubicom32vfb_lcd_write_16(u16 data)
{
	asm volatile (
		"move.2 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #-1		\n\t"
		"move.2 "D(LCD_DATA_OUT_OFFSET)"(%[port]), %[data]	\n\t" 
		:
		: [port] "a" (bustype->data_port), 
		  [data] "d" (data)
	);

	ubicom32_write_port_pin_low(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(50);

	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);

	ubicom32vfb_lcd_wait_ns(5);

	asm volatile (
		"move.2 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #0\n\t"
		:
		: [port] "a" (bustype->data_port)
	);

	ubicom32vfb_lcd_wait_ns(45);
}

/*
 * ubicom32vfb_lcd_write_command
 *	Performs a write command cycle on the bus (assumes CS asserted, RD & WR set)
 */
static void ubicom32vfb_lcd_write_command(u16 command)
{
	ubicom32_write_port_pin_low(bustype->rs_port, bustype->rs_pin);
	bustype->write_fn(command);
}

/*
 * ubicom32vfb_lcd_write_data
 *	Performs a write data cycle on the bus (assumes CS asserted, RD & WR set)
 */
static void ubicom32vfb_lcd_write_data(u16 data)
{
	ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);
	bustype->write_fn(data);
}

/*
 * ubicom32vfb_lcd_read_data_8h
 *	Performs a read cycle on the bus (assumes CS asserted, RD & WR set)
 */
static u16 ubicom32vfb_lcd_read_data_8h(void)
{
	asm volatile ( "move.1 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));

	ubicom32_write_port_pin_low(bustype->rs_port, bustype->rs_pin);

	ubicom32vfb_lcd_wait_ns(300);

	u16 data;
	asm volatile ("move.1 %[data], "D(LCD_DATA_IN_OFFSET)"(%[port])" : [data] "=d" (data) : [port] "a" (bustype->data_port));

	ubicom32vfb_lcd_wait_ns(200);

	ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);

	ubicom32vfb_lcd_wait_ns(200);

	ubicom32_write_port_pin_low(bustype->rd_port, bustype->rd_pin);

	ubicom32vfb_lcd_wait_ns(300);

	asm volatile ("shmrg.1 %[data], "D(LCD_DATA_IN_OFFSET)"(%[port]), %[data]" : [data] "+d" (data) : [port] "a" (bustype->data_port));

	ubicom32vfb_lcd_wait_ns(200);

	ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);

	ubicom32vfb_lcd_wait_ns(500);

	return data;
}

/*
 * ubicom32vfb_lcd_read_data_16
 *	Performs a read cycle on the bus (assumes CS asserted, RD & WR set)
 */
static u16 ubicom32vfb_lcd_read_data_16(void)
{
	asm volatile ( "move.2 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));

	ubicom32_write_port_pin_low(bustype->rd_port, bustype->rd_pin);

	ubicom32vfb_lcd_wait_ns(300);

	u16 data;
	asm volatile ("move.2 %[data], "D(LCD_DATA_IN_OFFSET)"(%[port])" : [data] "=d" (data) : [port] "a" (bustype->data_port));

	ubicom32vfb_lcd_wait_ns(200);

	ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);

	ubicom32vfb_lcd_wait_ns(500);

	return data;
}

/*
 * ubicom32vfb_lcd_execute
 *	Executes a script for performing operations on the LCD (assumes CS set)
 */
static void ubicom32vfb_lcd_execute(const struct ubicom32vfb_lcd_panel *panel,
				    const struct ubicom32vfb_lcd_step *script)
{
	while (1) {
		switch (script->op) {
		case LCD_STEP_CMD:
			ubicom32vfb_lcd_write_command(script->cmd);
			break;

		case LCD_STEP_DATA:
			ubicom32vfb_lcd_write_data(script->data);
			break;

		case LCD_STEP_CMD_DATA:
			ubicom32vfb_lcd_write_command(script->cmd);
			ubicom32vfb_lcd_write_data(script->data);
			break;

		case LCD_STEP_SLEEP:
			ubicom32vfb_lcd_wait_ms(script->data);
			break;

		case LCD_STEP_DONE:
			return;
		}
		script++;
	}
}

/*
 * ubicom32vfb_lcd_get_bustype
 *	find bustype from bus name
 */
static const struct ubicom32vfb_lcd_bus *ubicom32vfb_lcd_get_bustype(const char *busname)
{
	int i;
	for (i = 0; i < sizeof(ubicom32vfb_lcd_busses) / sizeof(struct ubicom32vfb_lcd_bus); i++) {
		if (strcasecmp(ubicom32vfb_lcd_busses[i].name, busname + 3) == 0) {
			return &ubicom32vfb_lcd_busses[i];
		}
	}
	return NULL;
}

/*
 * ubicom32vfb_lcd_init
 *	Initializes the lcd panel.
 */
int ubicom32vfb_lcd_init(const struct ubicom32vfb_lcd_panel **panel, int w, int h)
{
	char *bus_mode = getenv("UBICOM32VFB_BUS_MODE");
	char *panel_name = getenv("UBICOM32VFB_PANEL");

	if (!bus_mode) {
		/*
		 * Default to 16 bit
		 */
		bustype = &ubicom32vfb_lcd_busses[0];
		D_DEBUG_AT(ubicom32vfb, "Default bus selected.\n");
	} else {
		char *p = strdup(bus_mode);
		if (!p) {
			D_DEBUG_AT(ubicom32vfb, "Unable to dup env string\n");
			return -ENOMEM;
		}

		char *q = strtok(p, ",");
		while (q) {
			if (strncasecmp(q, "BUS", 3) == 0) {
				bustype = ubicom32vfb_lcd_get_bustype(q);
			}
			q = strtok(NULL, ",");
		}

		free(p);
	}
	
	if (!bustype) {
		D_DEBUG_AT(ubicom32vfb, "No bus type found!\n");
		return -ENODEV;
	}

	D_ASSERT(bustype);
	D_ASSERT(bustype->write_fn);
	D_ASSERT(bustype->update_fn);
	D_ASSERT(bustype->data_port);
	D_ASSERT(bustype->data_mask);
	D_ASSERT(bustype->rs_port);
	D_ASSERT(bustype->wr_port);

	D_DEBUG_AT(ubicom32vfb, "Bus type selected: %s\n", bustype->name);

	if (bustype->reset_port) {
		ubicom32_write_port_pin_low(bustype->reset_port, bustype->reset_pin);
		ubicom32_set_port_pin_dir_out(bustype->reset_port, bustype->reset_pin);
	}

	/*
	 * Set data input and gpio if we can read, otherwise, always output
	 */
	asm volatile ( 
		/*
		 * set the pins to GPIO
		 */
		"	or.4	"D(IO_GPIO_MASK)"(%[data_port]), "D(IO_GPIO_MASK)"(%[data_port]), %[mask]	\n\t" 
		"	and.4	#0, %[rd_port], %[rd_port]							\n\t"
		"	jmpne.t	1f										\n\t"

		/*
		 * always output
		 */
		"	or.4	"D(IO_GPIO_CTL)"(%[data_port]), "D(IO_GPIO_CTL)"(%[data_port]), %[mask]		\n\t" 
		"	jmpt.t	2f										\n\t"

		/*
		 * start off input
		 */
		"1:												\n\t"
		"	not.4	%[mask], %[mask]								\n\t"
		"	and.4	"D(IO_GPIO_CTL)"(%[data_port]), "D(IO_GPIO_CTL)"(%[data_port]), %[mask]		\n\t" 

		"2:												\n\t"
		: 
		: [data_port] "a" (bustype->data_port),
		  [mask] "d" (bustype->data_mask),
		  [rd_port] "d" (bustype->rd_port)
		: "cc"
	);

	ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);
	ubicom32_set_port_pin_dir_out(bustype->rs_port, bustype->rs_pin);
	ubicom32_write_port_pin_high(bustype->wr_port, bustype->wr_pin);
	ubicom32_set_port_pin_dir_out(bustype->wr_port, bustype->wr_pin);

	if (bustype->rd_port) {
		ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);
		ubicom32_set_port_pin_dir_out(bustype->rd_port, bustype->rd_pin);
	}
	if (bustype->cs_port) {
		ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
		ubicom32_set_port_pin_dir_out(bustype->cs_port, bustype->cs_pin);
	}

	if (bustype->reset_port) {
		ubicom32vfb_lcd_wait_ms(20);

		ubicom32_write_port_pin_high(bustype->reset_port, bustype->reset_pin);

		ubicom32vfb_lcd_wait_ms(20);
	}

	if (bustype->cs_port) {
		ubicom32_write_port_pin_low(bustype->cs_port, bustype->cs_pin);
	}

	/*
	 * We will try to figure out what kind of panel we have if we were not told.
	 */
	u16 id = 0;
	if (!*panel) {
		const struct ubicom32vfb_lcd_panel **p = ubicom32vfb_lcd_panels;
		while (*p) {
			/*
			 * 8 bit flag must be both set or both unset
			 */
			if ((!((*p)->flags & UBICOM32VFB_LCD_PANEL_FLAG_8BIT) && (bustype->flags & UBICOM32VFB_LCD_BUS_8BIT)) || 
			    (((*p)->flags & UBICOM32VFB_LCD_PANEL_FLAG_8BIT) && !(bustype->flags & UBICOM32VFB_LCD_BUS_8BIT))) {
				p++;
				continue;
			}

			/*
			 * If we were told a panel name then find it
			 */
			if (panel_name) {
				if (strcasecmp(panel_name, (*p)->desc) == 0) {
					int matchxres = 0;
					int matchyres = 0;

					if (!w || ((*p)->xres == w)) {
						matchxres = 1;
					}
					if (!h || ((*p)->yres == h)) {
						matchyres = 1;
					}
					if (matchxres && matchyres) {
						break;
					}
				}
			} else {
				/*
				 * Probe the bus, unless the bus doesn't support reads.
				 */
				if (bustype->read_fn) {
					id = bustype->read_fn();

					D_DEBUG_AT(ubicom32vfb, "Looking for id=%04x %dx%d, panel: %dx%d %s\n", id, w, h, (*p)->xres, (*p)->yres, (*p)->desc);
					if ((*p)->id && ((*p)->id == id)) {
						int matchxres = 0;
						int matchyres = 0;
						if (!w || ((*p)->xres == w)) {
							matchxres = 1;
						}
						if (!h || ((*p)->yres == h)) {
							matchyres = 1;
						}
						if (matchxres && matchyres) {
							break;
						}
					}
				} else {
					D_DEBUG_AT(ubicom32vfb, "Must specify UBICOM32VFB_PANEL env variable, this bus does not support reads.\n");
					return -ENODEV;
				}
			}
			p++;
		}

		if (!*p) {
			if (panel_name) {
				D_DEBUG_AT(ubicom32vfb, "Could not find compatible panel, desc='%s', %dx%d\n", panel_name, w, h);
			} else {
				D_DEBUG_AT(ubicom32vfb, "Could not find compatible panel, id=%x, %dx%d\n", id, w, h);
			}
			return -ENODEV;
		}
		*panel = *p;
	} else {
		int matchxres = 0;
		int matchyres = 0;

		if (!w || ((*panel)->xres == w)) {
			matchxres = 1;
		}
		if (!h || ((*panel)->yres == h)) {
			matchyres = 1;
		}

		id = 0;
		if (bustype->read_fn) {
			id = bustype->read_fn();
		}

		/*
		 * Make sure panel ID matches if we were supplied a panel type
		 */
		if ((id && ((*panel)->id && ((*panel)->id != id))) || !matchxres || !matchyres) {
			ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
			D_DEBUG_AT(ubicom32vfb, "Expected panel id: %x got: %x\n", (*panel)->id, id);

			return -ENODEV;
		}
	}
	ubicom32vfb_lcd_execute(*panel, (*panel)->init_seq);

	if (bustype->cs_port) {
		ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
	}

	D_DEBUG_AT(ubicom32vfb, "Initialized panel id=%x: %s\n", id, (*panel)->desc);
	
	return 0;
}

/*
 * ubicom32vfb_lcd_goto_internal
 *	Places the gram pointer at a specific X, Y address
 */
static void ubicom32vfb_lcd_goto_internal(const struct ubicom32vfb_lcd_panel *panel, int x, int y)
{
	ubicom32vfb_lcd_write_command(panel->horz_reg);
	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_FLIPX) {
		ubicom32vfb_lcd_write_data(panel->xofs - x);
	} else {
		ubicom32vfb_lcd_write_data(x + panel->xofs);
	}

	ubicom32vfb_lcd_write_command(panel->vert_reg);
	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_FLIPY) {
		ubicom32vfb_lcd_write_data(panel->yofs - y);
	} else {
		ubicom32vfb_lcd_write_data(y + panel->yofs);
	}

	ubicom32vfb_lcd_write_command(panel->gram_reg);
}

/*
 * ubicom32vfb_lcd_goto
 *	Swaps the coordinates based on the rotation flag and then executes the goto instruction.
 */
static void ubicom32vfb_lcd_goto(const struct ubicom32vfb_lcd_panel *panel, int x, int y)
{
	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_ROTATED) {
		ubicom32vfb_lcd_goto_internal(panel, y, x);
		return;
	}
	ubicom32vfb_lcd_goto_internal(panel, x, y);
}

/*
 * ubicom32vfb_lcd_update_8h
 *	Update the entire LCD screen
 */
static void ubicom32vfb_lcd_update_8h(const struct ubicom32vfb_lcd_panel *panel, u16 *data)
{
	int pixels = panel->xres * panel->yres;
	int i;

	ubicom32_write_port_pin_low(bustype->cs_port, bustype->cs_pin);
	ubicom32vfb_lcd_goto(panel, 0, 0);

	ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);
	ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);

	/*
	 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
	 */
	asm volatile (
		"	move.1		"D(LCD_DATA8H_CTL_OFFSET)"(%[data_port]), #0xFF				\n\t"
		"	lsr.4		%[i], %[pixels], #3							\n\t"
		"	or.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"

		/*
		 * no need to check, we know we have some number of pixels > 16 (or this is one
		 * really small display!)
		 */
		"1:												\n\t"
		"	.rept 16										\n\t"
		"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	btst		%[delay], #0								\n\t"
		"	jmpeq.t		10f									\n\t"
		"	btst		%[delay], #1								\n\t"
		"	jmpeq.t		10f									\n\t"
		"	jmpt.f		+4									\n\t"
		"10:												\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	btst		%[delay], #16								\n\t"
		"	jmpeq.t		11f									\n\t"
		"	btst		%[delay], #17								\n\t"
		"	jmpeq.t		11f									\n\t"
		"	jmpt.f		+4									\n\t"
		"11:												\n\t"
		"	.endr											\n\t"

		"	add.4		%[i], #-1, %[i]								\n\t"
		"	jmpne.t		1b									\n\t"

		"	and.4		%[i], #0x0f, %[pixels]							\n\t"
		"	jmpeq.t		3f									\n\t"

		"2:												\n\t"
		"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"

		"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"

		"	add.4		%[i], #-1, %[i]								\n\t"
		"	jmpne.t		2b									\n\t"

		"3:												\n\t"

		: [data] "+&a" (data),
		  [i] "=&d" (i)
		: [wr_port] "a" (bustype->wr_port),
		  [data_port] "a" (bustype->data_port),
		  [pixels] "d" (pixels),
		  [delay] "d" (panel->delay),
		  [wr_mask] "d" (1 << bustype->wr_pin)
		: "cc"
	);

	ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
	asm volatile ( "move.1 "D(LCD_DATA8H_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));
}

/*
 * ubicom32vfb_lcd_update_region_8h
 *	Update the a region of the LCD screen
 */
static void ubicom32vfb_lcd_update_region_8h(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h)
{
	int i;

	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_NO_PARTIAL) {
		ubicom32vfb_lcd_update(panel, data);
		return;
	}

	/*
	 * Get the data to the correct pixel
	 */
	data += (y * (panel->stride)) + x;

	ubicom32_write_port_pin_low(bustype->cs_port, bustype->cs_pin);

	for (i = 0; i < h; i++) {
		int pixels = w;
		int i;
		u16 *p = data;
		ubicom32vfb_lcd_goto(panel, x, y);

		ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);

		/*
		 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
		 */
		asm volatile (
			"	move.1		"D(LCD_DATA8H_CTL_OFFSET)"(%[data_port]), #0xFF				\n\t"
			"	or.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	lsr.4		%[i], %[pixels], #2							\n\t"
			"	jmpeq.f		2f									\n\t"

			"1:												\n\t"
			"	.rept 4 * 2										\n\t"
			"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	btst		%[delay], #0								\n\t"
			"	jmpeq.t		10f									\n\t"
			"	btst		%[delay], #1								\n\t"
			"	jmpeq.t		10f									\n\t"
			"	jmpt.f		+4									\n\t"
			"10:												\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	btst		%[delay], #16								\n\t"
			"	jmpeq.t		11f									\n\t"
			"	btst		%[delay], #17								\n\t"
			"	jmpeq.t		11f									\n\t"
			"	jmpt.f		+4									\n\t"
			"11:												\n\t"
			"	.endr											\n\t"

			"	add.4		%[i], #-1, %[i]								\n\t"
			"	jmpne.t		1b									\n\t"

			"2:												\n\t"
			"	and.4		%[i], #3, %[pixels]							\n\t"
			"	jmpeq.f		4f									\n\t"

			"3:												\n\t"
			"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"

			"	move.1		"D(LCD_DATA8H_OUT_OFFSET)"(%[data_port]), (%[data])1++			\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"

			"	add.4		%[i], #-1, %[i]								\n\t"
			"	jmpne.t		3b									\n\t"
			"4:												\n\t"

			: [data] "+a" (p),
			  [i] "=&d" (i)
			: [wr_port] "a" (bustype->wr_port),
			  [wr_mask] "d" (1 << bustype->wr_pin),
			  [data_port] "a" (bustype->data_port),
			  [pixels] "d" (pixels),
			  [delay] "d" (panel->delay)
			: "cc"
		);

		data += panel->xres;
		y++;
	}

	ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
	asm volatile ( "move.1 "D(LCD_DATA8H_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));
}

/*
 * ubicom32vfb_lcd_update_16
 *	Update the entire LCD screen
 */
static void ubicom32vfb_lcd_update_16(const struct ubicom32vfb_lcd_panel *panel, u16 *data)
{
	int pixels = panel->xres * panel->yres;
	int i;

	ubicom32_write_port_pin_low(bustype->cs_port, bustype->cs_pin);
	ubicom32vfb_lcd_goto(panel, 0, 0);

	ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);
	ubicom32_write_port_pin_high(bustype->rd_port, bustype->rd_pin);


	/*
	 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
	 */
	asm volatile (
		"	move.2		"D(LCD_DATA_CTL_OFFSET)"(%[data_port]), #-1				\n\t"
		"	or.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	lsr.4		%[i], %[pixels], #4							\n\t"

		/*
		 * no need to check, we know we have some number of pixels > 16 (or this is one
		 * really small display!)
		 */
		"1:												\n\t"
		"	.rept 16										\n\t"
		"	move.2		"D(LCD_DATA_OUT_OFFSET)"(%[data_port]), (%[data])2++			\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	btst		%[delay], #0								\n\t"
		"	jmpeq.t		10f									\n\t"
		"	btst		%[delay], #1								\n\t"
		"	jmpeq.t		10f									\n\t"
		"	jmpt.f		+4									\n\t"
		"10:												\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	btst		%[delay], #16								\n\t"
		"	jmpeq.t		11f									\n\t"
		"	btst		%[delay], #17								\n\t"
		"	jmpeq.t		11f									\n\t"
		"	jmpt.f		+4									\n\t"
		"11:												\n\t"
		"	.endr											\n\t"

		"	add.4		%[i], #-1, %[i]								\n\t"
		"	jmpne.t		1b									\n\t"

		"	and.4		%[i], #0x0f, %[pixels]							\n\t"
		"	jmpeq.t		3f									\n\t"

		"2:												\n\t"
		"	move.2		"D(LCD_DATA_OUT_OFFSET)"(%[data_port]), (%[data])2++			\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	jmpt.f		+4									\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
		"	jmpt.f		+4									\n\t"

		"	add.4		%[i], #-1, %[i]								\n\t"
		"	jmpne.t		2b									\n\t"

		"3:												\n\t"

		: [data] "+&a" (data),
		  [i] "=&d" (i)
		: [wr_port] "a" (bustype->wr_port),
		  [wr_mask] "d" (1 << bustype->wr_pin),
		  [data_port] "a" (bustype->data_port),
		  [pixels] "d" (pixels),
		  [delay] "d" (panel->delay)
		: "cc"
	);

	ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
	asm volatile ( "move.2 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));
}

/*
 * ubicom32vfb_lcd_update_region_16
 *	Update the a region of the LCD screen
 */
static void ubicom32vfb_lcd_update_region_16(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h)
{
	int i;

	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_NO_PARTIAL) {
		ubicom32vfb_lcd_update(panel, data);
		return;
	}

	/*
	 * Get the data to the correct pixel
	 */
	data += (y * (panel->stride)) + x;

	ubicom32_write_port_pin_low(bustype->cs_port, bustype->cs_pin);

	for (i = 0; i < h; i++) {
		int pixels = w;
		int i;
		u16 *p = data;
		ubicom32vfb_lcd_goto(panel, x, y);

		ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);

		/*
		 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
		 */
		asm volatile (
			"	move.2		"D(LCD_DATA_CTL_OFFSET)"(%[data_port]), #-1				\n\t"
			"	lsr.4		%[i], %[pixels], #2							\n\t"
			"	jmpeq.f		2f									\n\t"

			"1:												\n\t"
			"	.rept 4											\n\t"
			"	move.2		"D(LCD_DATA_OUT_OFFSET)"(%[data_port]), (%[data])2++			\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	btst		%[delay], #0								\n\t"
			"	jmpeq.t		10f									\n\t"
			"	btst		%[delay], #1								\n\t"
			"	jmpeq.t		10f									\n\t"
			"	jmpt.f		+4									\n\t"
			"10:												\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	btst		%[delay], #16								\n\t"
			"	jmpeq.t		11f									\n\t"
			"	btst		%[delay], #17								\n\t"
			"	jmpeq.t		11f									\n\t"
			"	jmpt.f		+4									\n\t"
			"11:												\n\t"
			"	.endr											\n\t"

			"	add.4		%[i], #-1, %[i]								\n\t"
			"	jmpne.t		1b									\n\t"

			"2:												\n\t"
			"	and.4		%[i], #3, %[pixels]							\n\t"
			"	jmpeq.f		4f									\n\t"

			"3:												\n\t"
			"	move.2		"D(LCD_DATA_OUT_OFFSET)"(%[data_port]), (%[data])2++			\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	jmpt.f		+4									\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]	\n\t"
			"	jmpt.f		+4									\n\t"

			"	add.4		%[i], #-1, %[i]								\n\t"
			"	jmpne.t		3b									\n\t"
			"4:												\n\t"

			: [data] "+a" (p),
			  [i] "=&d" (i)
			: [wr_port] "a" (bustype->wr_port),
			  [wr_mask] "d" (1 << bustype->wr_pin),
			  [data_port] "a" (bustype->data_port), 
			  [pixels] "d" (pixels),
			  [delay] "d" (panel->delay)
			: "cc"
		);

		data += panel->xres;
		y++;
	}

	ubicom32_write_port_pin_high(bustype->cs_port, bustype->cs_pin);
	asm volatile ( "move.2 "D(LCD_DATA_CTL_OFFSET)"(%[port]), #0\n\t" : : [port] "a" (bustype->data_port));
}

/*
 * ubicom32vfb_lcd_update_8split
 *	Update the entire LCD screen
 */
static void ubicom32vfb_lcd_update_8split(const struct ubicom32vfb_lcd_panel *panel, u16 *data)
{
	int pixels = panel->xres * panel->yres;
	int i;
	int tmp;
	int tmp2;

	ubicom32vfb_lcd_goto(panel, 0, 0);

	ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);

	/*
	 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
	 */
	asm volatile (
		"	lsr.4		%[i], %[pixels], #3								\n\t"
		"	or.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high

		/*
		 * no need to check, we know we have some number of pixels > 16 (or this is one
		 * really small display!)
		 */
		"1:													\n\t"
		"	.rept 16											\n\t"
		"	move.1		%[tmp2], (%[data])1++								\n\t"
		"	lsl.1		%[tmp], %[tmp2], #4								\n\t"
		"	or.1		%[tmp], %[tmp2], %[tmp]								\n\t"
		"	and.4		%[tmp], %[tmp], %[data_mask]							\n\t"
		"	and.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[n_data_mask]	\n\t"
		"	or.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[tmp]		\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set low
		"	btst		%[delay], #0									\n\t"
		"	jmpeq.t		10f										\n\t"
		"	btst		%[delay], #1									\n\t"
		"	jmpeq.t		10f										\n\t"
		"	jmpt.f		+4										\n\t"
		"10:													\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high
		"	btst		%[delay], #16									\n\t"
		"	jmpeq.t		11f										\n\t"
		"	btst		%[delay], #17									\n\t"
		"	jmpeq.t		11f										\n\t"
		"	jmpt.f		+4										\n\t"
		"11:													\n\t"
		"	.endr												\n\t"

		"	add.4		%[i], #-1, %[i]									\n\t"
		"	jmpne.t		1b										\n\t"

		"	and.4		%[i], #0x0f, %[pixels]								\n\t"
		"	jmpeq.t		3f										\n\t"

		"2:													\n\t"
		"	.rept 2												\n\t"
		"	move.1		%[tmp2], (%[data])1++								\n\t"
		"	lsl.1		%[tmp], %[tmp2], #4								\n\t"
		"	or.1		%[tmp], %[tmp2], %[tmp]								\n\t"
		"	and.4		%[tmp], %[tmp], %[data_mask]							\n\t"
		"	and.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[n_data_mask]	\n\t"
		"	or.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[tmp]		\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set low
		"	jmpt.f		+4										\n\t"
		"	jmpt.f		+4										\n\t"
		"	jmpt.f		+4										\n\t"
		"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high
		"	jmpt.f		+4										\n\t"
		"	.endr												\n\t"

		"	add.4		%[i], #-1, %[i]									\n\t"
		"	jmpne.t		2b										\n\t"

		"3:													\n\t"

		: [data] "+&a" (data),
		  [i] "=&d" (i),
		  [tmp] "=&d" (tmp),
		  [tmp2] "=&d" (tmp2)
		: [wr_port] "a" (bustype->wr_port),
		  [wr_mask] "d" (1 << bustype->wr_pin),
		  [data_port] "a" (bustype->data_port),
		  [data_mask] "d" (bustype->data_mask),
		  [n_data_mask] "d" (~bustype->data_mask),
		  [pixels] "d" (pixels),
		  [delay] "d" (panel->delay)
		: "cc"
	);
}

/*
 * ubicom32vfb_lcd_update_region_8split
 *	Update the a region of the LCD screen
 */
static void ubicom32vfb_lcd_update_region_8split(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h)
{
	int i;

	if (panel->flags & UBICOM32VFB_LCD_PANEL_FLAG_NO_PARTIAL) {
		ubicom32vfb_lcd_update(panel, data);
		return;
	}

	/*
	 * Get the data to the correct pixel
	 */
	data += (y * (panel->stride)) + x;

	for (i = 0; i < h; i++) {
		int pixels = w;
		int tmp;
		int tmp2;
		int j;
		u16 *p = data;
		ubicom32vfb_lcd_goto(panel, x, y);

		ubicom32_write_port_pin_high(bustype->rs_port, bustype->rs_pin);

		/*
		 * The number of jmpt.f's should be sufficient for any cpu frequency up to around 600Mhz
		 */
		asm volatile (
			"	or.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high
			"	lsr.4		%[j], %[pixels], #2								\n\t"
			"	jmpeq.f		2f										\n\t"

			"1:													\n\t"
			"	.rept 4 * 2											\n\t"
			"	move.1		%[tmp2], (%[data])1++								\n\t"
			"	lsl.1		%[tmp], %[tmp2], #4								\n\t"
			"	or.1		%[tmp], %[tmp2], %[tmp]								\n\t"
			"	and.4		%[tmp], %[tmp], %[data_mask]							\n\t"
			"	and.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[n_data_mask]	\n\t"
			"	or.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[tmp]		\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set low
			"	btst		%[delay], #0									\n\t"
			"	jmpeq.t		10f										\n\t"
			"	btst		%[delay], #1									\n\t"
			"	jmpeq.t		10f										\n\t"
			"	jmpt.f		+4										\n\t"
			"10:													\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high
			"	btst		%[delay], #16									\n\t"
			"	jmpeq.t		11f										\n\t"
			"	btst		%[delay], #17									\n\t"
			"	jmpeq.t		11f										\n\t"
			"	jmpt.f		+4										\n\t"
			"11:													\n\t"
			"	.endr												\n\t"

			"	add.4		%[j], #-1, %[j]									\n\t"
			"	jmpne.t		1b										\n\t"

			"2:													\n\t"
			"	and.4		%[j], #3, %[pixels]								\n\t"
			"	jmpeq.f		4f										\n\t"

			"3:													\n\t"
			"	.rept 2												\n\t"
			"	move.1		%[tmp2], (%[data])1++								\n\t"
			"	lsl.1		%[tmp], %[tmp2], #4								\n\t"
			"	or.1		%[tmp], %[tmp2], %[tmp]								\n\t"
			"	and.4		%[tmp], %[tmp], %[data_mask]							\n\t"
			"	and.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[n_data_mask]	\n\t"
			"	or.4		"D(IO_GPIO_OUT)"(%[data_port]), "D(IO_GPIO_OUT)"(%[data_port]), %[tmp]		\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set low
			"	jmpt.f		+4										\n\t"
			"	jmpt.f		+4										\n\t"
			"	jmpt.f		+4										\n\t"
			"	xor.4		"D(IO_GPIO_OUT)"(%[wr_port]), "D(IO_GPIO_OUT)"(%[wr_port]), %[wr_mask]		\n\t"	// set high
			"	jmpt.f		+4										\n\t"
			"	.endr												\n\t"

			"	add.4		%[j], #-1, %[j]									\n\t"
			"	jmpne.t		3b										\n\t"
			"4:													\n\t"

			: [data] "+a" (p),
			  [j] "=&d" (j),
			  [tmp] "=&d" (tmp),
			  [tmp2] "=&d" (tmp2)
			: [wr_port] "a" (bustype->wr_port),
			  [wr_mask] "d" (1 << bustype->wr_pin),
			  [data_port] "a" (bustype->data_port), 
			  [data_mask] "d" (bustype->data_mask),
			  [n_data_mask] "d" (~bustype->data_mask),
			  [pixels] "d" (pixels),
			  [delay] "d" (panel->delay)
			: "cc"
		);

		data += panel->xres;
		y++;
	}
}

/*
 * ubicom32vfb_lcd_update
 *	Update the entire LCD screen
 */
void ubicom32vfb_lcd_update(const struct ubicom32vfb_lcd_panel *panel, u16 *data)
{
	bustype->update_fn(panel, data);
}

/*
 * ubicom32vfb_lcd_update_region
 *	Update the a region of the LCD screen
 */
void ubicom32vfb_lcd_update_region(const struct ubicom32vfb_lcd_panel *panel, u16 *data, int x, int y, int w, int h)
{
	if (bustype->update_region_fn) {
		bustype->update_region_fn(panel, data, x, y, w, h);
	} else {
		bustype->update_fn(panel, data);
	}
}

