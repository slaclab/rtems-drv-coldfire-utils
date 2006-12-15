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

/* ============== EPORT ============== */

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

static inline int
coldfEportIntDisable_inl(int pin)
{
int level;
int rval;
        rtems_interrupt_disable(level);
		rval = (MCF5282_EPORT_EPIER & (MCF5282_EPORT_EPIER_EPIE(pin)));
        MCF5282_EPORT_EPIER &= ~(MCF5282_EPORT_EPIER_EPIE(pin));
        rtems_interrupt_enable(level);

		return rval;
}

/* extern versions [pin = 1..7] */

void
coldfEportIntEnable(int pin);

/* Disable interrupt and return previous status of mask */
int
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

/* =============== DMA =============== */

/* DMA Transfer : WARNING -- this is work in progress and the API might change */

int
coldfDMAStart(int chan, uint8_t *to, uint8_t *from, uint32_t size, int ext, int poll, int tomem, int cache_coherency);

/* Dump raw registers of DMA channel (0..3) and DMA timer (0..3) to stdout
 * RETURNS: 0 on success, -1 (invalid argument)
 */
int
coldfDMADump(int chan, int timer);

/* ==== CHIP SELECT CONFIGURATION ==== */

#define CSELECT_FLAG_OFF	((uint32_t)-1)	/* disable this CS            */
#define CSELECT_FLAG_BWEN	(1<<3)		/* enable burst writes        */
#define CSELECT_FLAG_BREN	(1<<4)          /* enable burst reads         */
#define CSELECT_FLAG_WP	     	(1<<0)          /* write protection           */
#define CSELECT_FLAG_AA	     	(1<<8)		/* internal cycle termination */
#define CSELECT_FLAG_WS(n)	(((n)&0xf)<<10)	/* wait states                */ 

int
coldfCsSetup(int cs, uint32_t flags);

/* ============= FEC PHY ============= */

/* Read/Write the FEC's MII registers  */

int
getMII(int phyNumber, int regNumber);

/*
 * Write MII register
 * Busy-waits, but transfer time should be short!
 */
void
setMII(int phyNumber, int regNumber, int value);

/* ============== QSPI =============== */
/* NOTE: All access to the hardware is protected by an internal
 *       mutex. Hence, it is safe for multiple threads sharing a
 *       common setup to use the driver 'concurrently'. The mutex
 *       serializes 'Write' and 'WriteRead' calls.
 *
 *       Note that 'Write' doesn't wait for the SPI transaction
 *       to finish while 'WriteRead' does.
 *
 *       After a 'Write' operation, the driver keeps the mutex
 *       until completion so that a second 'Write' blocks until
 *       the first one has completed.
 */

/* Initialize the driver */
int
coldfQspiInit();

/* Setup the QSPI interface
 *
 * 'sysclock': system clock in Hz (e.g., 64000000)
 * 'baudrate': QSPI clock rate in Hz
 *
 *   setup_ns: delay from CS activation to active clock edge (in ns)
 *             NOTE: value of zero selects default of 1/2 QSPI clock period
 *
 *    hold_ns: delay after CS deactivation until next transfer (in ns)
 *             NOTE: value of zero selects default of 17/sysclock
 *                   which is the lowest possible value.
 *
 *      flags: ORed options:
 *                 CLK_ACTVLOW: active clock level is the low level.
 *                 CLK_FALLING: data is latched on the active->inactive transition of
 *                              the clock and changed on the inactive->active transition.
 *                  CS_ACTVLOW: CS is driven high between transfers.
 */

/* Clock active low */
#define DRV5282_QSPI_SETUP_CLK_ACTVLOW	1
/* Data latched on falling edge (i.e., clock active->inactive transition) */
#define DRV5282_QSPI_SETUP_CLK_FALLING	2
#define DRV5282_QSPI_SETUP_CS_ACTVLOW	4

int
coldfQspiSetup(
	uint32_t sysclock,
	uint32_t baudrate,
	uint32_t setup_ns,
	uint32_t hold_ns,
	uint32_t flags);


/* Write 'n' bytes to TX RAM and issue command. This also
 * clocks data into the RX RAM (which is retrieved by separate
 * command).
 *
 * All bytes go the same (set) of devices (chip-selects).
 *
 * 'cs_mask' reflects the values driven on the CS lines
 * during the transaction. The 'inactive' state is programmed
 * by the 'Setup' routine.
 *
 * RETURNS: # bytes written or <0 on error
 *
 * NOTE:    The driver mutex is eventually released by the ISR
 *          when the transaction is complete but the 'Write'
 *          routine does not wait for that to happen.
 *          You can explicitely synchronize either by calling
 *          the 'Status' routine (which waits until it gets
 *          the mutex) or by using the 'WriteRead' entry point.
 */
int
coldfQspiWrite(uint8_t *buf, int n, uint16_t cs_mask);

/* Read and clear status
 *
 * RETURNS: 
 *    0   after successful transfer
 *    QIR status flags on transfer error
 *   -1   if status was clear (xfer completed flag not set)
 */
int
coldfQspiStatus();

/* Read n bytes from 'offset' out of RX RAM.
 * This routine does not perform any actual
 * transfer on the SPI but just reads data
 * (non-destructively) from the RX buffer.
 * 
 * RETURNS: # bytes read or <0 on error:
 */
int
coldfQspiRead(uint8_t *buf, unsigned offset, int n);

/* Write a buffer (see coldfQspiWrite), wait for the transfer
 * to complete and read received data (see coldfQspiRead).
 * All actions are performed 'atomically' (protected by the driver mutex).
 *
 * NOTES: 'rbuf' may be equal to 'tbuf' or it may be NULL (in which case
 *        received data is thrown away - this is effectively a synchronous
 *        write operation).
 *    
 * RETURNS: # of bytes transferred or < 0 on error
 *
 * 			                      -1: parameter error
 * 		    -QspiStatus return value: transmission error or abort
 */
int
coldfQspiWriteRead(uint8_t *tbuf, uint8_t *rbuf, int n, uint16_t cs_mask);

/* Shut down the driver  */
int
coldfQspiCleanup();


#ifdef __cplusplus
}
#endif


#endif
