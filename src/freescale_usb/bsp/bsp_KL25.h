
#include "common.h"
#include "Settings.h"

enum usb_clock
{
  MCGPLL0,
  MCGFLL,
  CLKIN
};


/* Select Clock source */
#define USB_CLOCK   MCGPLL0
//#define USB_CLOCK   MCGFLL
//#define USB_CLOCK   CLKIN



void vfnInitUSBClock (uint8 u8ClkOption);

