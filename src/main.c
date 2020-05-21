#include "main.h"
#include <stm32f1xx.h>
#include <time.h>
#include <LoRa.h>
#include <UART.h>

#define LED 13

const long BAND = 4330E5;

#define LEN1 12
#define LEN2 13
#define LEN31 14
#define LEN32 5
const uint8_t str1[LEN1] = "Mosfet - ON";
const uint8_t str2[LEN2] = "Mosfet - OFF";
const uint8_t str31[LEN31] = "Mosfet ON at ";
const uint8_t str32[LEN32] = " min";

#define TIME_SEC 10
uint8_t timer_on = 0;
uint16_t time = 0;

int main() {
    // clock_init();
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV16;

    struct LoRaClass lora;
    LoRaClass_init(&lora);
    begin(&lora, BAND, 1);
    ms_delay(15);
    setTxPower(&lora, 14, RF_PACONFIG_PASELECT_PABOOST);

    // Инициализация диода
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    GPIOC->CRH |= GPIO_CRH_MODE13_1;
    GPIOC->ODR &= ~GPIO_ODR_ODR13;
    // GPIOC->ODR |= GPIO_ODR_ODR13;

    // Инициализация мосфета
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRL &= ~(GPIO_CRL_MODE1 | GPIO_CRL_CNF1);
    GPIOA->CRL |= GPIO_CRL_MODE1_1;
    GPIOA->ODR &= ~GPIO_ODR_ODR1;
    // GPIOA->ODR |= GPIO_ODR_ODR1;

    for (int j = 0; j < 3; ++j) {
        GPIOC->ODR &= ~(1<<LED);
        ms_delay(500);
        GPIOC->ODR |= (1<<LED);
        ms_delay(500);
    }
    
            beginPacket(&lora, 0);
                GPIOC->ODR |= (1<<LED);
                //Mosfet OFF
                GPIOA->ODR &= ~GPIO_ODR_ODR1;
                timer_on = 0;
                time = 0;
                for (uint8_t i = 0; i < LEN2-1; ++i)
                    write(&lora, str2[i]);
            endPacket(&lora, 0);


    ms_delay(5);
    receive(&lora, 0);
    ms_delay(20);
    uint16_t packet_size = 0;
    uint8_t symbol;
    uint8_t symbol_m[2];
    uint8_t i = 0;

    while(1) {
        packet_size = parsePacket(&lora, 0);
        
        if (packet_size == 2) { 
            for (int j = 0; j < 1; ++j) {
                GPIOC->ODR &= ~(1<<LED);
                ms_delay(500);
                GPIOC->ODR |= (1<<LED);
                ms_delay(500);
            }
            symbol_m[0] = read(&lora) - '0';
            symbol_m[1] = read(&lora) - '0';
            symbol = symbol_m[0] * 10 + symbol_m[1];

            beginPacket(&lora, 0);
            if (symbol == '+') {
                GPIOC->ODR &= ~(1<<LED);
                //Mosfet ON
                GPIOA->ODR |= GPIO_ODR_ODR1;
                timer_on = 0;
                time = 0;
                for (i = 0; i < LEN1-1; ++i)
                    write(&lora, str1[i]);
            }
            else if (symbol == '-') {
                GPIOC->ODR |= (1<<LED);
                //Mosfet OFF
                GPIOA->ODR &= ~GPIO_ODR_ODR1;
                timer_on = 0;
                time = 0;
                for (i = 0; i < LEN2-1; ++i)
                    write(&lora, str2[i]);
            }
            else if (symbol == '0') {
                GPIOC->ODR &= ~(1<<LED);
                //Mosfet ON at 0,5' min
                GPIOA->ODR |= GPIO_ODR_ODR1;
                timer_on = 1;
                time = TIME_SEC * 30;
                for (i = 0; i < LEN31-1; ++i)
                    write(&lora, str31[i]);
                write(&lora, '0');
                write(&lora, '.');
                write(&lora, '5');
                for (i = 0; i < LEN32-1; ++i)
                    write(&lora, str32[i]);
            }
            else if (('1' <= symbol) && (symbol <= '9')) {
                GPIOC->ODR &= ~(1<<LED);
                //Mosfet ON at (symbol-'0') min
                GPIOA->ODR |= GPIO_ODR_ODR1;
                timer_on = 1;
                time = TIME_SEC * 60 * (symbol - '0');
                for (i = 0; i < LEN31-1; ++i)
                    write(&lora, str31[i]);
                write(&lora, symbol);
                for (i = 0; i < LEN32-1; ++i)
                    write(&lora, str32[i]);
            }
            endPacket(&lora, 0);
            packet_size = 0;
        }
        else {
            for (i = 0; i < packet_size; ++i) {
                symbol = read(&lora);
            }
        }
        ms_delay(100);

        if(timer_on == 1) {
            --time;
            if(time <= 0) {
                GPIOC->ODR |= (1<<LED);
                GPIOA->ODR &= ~GPIO_ODR_ODR1;
                timer_on = 0;
                time = 0;
                beginPacket(&lora, 0);
                for (i = 0; i < LEN2-1; ++i)
                    write(&lora, str2[i]);
                endPacket(&lora, 0);
            }
        }
    }
    return 0;
}