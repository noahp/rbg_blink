
//
// systick.c
// Systick provides ms time for use by other modules.
//

#include "systick.h"

uint32_t systick_ms = 0;
uint32_t systick_us = 0;
uint32_t systick_usCnt = 0; // us count in this ms period

// call every SYSTICK_USEC_BEAT microseconds
void systick_update(void)
{
    systick_us += SYSTICK_USEC_BEAT;
    systick_usCnt += SYSTICK_USEC_BEAT;
    if(systick_usCnt >= 1000){
        systick_ms++;
        systick_usCnt -= 1000;
    }
}

// return ms time
uint32_t systick_getMs(void)
{
    return systick_ms;
}

// return us time
uint32_t systick_getUs(void)
{
    return systick_us;
}
