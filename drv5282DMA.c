#include <rtems.h>

#include <mcf5282/mcf5282.h>
#include <rtems/rtems/cache.h>
#include <stdio.h>

#include <drv5282DMA.h>

/* Hmm __IPSBAR is defined in the linker script :-( */

#ifndef __IPSBAR
#define __IPSBAR ((volatile uint8_t *)0x40000000)
#endif
          
/* Seems to be missing */
#ifndef MCF5282_DMA_DCR_DSIZE_LINE
#define MCF5282_DMA_DCR_DSIZE_LINE                      (0x00030000)
#endif

#include <stdint.h>

#define DMA_CHNL 0
#define DMA_TIMR 0

extern void Timer_initialize();
extern uint32_t Read_timer();

/* I'm not a fan of those macros...
 * Note that e.g., MCF5282_EPDR_EPD(bit) doesn't protect 'bit' in the
 * expansion -- this tells me that whoever wrote those was maybe a novice...
 */

void
drv5282DMATimerSetup()
{
	/* route DMA timer to channel */
	MCF5282_SCM_DMAREQC  = MCF5282_SCM_DMAREQC_DMAC0( (MCF5282_SCM_DMAREQC_DMATIMER0 + DMA_TIMR) ) << ( DMA_CHNL << 2 );

#if 0 == DMA_TIMR
	/* configure input pin */
	MCF5282_GPIO_PTDPAR  = MCF5282_GPIO_PTDPAR_PTDPA1(3);
#else
#error "Please add code to assign DTINxx pin"
#endif

	/* setup DMA timer */
	/* capture event on raising edge (fifo full), use internal clock and bring out of reset */
	MCF5282_TIMER_DTMR((DMA_TIMR)) = MCF5282_TIMER_DTMR_CE_RISE | MCF5282_TIMER_DTMR_CLK_DIV1 | MCF5282_TIMER_DTMR_RST;

	/* enable DMA request */
	MCF5282_TIMER_DTXMR((DMA_TIMR)) = MCF5282_TIMER_DTXMR_DMAEN;

	/* make sure diagnostic timer is up */
	Timer_initialize();
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

uint32_t
drv5282ioDMA(uint8_t *to, uint8_t *from, uint32_t size, int ext, int poll, int tomem)
{
uint32_t now, then;
uint32_t linesz = rtems_cache_get_data_line_size();
uint32_t abeg,aend;
uint32_t linemsk = linesz - 1;

	/* INVALIDATE/FLUSH CACHE */

#ifdef CACHE_DEBUG
then = Read_timer();
#endif

	/* compute aligned beginning and end address */
	if ( tomem ) {
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
	} else {
		abeg = (((uint32_t)from))                   & ~linemsk;
		aend = (((uint32_t)from) + size + linemsk ) & ~linemsk;
		rtems_cache_flush_multiple_data_lines((void*)abeg, aend-abeg);
	}

#ifdef CACHE_DEBUG
now = Read_timer();

printf("Flushing %i bytes took %uus\n",aend-abeg,now-then);
#endif

	/* SETUP DMA */

	/* clear last condition */
	MCF5282_DMA_DSR((DMA_CHNL)) = MCF5282_DMA_DSR_DONE;

	/* addresses and byte count */
	MCF5282_DMA_SAR((DMA_CHNL)) = (uint32_t)from;
	MCF5282_DMA_DAR((DMA_CHNL)) = (uint32_t)to;

	MCF5282_DMA_BCR((DMA_CHNL)) = (MCF5282_SCM_MPARK & MCF5282_SCM_MPARK_BCR24BIT) ? size : size << 16;

	then = Read_timer();

	/* setup control register and START */

	/* Measured the following (1kB transfer):
	 * DSIZE_LINE: 160us
	 * DSIZE_LONG:  66us
	 * DSIZE_WORD: 130us
	 */
	MCF5282_DMA_DCR((DMA_CHNL)) = (tomem ? DMA_BITS_TO_MEM : DMA_BITS_FROM_MEM) | (ext ? MCF5282_DMA_DCR_EEXT : MCF5282_DMA_DCR_START);

	/* POLL FOR TERMINATION */

	if ( !ext && poll ) {
		while ( ! (MCF5282_DMA_DSR((DMA_CHNL)) & MCF5282_DMA_DSR_DONE) )
			/* Poll */;
	}

	now  = Read_timer();

	return ((now-then) & 0x00ffffff) | (MCF5282_DMA_DSR((DMA_CHNL))<<24) ;
}

void
drv5282DMADump()
{
	printf("DMA\n");
	printf("  SAR: 0x%08lx, DAR: 0x%08lx, BCR: 0x%08lx, DSR: 0x%02x\n",
		MCF5282_DMA_SAR((DMA_CHNL)),
		MCF5282_DMA_DAR((DMA_CHNL)),
		MCF5282_DMA_BCR((DMA_CHNL)),
		MCF5282_DMA_DSR((DMA_CHNL)));
	printf("DMATIMER\n");
	printf("  DTER: 0x%02x DTCR: 0x%08lx\n",
		MCF5282_TIMER_DTER((DMA_TIMR)),
		MCF5282_TIMER_DTCR((DMA_TIMR)));
}
