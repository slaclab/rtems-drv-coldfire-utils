/* $Id$ */

/* Support for several arcturus / coldfire features */

#ifndef COLDFIRE_5282_UTILS_H
#define COLDFIRE_5282_UTILS_H

#include <rtems.h>
#include <mcf5282/mcf5282.h>
#include <stdint.h>

/* Hmm __IPSBAR is defined in the linker script :-( */
#ifndef __IPSBAR
#define __IPSBAR ((volatile uint8_t *)0x40000000)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* EPORT */

/* EPORT interrupts, pin configuration & the like */

/* EPORT setup */

#define EPORT_OUTPUT 	(-1)
#define EPORT_LEVEL  	(0)
#define EPORT_RAISING	(1)
#define EPORT_FALLING	(2)
#define EPORT_BOTH	(3)

int
coldfEportSetup(int pin, int config);


/* enable/disable interrupts; pin must be 1..7 [not checked] */

static inline void
coldfEportIntEnable_inl(int pin)
{
int level;
        rtems_interrupt_disable(level);
        MCF5282_EPORT_EPIER |= (MCF5282_EPORT_EPIER_EPIE(pin));
        rtems_interrupt_enable(level);
}

static inline void
coldfEportIntDisable_inl(int pin)
{
int level;
        rtems_interrupt_disable(level);
        MCF5282_EPORT_EPIER &= ~(MCF5282_EPORT_EPIER_EPIE(pin));
        rtems_interrupt_enable(level);
}

/* extern versions [pin = 1..7] */
void
coldfEportIntEnable(int pin);

void
coldfEportIntDisable(int pin);

/* Toggle a bit [pin = 1..7] */
int
coldfEportEpdrToggle(int bit);

/* Set or clear a bit. If 'bit' is zero then all bits
 * in 'value' are written to the EPORT.
 */
int
coldfEportEpdrSet(int bit, unsigned value);

/* Read current status of pin; if pin==0 all bits are read */
int
coldfEportEppdrGet(int pin);

/* Get flag register bit; if pin == 0 all bits are returned */
int
coldfEportFlagGet(int pin);

/* Clear flag register bit (returns pre-clear status); if pin == 0 all bits are cleared / returned */
int
coldfEportFlagClr(int pin);

/* DMA */

/* DMA Transfer : WARNING -- this is work in progress and the API might change */

int
coldfDMAStart(int chan, uint8_t *to, uint8_t *from, uint32_t size, int ext, int poll, int tomem, int cache_coherency);

/* Dump raw registers of DMA channel (0..3) and DMA timer (0..3) to stdout
 * RETURNS: 0 on success, -1 (invalid argument)
 */
int
coldfDMADump(int chan, int timer);

/* CHIP SELECT CONFIGURATION */

#define CSELECT_FLAG_OFF	((uint32_t)-1)	/* disable this CS            */
#define CSELECT_FLAG_BWEN	(1<<3)		/* enable burst writes        */
#define CSELECT_FLAG_BREN	(1<<4)          /* enable burst reads         */
#define CSELECT_FLAG_WP	     	(1<<0)          /* write protection           */
#define CSELECT_FLAG_AA	     	(1<<8)		/* internal cycle termination */
#define CSELECT_FLAG_WS(n)	(((n)&0xf)<<10)	/* wait states                */ 

int
coldfCsSetup(int cs, uint32_t flags);

#ifdef __cplusplus
}
#endif


#endif
