
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

uint16_t si70xx_readHumidityTemperature(void)
{
    // 1. send start signal

    // 2. write slave id + w/r bit set to write

    // 3. wait for ack

    // 4. send read humidity, no hold (nak) mode

    // 5. wait for ack

    // 6a. send repeated start command to hold the bus

    // 6b. write slave id + w/r bit set to read

    // 7. wait for ack/nak- if ack done, else repeat above

    // 8. set read mode

    // 9. first byte

    // 10. second byte

    // 11. nak

    // 12. send stop

    return 0;
}
