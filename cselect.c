
#include "coldfUtils.h"

#define CSELECT_FLAG_OFF	((uint32_t)-1)	/* disable this CS            */
#define CSELECT_FLAG_BWEN	(1<<3)		/* enable burst writes        */
#define CSELECT_FLAG_BREN	(1<<4)          /* enable burst reads         */
#define CSELECT_FLAG_WP	     	(1<<0)          /* write protection           */
#define CSELECT_FLAG_AA	     	(1<<8)		/* internal cycle termination */
#define CSELECT_FLAG_WS(n)	(((n)&0xf)<<10)	/* wait states                */ 

#define CSCR_MSK	(0x3df8)

int
coldfCsSetup(int cs, uint32_t flags)
{
	if ( cs < 0 || cs > 6 )
		return -1;

	if ( CSELECT_FLAG_OFF == flags ) {
		MCF5282_CS_CSMR(cs) &= ~(MCF5282_CS_CSMR_V);
		return 0;
	}

	/* No need to change CSAR, we use the BSP setup */

	/* open 1M window, allow supervisory and user data read access */
	MCF5282_CS_CSMR(cs) = MCF5282_CS_CSMR_BAM_1M
                           /* | MCF5282_CS_CSMR_WP */
                           /* | MCF5282_CS_CSMR_AM */  /* Allow DMA access */
                           | MCF5282_CS_CSMR_CI
                           | MCF5282_CS_CSMR_SC
                           /* | MCF5282_CS_CSMR_SD */
                           | MCF5282_CS_CSMR_UC
                           /* | MCF5282_CS_CSMR_UD */
       	                   ; 

	if ( CSELECT_FLAG_WP & flags )
		MCF5282_CS_CSMR(cs) |= MCF5282_CS_CSMR_WP;

	/* use 2 wait states, internal termination; burst reads end up requiring even more wait states
	 * [note that write-cycles would only need one!] :-(
	 */
	MCF5282_CS_CSCR(cs) = MCF5282_CS_CSCR_PS_16 | (flags & CSCR_MSK);

	MCF5282_CS_CSMR(cs) |= MCF5282_CS_CSMR_V;

	return 0;
}
