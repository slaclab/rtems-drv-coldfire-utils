#include <rtems.h>

#include <mcf5282/mcf5282.h>
#include <rtems/rtems/cache.h>
#include <stdio.h>

#include "coldfUtils.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

         
/* Seems to be missing */
#ifndef MCF5282_DMA_DCR_DSIZE_LINE
#define MCF5282_DMA_DCR_DSIZE_LINE                      (0x00030000)
#endif

#include <stdint.h>

#define CHANCHK(chan) do { if ((chan) < 0 || (chan) > 3) return -1; } while (0)

#define DMA_CHNL 0
#define DMA_TIMR 0

#if RTEMS_VERSION_ATLEAST(4,9,0)
extern void     benchmark_timer_initialize();
extern uint32_t benchmark_timer_read();
#define Timer_initialize benchmark_timer_initialize
#define Read_timer       benchmark_timer_read
#else
extern void Timer_initialize();
extern uint32_t Read_timer();
#endif

/* I'm not a fan of those macros...
 * Note that e.g., MCF5282_EPDR_EPD(bit) doesn't protect 'bit' in the
 * expansion -- this tells me that whoever wrote those was maybe a novice...
 */

int
coldfDMATimerSetup(int chan, int dtmr)
{
	CHANCHK(chan);
	CHANCHK(dtmr);

	/* route DMA timer to channel */
	MCF5282_SCM_DMAREQC  = MCF5282_SCM_DMAREQC_DMAC0( (MCF5282_SCM_DMAREQC_DMATIMER0 + dtmr) ) << ( chan << 2 );

	if ( 0 == dtmr ) {
		/* configure input pin */
		MCF5282_GPIO_PTDPAR |= MCF5282_GPIO_PTDPAR_PTDPA1(3);
	} else {
		fprintf(stderr,"Setup for DMA timers other than 0 not implemented yet\n");
		fprintf(stderr,"Please add code to assign DTINxx pin\n");
		return -1;
	}

	/* setup DMA timer */
	/* capture event on raising edge (fifo full), use internal clock and bring out of reset */
	MCF5282_TIMER_DTMR((dtmr)) = MCF5282_TIMER_DTMR_CE_RISE | MCF5282_TIMER_DTMR_CLK_DIV1 | MCF5282_TIMER_DTMR_RST;

	/* enable DMA request */
	MCF5282_TIMER_DTXMR((dtmr)) = MCF5282_TIMER_DTXMR_DMAEN;

	/* make sure diagnostic timer is up */
	Timer_initialize();

	return 0;
}

#define DMA_BITS_TO_MEM  0 \
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
		  | MCF5282_DMA_DCR_AT

#define DMA_BITS_FROM_MEM  0 \
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
		  | MCF5282_DMA_DCR_AT

/* compute pre-cooked mode */
uint32_t
coldfDMAMode(int ext, int poll, int tomem, int cache_coherency)
{
uint32_t rval;

	rval = tomem > 0 ? COLDF_DMA_MODE_TO_MEM : COLDF_DMA_MODE_FROM_MEM;

	if ( tomem > 1 )
		rval |= COLDF_DMA_MODE_SINC;
	else if ( tomem < 0 )
		rval |= COLDF_DMA_MODE_DINC;

	if ( cache_coherency ) {
		if ( cache_coherency < 0 )
			rval |= COLDF_DMA_MODE_CC_TO | COLDF_DMA_MODE_CC_FROM;
		else
			rval |= tomem > 0 ? COLDF_DMA_MODE_CC_TO : COLDF_DMA_MODE_CC_FROM;
	}

	if ( poll > 0 )
		rval |= COLDF_DMA_MODE_SYNC_POLL;
	else if ( poll < 0 )
		rval |= COLDF_DMA_MODE_SYNC_IRQ;

	if ( ext )
		rval |= COLDF_DMA_MODE_START_EXT;
	else
		rval |= COLDF_DMA_MODE_START_INT;
		
	return rval;
}

int
coldfDMAStart(int chan, uint8_t *to, uint8_t *from, uint32_t size, uint32_t mode)
{
#if defined(CACHE_DEBUG) || defined(TIMING_DEBUG)
uint32_t now, then;
#endif
uint32_t linesz  = rtems_cache_get_data_line_size();
uint32_t linemsk = linesz - 1;
uint32_t abeg,aend;
uint32_t dcr;
uint32_t poll    = 0;
int      rval    = 0;

	CHANCHK(chan);


	if ( (mode & COLDF_DMA_MODE_CC_MSK) ) {
		/* INVALIDATE/FLUSH CACHE */

#ifdef CACHE_DEBUG
then = Read_timer();
#endif

	/* compute aligned beginning and end address */
	if ( mode & COLDF_DMA_MODE_CC_TO ) {
		abeg = (((uint32_t)to) + linemsk ) & ~linemsk;
		aend = (((uint32_t)to) + size)     & ~linemsk;


		if ( (uint32_t)to & linemsk )
			/* flush unaligned head of buffer that may overlap other memory */
			rtems_cache_flush_multiple_data_lines((void*)to, abeg - (uint32_t)to);

		if ( ((uint32_t)to + size) & linemsk )
			/* flush unaligned end of buffer that may overlap other memory */
			rtems_cache_flush_multiple_data_lines((void*)aend, (uint32_t)to+size-aend);

		if ( aend > abeg )
			rtems_cache_invalidate_multiple_data_lines((void*)abeg, aend-abeg);
	}

	if ( mode & COLDF_DMA_MODE_CC_FROM ) {
		abeg = (((uint32_t)from))                   & ~linemsk;
		aend = (((uint32_t)from) + size + linemsk ) & ~linemsk;
		rtems_cache_flush_multiple_data_lines((void*)abeg, aend-abeg);
	}


#ifdef CACHE_DEBUG
now = Read_timer();

printf("Flushing %i bytes took %uus\n",aend-abeg,now-then);
#endif
	}

	/* SETUP DMA */

	/* clear last condition */
	MCF5282_DMA_DSR((chan)) = MCF5282_DMA_DSR_DONE;

	/* addresses and byte count */
	MCF5282_DMA_SAR((chan)) = (uint32_t)from;
	MCF5282_DMA_DAR((chan)) = (uint32_t)to;

	MCF5282_DMA_BCR((chan)) = (MCF5282_SCM_MPARK & MCF5282_SCM_MPARK_BCR24BIT) ? size : size << 16;

#ifdef TIMING_DEBUG
	then = Read_timer();
#endif

	/* setup control register and START */

	/* Measured the following (1kB transfer):
	 * DSIZE_LINE: 160us
	 * DSIZE_LONG:  66us
	 * DSIZE_WORD: 130us
	 */
 
	dcr  = ( mode & COLDF_DMA_MODE_MASK );

	if ( ! (COLDF_DMA_MODE_START_EXT & mode) ) {
		dcr |= MCF5282_DMA_DCR_START;
		poll = (COLDF_DMA_MODE_SYNC_POLL & mode);
	}

	MCF5282_DMA_DCR((chan)) = dcr;

	/* POLL FOR TERMINATION */

	if ( poll ) {
		while ( ! (MCF5282_DMA_DSR((chan)) & MCF5282_DMA_DSR_DONE) )
			/* Poll */;
		rval = (uint8_t)(MCF5282_DMA_DSR((chan)) & ~MCF5282_DMA_DSR_DONE);
	}

#ifdef TIMING_DEBUG
	now  = Read_timer();
	printf("DMA took %uus\n",now-then);
#endif

	return rval;
}

/*
 * Obtain interrupt vector for DMA channel 'chan';
 * -1 is returned if the channel number is out
 * of range
 */
int
coldfDMAIrqVector(int chan)
{
	if ( chan < 0 || chan > 3 )
		return -1;
	return 9+chan;
}

/*
 * Read status and reset 'DONE' if set.
 * Returns nonzero of an error had occurred
 * during the last DMA transfer.
 */
int
coldfDMAAck(int chan)
{
int rval = (uint8_t)MCF5282_DMA_DSR((chan));
	if ( MCF5282_DMA_DSR_DONE & rval ) {
		MCF5282_DMA_DSR((chan)) = MCF5282_DMA_DSR_DONE;
		rval &= ~MCF5282_DMA_DSR_DONE;
	}
	return rval;
}

int
coldfDMADump(int chan, int dtmr)
{
	CHANCHK(chan);
	CHANCHK(dtmr);

	printf("DMA (channel %i)\n",chan);
	printf("  SAR: 0x%08lx, DAR: 0x%08lx, BCR: 0x%08lx, DSR: 0x%02x\n",
		(unsigned long)MCF5282_DMA_SAR((chan)),
		(unsigned long)MCF5282_DMA_DAR((chan)),
		(unsigned long)MCF5282_DMA_BCR((chan)),
		(unsigned     )MCF5282_DMA_DSR((chan)));
	printf("DMATIMER\n");
	printf("  DTER: 0x%02x DTCR: 0x%08lx\n",
		(unsigned     )MCF5282_TIMER_DTER((dtmr)),
		(unsigned long)MCF5282_TIMER_DTCR((dtmr)));
	return 0;
}
