
#include "Settings.h"
#include <stdint.h>
#include "MKL26Z4.h"

enum usb_clock
{
  MCGPLL0,
  MCGFLL,
  CLKIN
};

/* Select Clock source */
//#define USB_CLOCK_MCGPLL0
#define USB_CLOCK_MCGFLL
//#define USB_CLOCK_CLKIN

void vfnInitUSBClock (void);
