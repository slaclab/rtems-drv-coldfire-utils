/* $Id$ */

/* Support for several arcturus / coldfire features */

/* EPORT interrupts */

/* enable/disable   */

#include "coldfUtils.h"

void
coldfEportIntDisable(int pin)
{
	coldfEportIntDisable_inl(pin);
}

void
coldfEportIntEnable(int pin)
{
	coldfEportIntEnable_inl(pin);
}

#define BITCHECK(bit) do { if ( (bit) < 1 || (bit) > 7 ) return -1; } while (0)

#define MSKCHECK(msk,bit) \
	do { \
		if ( (bit) < 0 || (bit) > 7 ) return -1; \
		msk = (bit) ? (1<<(bit)) : 0xfe; \
	} while (0)

int
coldfEportEpdrToggle(int bit)
{
uint32_t flags;

	BITCHECK(bit);

	rtems_interrupt_disable(flags);
		MCF5282_EPORT_EPDR ^= MCF5282_EPORT_EPDR_EPD((bit));
	rtems_interrupt_enable(flags);
	return 0;
}

int
coldfEportEpdrSet(int bit, unsigned value)
{
uint32_t flags;

	if ( bit < 0 || bit > 7 )
		return -1;

	if ( bit ) {
	rtems_interrupt_disable(flags);
		if ( value )
			MCF5282_EPORT_EPDR |=  MCF5282_EPORT_EPDR_EPD((bit));
		else
			MCF5282_EPORT_EPDR &= ~MCF5282_EPORT_EPDR_EPD((bit));
	rtems_interrupt_enable(flags);
	} else {
		MCF5282_EPORT_EPDR = value;
	}
	return 0;
}

	/* EPORT & GPIO setup */

#define EPORT_OUTPUT 	(-1)
#define EPORT_LEVEL  	(0)
#define EPORT_RAISING	(1)
#define EPORT_FALLING	(2)
#define EPORT_BOTH	(3)

int
coldfEportSetup(int pin, int config)
{
uint32_t flags;
unsigned bits = 0; /* keep compiler happy */

	BITCHECK(pin);

	switch ( config ) {
		default:            return -1;
		case EPORT_OUTPUT:                                             break;
		case EPORT_LEVEL:   bits = MCF5282_EPORT_EPPAR_EPPA1_LEVEL;    break;
		case EPORT_RAISING: bits = MCF5282_EPORT_EPPAR_EPPA1_RISING;   break;
		case EPORT_FALLING: bits = MCF5282_EPORT_EPPAR_EPPA1_FALLING;  break;
		case EPORT_BOTH:    bits = MCF5282_EPORT_EPPAR_EPPA1_BOTHEDGE; break;
	}

	rtems_interrupt_disable(flags);

	if ( config < 0 ) {
		/* output */
		MCF5282_EPORT_EPDDR |=  MCF5282_EPORT_EPDDR_EPDD(pin);
	} else {
		/* input  */
		MCF5282_EPORT_EPPAR &= ~(MCF5282_EPORT_EPPAR_EPPA1_BOTHEDGE<<(2*(pin-1)));
		MCF5282_EPORT_EPPAR |=  (bits<<(2*(pin-1)));

		MCF5282_EPORT_EPDDR &= ~MCF5282_EPORT_EPDDR_EPDD(pin);
	}

	rtems_interrupt_enable(flags);

	return 0;
}

int
coldfEportFlagGet(int pin)
{
unsigned char msk;

	MSKCHECK(msk,pin);

	return (MCF5282_EPORT_EPFR & msk);
}

/* Clear flag register bit (returns pre-clear status) */
int
coldfEportFlagClr(int pin)
{
int           rval;
uint32_t      flags;
unsigned char msk;

	MSKCHECK(msk, pin);

	rtems_interrupt_disable(flags);
		rval = MCF5282_EPORT_EPFR & msk;
		/* only clear if it was set otherwise we might miss an event */
		if ( rval )
			MCF5282_EPORT_EPFR = rval;
	rtems_interrupt_enable(flags);

	return rval;
}

int
coldfEportEppdrGet(int pin)
{
unsigned char msk;

	MSKCHECK(msk,pin);

	return (MCF5282_EPORT_EPPDR & msk);
}
