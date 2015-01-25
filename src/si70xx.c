
//
// si70xx.c
//
//  si70xx driver.
//
#include "si70xx.h"

#define SI70XX_SLAVE_ADDRESS (0x40)

#define SI70XX_CMD__MEASURE_RH_HOLD         (0xE5)
#define SI70XX_CMD__MEASURE_RH_NOHOLD       (0xF5)
#define SI70XX_CMD__MEASURE_TEMP_HOLD       (0xE3)
#define SI70XX_CMD__MEASURE_TEMP_NOHOLD     (0xF3)
#define SI70XX_CMD__READ_TEMP_LAST_RH_MEAS  (0xE0)

uint8_t si70xx_readHumidityTemperature(void)
{
    return 0;
}