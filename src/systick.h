
//
// systick.h
// Header file for systick. Systick provides ms time
// for use by other modules.
//

#include <stdint.h>

#if !defined(SYSTICK_H)
#define SYSTICK_H

// set period in useconds systick_update will be called
#define SYSTICK_USEC_BEAT 100

// call every SYSTICK_USEC_BEAT microseconds
void systick_update(void);

// return ms time
uint32_t systick_getMs(void);

// return us time
uint32_t systick_getUs(void);

#endif // SYSTICK_H
