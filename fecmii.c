#include <rtems.h>
#include <bsp.h>
#include <mcf5282/mcf5282.h>

#ifndef __IPSBAR
#define __IPSBAR ((volatile uint8_t *)0x40000000)
#endif
 
int
getMII(int phyNumber, int regNumber)
{
    MCF5282_FEC_MMFR = (0x1 << 30)       |
                       (0x2 << 28)       |
                       (phyNumber << 23) |
                       (regNumber << 18) |
                       (0x2 << 16);
    while ((MCF5282_FEC_EIR & MCF5282_FEC_EIR_MII) == 0);
    MCF5282_FEC_EIR = MCF5282_FEC_EIR_MII;
    return MCF5282_FEC_MMFR & 0xFFFF;
}

/*
 * Write MII register
 * Busy-waits, but transfer time should be short!
 */
void
setMII(int phyNumber, int regNumber, int value)
{
    MCF5282_FEC_MMFR = (0x1 << 30)       |
                       (0x1 << 28)       |
                       (phyNumber << 23) |
                       (regNumber << 18) |
                       (0x2 << 16)       |
                       (value & 0xFFFF);
    while ((MCF5282_FEC_EIR & MCF5282_FEC_EIR_MII) == 0);
    MCF5282_FEC_EIR = MCF5282_FEC_EIR_MII;
}
