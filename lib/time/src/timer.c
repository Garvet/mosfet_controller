#include "time.h"

uint8_t CoreClock = 8;

int clock_init() {
    __IO int start_counter;
    RCC->CR |= RCC_CR_HSEON; // Запускаем генератор HSE
    // Ожидаем пока генератор не запустится 
    for(start_counter = 0;(RCC->CR & RCC_CR_HSERDY) == 0; ++start_counter) {
        // Если превышено время ожидания
        if (start_counter > 0x1000) {
            RCC->CR &= ~RCC_CR_HSEON; // Останавливаем генератор HSE
            return START_HSE;
        }
    }
    
    RCC->CFGR |= RCC_CFGR_PLLMULL9; // Устанавливаем множитель генератора = 9 (8*9=72МГц)
    RCC->CFGR |= RCC_CFGR_PLLSRC;
    
    RCC->CR |= RCC_CR_PLLON; // Запускаем генератор PLL
    // Ожидаем пока генератор не запустится 
    for(start_counter = 0;(RCC->CR & RCC_CR_PLLRDY) == 0; ++start_counter) {
        // Если превышено время ожидания
        if (start_counter > 0x1000) {
            RCC->CR &= ~RCC_CR_PLLON; // Останавливаем генератор PLL
            RCC->CR &= ~RCC_CR_HSEON; // Останавливаем генератор HSE
            return START_PLL;
        }
    }

    // Устанавливаем 2 цикла ожидания для Flash
    // 0 при (0, 24] МГц, 1 при (24, 48], 1 при (48, 72]
    // так как частота ядра у нас будет 48 МГц < SYSCLK <= 72 МГц 
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    // Обнуляем значение регистра HPRE (делитель шины AHB)
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    // Обнуляем значение регистра PPRE1 (делитель шины APB1)  
    // Присваиваем для регистра PPRE1 значение деления = 2 (APB1 макс. 36 МГц)
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    // Обнуляем значение регистра PPRE2 (делитель шины APB2)
    RCC->CFGR &= ~RCC_CFGR_PPRE2;

    // Переводим систему на генератор PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    // Ждем, пока переключимся
    while((RCC->CFGR & RCC_CFGR_SWS_PLL) == 0) {}
    CoreClock = 36;
    // Останавливаем генератор HSI, он больше не используется
    RCC->CR &= ~RCC_CR_HSION;

    return NO_ERROR;
}

void ms_delay(uint32_t ms) {
    
    
    ms *= CoreClock/4;
    while (ms-- > 0) {
    //    mks_delay(500);
        volatile int x=500;
        while (x-- > 0)
            __asm("nop");
    }
}

void mks_delay(uint32_t mks) {
    mks *= CoreClock/8;
    while (mks-- > 0) {
        __asm("nop");
    }
}