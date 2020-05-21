#ifndef __TIME_H__
#define __TIME_H__

#include <stm32f1xx.h>

// uint8_t CoreClock = 8;

enum rcc_error {
    NO_ERROR = 0,
    START_HSE = 1,
    START_PLL = 2
};

int clock_init();

void ms_delay(uint32_t ms);

void mks_delay(uint32_t mks);

#endif //__TIME_H__