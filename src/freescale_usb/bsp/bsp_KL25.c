#include "bsp_KL25.h"

void vfnInitUSBClock (void)
{
    #if defined(USB_CLOCK_MCGPLL0)
    SIM_SOPT2 |= SIM_SOPT2_PLLFLLSEL_MASK     /** PLL reference */
            |  SIM_SOPT2_USBSRC_MASK;       /** USB clock source MCGPLLCLK/2 or MCGFLLCLK  */
    #elif defined(USB_CLOCK_MCGFLL)
    /** Assumes FLL is configured to generate a 48MHz core clock */

    SIM_SOPT2 &= ~SIM_SOPT2_PLLFLLSEL_MASK;       /** FLL reference */
    SIM_SOPT2 |=  SIM_SOPT2_USBSRC_MASK;          /** USB clock source MCGPLLCLK/2 or MCGFLLCLK  */
    #elif defined(USB_CLOCK_CLKIN)
    SIM_SOPT2 &= (uint32_t)(~SIM_SOPT2_USBSRC_MASK);    /** PTA5 selected as USBFS CLK source */
    FLAG_SET(SIM_SCGC5_PORTA_SHIFT,SIM_SCGC5);
    PORTA_PCR5=(0|PORT_PCR_MUX(2));                   /** Enable PTA5 as CLK input */
    #endif

    SIM_SCGC4|=(SIM_SCGC4_USBOTG_MASK);             // USB Clock Gating
}
