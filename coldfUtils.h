/* $Id$ */

/* Support for several arcturus / coldfire features */

#ifndef COLDFIRE_5282_UTILS_H
#define COLDFIRE_5282_UTILS_H

#include <rtems.h>
#include <mcf5282/mcf5282.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EPORT interrupts, pin configuration & the like
 * The 'pin' argument to all routines must be in the
 * range 1..7 (except for EpdrSet which accepts 0 also).
 */

/* enable/disable interrupts  */

static inline void
coldfEportIntDisable_inl(int pin)
{
int level;
        rtems_interrupt_disable(level);
        MCF5282_EPORT_EPIER |= (MCF5282_EPORT_EPIER_EPIE(pin));
        rtems_interrupt_enable(level);
}

static inline void
coldfEportIntEnable_inl(int pin)
{
int level;
        rtems_interrupt_disable(level);
        MCF5282_EPORT_EPIER &= ~(MCF5282_EPORT_EPIER_EPIE(pin));
        rtems_interrupt_enable(level);
}

/* extern versions */
void
coldfEportIntDisable(int pin);

void
coldfEportIntEnable(int pin);

/* Toggle a bit    */
int
coldfEportEpdrToggle(int bit);

/* Set or clear a bit. If 'bit' is zero then all bits
 * in 'value' are written to the EPORT.
 */
int
coldfEportEpdrSet(int bit, unsigned value);

/* EPORT setup */

#define EPORT_OUTPUT 	(-1)
#define EPORT_LEVEL  	(0)
#define EPORT_RAISING	(1)
#define EPORT_FALLING	(2)
#define EPORT_BOTH	(3)

int
coldfEportSetup(int pin, int config);

#ifdef __cplusplus
}
#endif

#endif
