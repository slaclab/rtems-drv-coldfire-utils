/* $Id$ */

/* QSPI driver (supports byte-wide transfers only for now) */
#include <stdint.h>

/* T. Straumann, stealing some stuff from Dayle Kotturi, 12/2006 */

#include <coldfUtils.h>
#include <rtems/bspIo.h>
#include <rtems/error.h>
#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#undef IRQDEBUG

#define IRQFLAGS (MCF5282_QSPI_QIR_WCEF | MCF5282_QSPI_QIR_ABRT | MCF5282_QSPI_QIR_SPIF)
#define IRQMASK  ( \
	   MCF5282_QSPI_QIR_WCEFB \
	 | MCF5282_QSPI_QIR_ABRTB \
	 | MCF5282_QSPI_QIR_ABRTL \
	 | MCF5282_QSPI_QIR_WCEFE \
	 | MCF5282_QSPI_QIR_ABRTE \
	 | MCF5282_QSPI_QIR_SPIFE)

static rtems_id 		 mutex    = 0;
static rtems_id 		 syncS    = 0;
static volatile rtems_id waitfor  = 0;

#define QSPI_LOCK()		        \
	do {				        \
		rtems_status_code __sc; \
		if ( !mutex )	        \
			return -1;	        \
		__sc = rtems_semaphore_obtain(mutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);	\
		assert( RTEMS_SUCCESSFUL == __sc ); \
	} while (0)

#define QSPI_UNLOCK()	        \
	do {                        \
		rtems_status_code __sc; \
		__sc = rtems_semaphore_release(mutex); \
		assert( RTEMS_SUCCESSFUL == __sc );    \
	} while (0)


/* c.f. pp 10-12 */
#define QSPI_IRQ_VEC	(64 + 18)

/* write TX ram and send unprotected (unlocked w/o parm checking) */
void
send_unprot(uint8_t *buf, int n, uint16_t cs_mask)
{
register int i;
register uint16_t cmd;
uint16_t          dly;

	dly = MCF5282_QSPI_QDLYR;

	cs_mask = ((cs_mask<<8) & 0x0f00);

	/* Ignore the 'loong' delay of 8k/fsys implied by DTL == 0
	 * and use DT = 0 instead.
	 */
	if ( dly & MCF5282_QSPI_QDLYR_DTL(255) ) {
		cs_mask |= MCF5282_QSPI_QCR_DT;
	}

	/* Contrary to what the datasheet claims, QCR_DSCK == 0 and
	 * QDLYR_QCD == 0 do *not* have the same effect. Delaying
	 * by 1/2 the SPI clock is only in effect if QCR_DSCK == 0...
	 */
	if ( dly & MCF5282_QSPI_QDLYR_QCD(127) ) {
		cs_mask |= MCF5282_QSPI_QCR_DSCK;
	}

	cmd     = cs_mask | MCF5282_QSPI_QCR_CONT;

	/* write data */
	if ( buf ) {
		MCF5282_QSPI_QAR = 0;
		for ( i=0; i<n; i++) {
			MCF5282_QSPI_QDR = *buf++;
		}
	}

	n--;
	/* write ctl words */
	MCF5282_QSPI_QAR = 0x20;

	for ( i=0;  i<n; i++) {
		MCF5282_QSPI_QDR = cmd;
	}
	MCF5282_QSPI_QDR = cs_mask;

	/* Reset 'fifo' pointers */
	cmd  =  MCF5282_QSPI_QWR;
	cmd &= ~(MCF5282_QSPI_QWR_ENDQP(15) | MCF5282_QSPI_QWR_CPTQP(15) | MCF5282_QSPI_QWR_NEWQP(15));
	cmd |= MCF5282_QSPI_QWR_ENDQP(n);
	MCF5282_QSPI_QWR = cmd;

	/* Enable Interrupts */
	MCF5282_QSPI_QIR = IRQMASK | IRQFLAGS;

	/* Initiate transfer */
	MCF5282_QSPI_QDLYR = dly | MCF5282_QSPI_QDLYR_SPE;
}

int
coldfQspiWrite(uint8_t *buf, int n, uint16_t cs_mask)
{

	if ( n > 16 || n <= 0 )
		return -1;

	QSPI_LOCK();

	waitfor = mutex;

	send_unprot(buf, n, cs_mask);


#ifdef IRQDEBUG
	QSPI_UNLOCK();
#endif

	return n;
}

/* read RX ram unprotected (unlocked w/o parm checking) */
static void
read_unprot(uint8_t *buf, unsigned off, int n)
{
	/* Address RX RAM */
	MCF5282_QSPI_QAR = off + 0x10;
	while ( n-- > 0 ) {
		*buf++ = MCF5282_QSPI_QDR;
	}
}

int
coldfQspiRead(uint8_t *buf, unsigned offset, int n)
{
	if ( offset + n > 16 )
		return -1;

	QSPI_LOCK();

	read_unprot(buf, offset, n);

	QSPI_UNLOCK();

	return n;
}

int
coldfQspiWriteRead(uint8_t *tbuf, uint8_t *rbuf, int n, uint16_t cs_mask)
{
rtems_status_code sc;
int               rval;

	if ( ((unsigned)n) > 16 )
		return -1;

	QSPI_LOCK();

		waitfor = syncS;

		send_unprot(tbuf, n, cs_mask);

		if ( (sc = rtems_semaphore_obtain(syncS, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) ) {
			rtems_error(sc, "coldfQspiWriteRead: FATAL ERROR -- unable to obtainsync semaphore\n");
			QSPI_UNLOCK();
			return -1;
		}
		rval = MCF5282_QSPI_QIR & IRQFLAGS;

		if ( MCF5282_QSPI_QIR_SPIF != rval ) {
			QSPI_UNLOCK();
			return -rval;
		}

		if ( rbuf )
			read_unprot(rbuf, 0, n);

	QSPI_UNLOCK();

	return n;
}

int
coldfQspiSetup(uint32_t sysclock, uint32_t baud, uint32_t setup_ns, uint32_t hold_ns, uint32_t flags)
{
uint16_t	mode = MCF5282_QSPI_QMR_BITS_8 | MCF5282_QSPI_QMR_MSTR;

	if ( 0 == sysclock ) {
		if ( 0 == ( sysclock = bsp_get_CPU_clock_speed() ) ) {
			return -1;
		}
	}

	if ( setup_ns ) {
		/* Convert ns to clock ticks */
		setup_ns *= (sysclock/10000);
		setup_ns  = (setup_ns + 50000) / 100000;
		if ( setup_ns > 127 )
			setup_ns = 127;

		/* always use at least one tick */
		if ( 0 == setup_ns )
			setup_ns = 1;
	}

	if ( hold_ns ) {
		/* 'hold' clock is sysclock/32 */
		hold_ns  *= (sysclock/32/10000);
		hold_ns   = (hold_ns + 50000) / 100000;
		if ( hold_ns > 255 )
			hold_ns = 255;
		/* always use at least one tick */
		if ( 0 == hold_ns ) {
			hold_ns = 1;
		}
	}

	/* do lazy init */
	if ( !mutex && coldfQspiInit() ) {
		fprintf(stderr,"coldfQspiSetup: lazy init failed!!\n");
		return -1;
	}

	QSPI_LOCK();

	sysclock/=baud;
	mode |= MCF5282_QSPI_QMR_BAUD((sysclock/2));

	if ( flags & DRV5282_QSPI_SETUP_CLK_ACTVLOW )
		mode |= MCF5282_QSPI_QMR_CPOL;

	if ( flags & DRV5282_QSPI_SETUP_CLK_FALLING )
		mode |= MCF5282_QSPI_QMR_CPHA;

	MCF5282_QSPI_QMR = mode;

	MCF5282_QSPI_QDLYR = MCF5282_QSPI_QDLYR_QCD(setup_ns) | MCF5282_QSPI_QDLYR_DTL(hold_ns);

	mode = 0;
	if ( flags & DRV5282_QSPI_SETUP_CS_ACTVLOW )
		mode |= MCF5282_QSPI_QWR_CSIV;

	MCF5282_QSPI_QWR = mode;

	QSPI_UNLOCK();

	return 0;
}

static void
qspi_isr(void *usr_arg, rtems_vector_number v)
{
rtems_status_code sc;
	/* Disable IRQs but leave flags pending */
	MCF5282_QSPI_QIR = 0;
#ifdef IRQDEBUG
	printk("QSPI IRQ\n");
#else
	sc = rtems_semaphore_release( waitfor );
	if ( RTEMS_SUCCESSFUL != sc ) {
		printk("drv5282QSPI: FATAL ERROR in ISR: %i\n", sc);
		abort();
	}
#endif
}

/* Initialize driver */
int
coldfQspiInit()
{
uint8_t mode = 0;
rtems_status_code sc;

	/* use GPIO pins for QSPI:
	 * PQS[0]    => data out
	 * PQS[1]    => data in
	 * PQS[2]    => clock
	 * PQS[3..6] => CS[0:3]
	 */
	mode |= MCF5282_GPIO_PQSPAR_PQSPA0;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA1;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA2;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA3;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA4;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA5;
	mode |= MCF5282_GPIO_PQSPAR_PQSPA6;

	if ( !mutex ) {
		/* Do only once */
		MCF5282_GPIO_PQSPAR = mode;
	}

	if ( mutex ) {
		QSPI_LOCK();
	}

	/* Make sure the thing is stopped */
	MCF5282_QSPI_QDLYR &= ~MCF5282_QSPI_QDLYR_SPE;

	/* Make sure interrupts are cleared */
	MCF5282_QSPI_QIR = IRQFLAGS;

	if ( mutex ) {
		QSPI_UNLOCK();
		return 0;
	}

	/* Use a simple binary semaphore for the mutex:
	 *   o OK because we never nest.
	 *   o REQUIRED because we use it for synchronization
	 *     with termination of async write operation.
	 *     In this case, the mutex is released by the ISR
	 *     which would be illegal for a true mutex
	 *     (obtain/release by same task but ISR may be
	 *     executing from the 'context' of another task)
	 *   o REQUIRED because we use it for synchronization
	 *     at module cleanup time.
	 */
	sc = rtems_semaphore_create(
			rtems_build_name('s','p','i','m'), 
			1,
			RTEMS_SIMPLE_BINARY_SEMAPHORE,
			0,
			&mutex);
	if ( RTEMS_SUCCESSFUL != sc ) {
		rtems_error(sc, "coldfQspi: unable to create mutex\n");
		goto egress;
	}

	sc = rtems_semaphore_create(
			rtems_build_name('s','p','i','s'), 
			0,
			RTEMS_SIMPLE_BINARY_SEMAPHORE,
			0,
			&syncS);
	if ( RTEMS_SUCCESSFUL != sc ) {
		rtems_error(sc, "coldfQspi: unable to create semaphore\n");
		goto egress;
	}


	if ( BSP_installVME_isr(QSPI_IRQ_VEC, qspi_isr, 0) ) {
		fprintf(stderr, "coldfQspi: unable to install ISR\n");
		goto egress;
	}

	return 0;

egress:
	if ( mutex ) {
		rtems_semaphore_delete(mutex);
		mutex = 0;
	}
	if ( syncS ) {
		rtems_semaphore_delete(syncS);
		syncS = 0;
	}
	return -1;
}

/* Read and clear status
 *
 * RETURNS: 
 *    0   after successful transfer
 *    QIR status flags on transfer error
 *   -1   if status was clear (xfer completed flag not set)
 */
int
coldfQspiStatus()
{
int rval;
	/* Wait for ISR to finish */
	QSPI_LOCK();
	rval = MCF5282_QSPI_QIR & IRQFLAGS;
	/* Clear irqs */
	MCF5282_QSPI_QIR = IRQFLAGS;
	QSPI_UNLOCK();

	switch ( rval ) {
		case MCF5282_QSPI_QIR_SPIF:
			return 0;
		case 0:
			return -1;
		default:
			break;
	}
	return rval;
}

int
coldfQspiCleanup()
{
rtems_status_code sc;

	/* was maybe never used / initialized */
	if ( !mutex )
		return 0;

	QSPI_LOCK();

	/* Make sure the thing is stopped */
	MCF5282_QSPI_QDLYR &= ~MCF5282_QSPI_QDLYR_SPE;

	/* Make sure interrupts are cleared and disabled */
	MCF5282_QSPI_QIR = IRQFLAGS;

	sc = rtems_semaphore_delete(mutex);
	if ( RTEMS_SUCCESSFUL != sc ) {
		rtems_error(sc, "coldfQspi: unable do delete mutex\n");
		return -1;
	}

	sc = rtems_semaphore_delete(syncS);
	if ( RTEMS_SUCCESSFUL != sc ) {
		rtems_error(sc, "coldfQspi: unable do delete sync semaphore\n");
		return -1;
	}
	
	mutex = 0;

	if ( BSP_removeVME_isr( QSPI_IRQ_VEC, qspi_isr, 0 ) ) {
		fprintf(stderr, "coldfQspi: unable do remove ISR\n");
		return -1;
	}

	return 0;
}
