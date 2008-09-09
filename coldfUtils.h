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

/* =============== IRQ =============== */

/* A simple ISR that can be used for testing
 * (install/remove using BSP_installVME_isr(vector, isr, uarg)
 * It prints the user arg and vector number to
 * the console and disables the interrupt at the controller
 * (user BSP_enable_irq_at_pic() to re-enable).
 */
void
coldfTestISR(void *uarg, unsigned long vector);

/* =============== DMA =============== */

/*
 * DMA Transfer : WARNING -- this is work in progress and the API might change
 */

/*
 * The 'sync' flag can have the the following values
 */

/*
 * DMA 'mode' flags:
 */
#define COLDF_DMA_MODE_CC_TO		(1<<0)
	/* invalidate cache covering 'to' area    */
#define COLDF_DMA_MODE_CC_FROM		(1<<1)
	/* invalidate cache covering 'from' area  */
#define COLDF_DMA_MODE_CC_MSK		(3<<0)
	/* use external (HW) signal to start DMA; */
#define COLDF_DMA_MODE_START_EXT	(MCF5282_DMA_DCR_EEXT)
	/* start DMA immediately by software      */
#define COLDF_DMA_MODE_START_INT	(MCF5282_DMA_DCR_START)
	/* enable interrupt on completion or error (user still
	 * must hook an ISR).
	 */
#define COLDF_DMA_MODE_SYNC_IRQ		(MCF5282_DMA_DCR_INT)
	/* poll for DMA completion (mostly useful for timing/debugging) */
#define COLDF_DMA_MODE_SYNC_POLL	(1<<2)
	/* transfer from peripheral to memory (used to optimize port width etc)
	 */
#define COLDF_DMA_MODE_TOMEM		(1<<3)

#define COLDF_DMA_MODE_TO_MEM  (0 \
          /* | MCF5282_DMA_DCR_INT        */ \
          /* | MCF5282_DMA_DCR_EEXT       */ \
          /* | MCF5282_DMA_DCR_CS         */ \
             | MCF5282_DMA_DCR_AA            \
             | MCF5282_DMA_DCR_BWC_DMA       \
          /* | MCF5282_DMA_DCR_BWC_512    */ \
          /* | MCF5282_DMA_DCR_BWC_1024   */ \
          /* | MCF5282_DMA_DCR_BWC_2048   */ \
          /* | MCF5282_DMA_DCR_BWC_4096   */ \
          /* | MCF5282_DMA_DCR_BWC_8192   */ \
          /* | MCF5282_DMA_DCR_BWC_16384  */ \
          /* | MCF5282_DMA_DCR_BWC_32768  */ \
          /* | MCF5282_DMA_DCR_SINC       */ \
          /* | MCF5282_DMA_DCR_SSIZE_LONG */ \
          /* | MCF5282_DMA_DCR_SSIZE_BYTE */ \
             | MCF5282_DMA_DCR_SSIZE_WORD    \
          /* | MCF5282_DMA_DCR_SSIZE_LINE */ \
             | MCF5282_DMA_DCR_DINC          \
		  /* line is apparently slowest, LONG is best */ \
		  /* | MCF5282_DMA_DCR_DSIZE_LINE */ \
		     | MCF5282_DMA_DCR_DSIZE_LONG    \
		  /* | MCF5282_DMA_DCR_DSIZE_BYTE */ \
		  /* | MCF5282_DMA_DCR_DSIZE_WORD */ \
		  /* | MCF5282_DMA_DCR_START      */ \
		  | MCF5282_DMA_DCR_AT)

#define COLDF_DMA_MODE_FROM_MEM  (0 \
          /* | MCF5282_DMA_DCR_INT        */ \
          /* | MCF5282_DMA_DCR_EEXT       */ \
          /* | MCF5282_DMA_DCR_CS         */ \
          | MCF5282_DMA_DCR_AA               \
          | MCF5282_DMA_DCR_BWC_DMA          \
          /* | MCF5282_DMA_DCR_BWC_512    */ \
          /* | MCF5282_DMA_DCR_BWC_1024   */ \
          /* | MCF5282_DMA_DCR_BWC_2048   */ \
          /* | MCF5282_DMA_DCR_BWC_4096   */ \
          /* | MCF5282_DMA_DCR_BWC_8192   */ \
          /* | MCF5282_DMA_DCR_BWC_16384  */ \
          /* | MCF5282_DMA_DCR_BWC_32768  */ \
             | MCF5282_DMA_DCR_SINC          \
          /* | MCF5282_DMA_DCR_SSIZE_LONG */ \
          /* | MCF5282_DMA_DCR_SSIZE_BYTE */ \
          /* | MCF5282_DMA_DCR_SSIZE_WORD */ \
             | MCF5282_DMA_DCR_SSIZE_LINE    \
          /* | MCF5282_DMA_DCR_DINC       */ \
		  /* | MCF5282_DMA_DCR_DSIZE_LINE */ \
		     | MCF5282_DMA_DCR_DSIZE_LONG    \
		  /* | MCF5282_DMA_DCR_DSIZE_BYTE */ \
		  /* | MCF5282_DMA_DCR_DSIZE_WORD */ \
		  /* | MCF5282_DMA_DCR_START      */ \
		  | MCF5282_DMA_DCR_AT)


/* Increment source/destination pointer;
 * NOTE: if TOMEM is set then DINC is automatically set;
 *       if TOMEM is clear then SINC is automatically set.
 * This flag only affects SINC (TOMEM set) or DINC (TOMEM clear),
 * respectively.
 */
#define COLDF_DMA_MODE_SINC			(MCF5282_DMA_DCR_SINC)
#define COLDF_DMA_MODE_DINC			(MCF5282_DMA_DCR_DINC)

	/* strip all non-DCR flags -- INTERNAL USE ONLY */
#define COLDF_DMA_MODE_MASK         (~0xf)

/*
 * mode can be a combination of flags above but it can
 * also just be any combination of DMA/DCR flags.
 */

/*
 * compute pre-cooked mode word from simple parameters
 *
 * 'ext'   nonzero: use external/HW start condition, (SW start otherwise)
 * 'poll'  nonzero: poll for completion (poll>0) or enable irq (poll<0)
 * 'tomem'     > 0: use combination of flags useful for xfer from
 *                  periph. port to memory.
 * 'tomem'    <= 0: use combination of flags useful for xfer from
 *                  memory to periph. port.
 * 'tomem'     > 1: as tomem==1 but DO increment source address.
 * 'tomem'     < 0: as tomem==0 but DO increment dest address.
 *
 *                  NOTE: 'tomem == 1' does NOT increment the source address;
 *                        'tomem == 0' does NOT increment the dest address.
 *
 * 'cache_coherency'
 *               0: do not flush source nor invalidate destination from
 *                  cache.
 *              >0: IF (tomem>0) invalidate destination ELSE flush source
 *              <0: flush source and invalidate destination from cache.
 */
uint32_t
coldfDMAMode(int ext, int poll, int tomem, int cache_coherency);

int
coldfDMAStart(int chan, uint8_t *to, uint8_t *from, uint32_t size, uint32_t mode);
 
/*
 * Obtain interrupt vector for DMA channel 'chan';
 * -1 is returned if the channel number is out
 * of range
 */
int
coldfDMAIrqVector(int chan);

/*
 * Read status and reset 'DONE' if set.
 * Returns nonzero of an error had occurred
 * during the last DMA transfer.
 */
int
coldfDMAAck(int chan);

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
 *             NOTE: if 0 is passed then the routine will try to
 *             determine the clock speed automatically (returns nonzero
 *             if that fails).
 *
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
 *        'tbuf' may also be NULL which makes this effectively a synchronous
 *        read operation.
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

/* Copy contents of a file to flash.
 *
 * 'path'   Path of a file.
 * 'bank'   is unused (compatibility with other BSPs).
 * 'offset' should be 0 for a boot image.
 *
 * RETURNS: 0 on success, nonzero on error.
 */
int
BSP_flashWriteFile(int bank, uint32_t offset, char *path);

/* alternate entry point:
 * 'quiet': 0 all messages printed, ask for confirmation
 *          before erasing.
 * 'quiet': 1 all messages printed, no questions asked
 * 'quiet': 2 only error messages printed, no questions asked
 * 'quiet': 3 no messages printed, no questions asked
 */
int
BSP_flashWriteFile_1(int bank, uint32_t offset, char *path, int quiet);


#ifdef __cplusplus
}
#endif


#endif
