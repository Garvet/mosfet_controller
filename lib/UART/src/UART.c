#include "UART.h"

int16_t _uart_init(USART_TypeDef *UARTx, const UARTInitStructure_t *init);

// UART1
int16_t UART1_init(const UARTInitStructure_t *init);
int16_t UART1_put(uint8_t symbol);
int16_t UART1_get(); 
int16_t UART1_print(uint8_t symbol);
int16_t UART1_scan();
int16_t UART1_bytesRB(); 
int16_t UART1_bytesWB(); 
void UART1_clearRB(); 
void UART1_clearWB();

// UART2
int16_t UART2_init(const UARTInitStructure_t *init);
int16_t UART2_put(uint8_t symbol);
int16_t UART2_get(); 
int16_t UART2_print(uint8_t symbol);
int16_t UART2_scan();
int16_t UART2_bytesRB(); 
int16_t UART2_bytesWB(); 
void UART2_clearRB(); 
void UART2_clearWB();

// UARTx
int16_t UART_init(uint8_t id, const UARTInitStructure_t *init) {
    switch(id) {
    case 1: return UART1_init(init);
    case 2: return UART2_init(init);
    default: return -1;
    }
}

int16_t UART_put(uint8_t id, uint8_t symbol) {
    switch(id) {
    case 1: return UART1_put(symbol);
    case 2: return UART2_put(symbol);
    default: return -1;
    }
}

int16_t UART_get(uint8_t id) {
    switch(id) {
    case 1: return UART1_get();
    case 2: return UART2_get();
    default: return -1;
    }
}

int16_t UART_print(uint8_t id, uint8_t symbol) {
    switch(id) {
    case 1: return UART1_print(symbol);
    case 2: return UART2_print(symbol);
    default: return -1;
    }
}

int16_t UART_scan(uint8_t id) {
    switch(id) {
    case 1: return UART1_scan();
    case 2: return UART2_scan();
    default: return -1;
    }
}

// --- Получить количество непрочитанных байт в буфере приемника --- 
int16_t UART_bytesRB(uint8_t id) {
    switch(id) {
    case 1: return UART1_bytesRB();
    case 2: return UART2_bytesRB();
    default: return -1;
    }
}

// --- Получить количество еще не отправленных байт в буфере передатчика --- 
int16_t UART_bytesWB(uint8_t id) {
    switch(id) {
    case 1: return UART1_bytesWB();
    case 2: return UART2_bytesWB();
    default: return -1;
    }
}

// --- Очистить буфер приемника --- 
void UART_clearRB(uint8_t id) {
    switch(id) {
    case 1: return UART1_clearRB();
    case 2: return UART2_clearRB();
    default: return -1;
    }
}

// --- Очистить буфер передатчика --- 
void UART_clearWB(uint8_t id) {
    switch(id) {
    case 1: return UART1_clearWB();
    case 2: return UART2_clearWB();
    default: return -1;
    }
}


// ----- ----- -----
// ----- UART1 -----
// ----- ----- -----
// ПОРТЫ:
//  |     |  DEFAULT | REMAP |
//  | TX1 |    PA9   |  PB6  |
//  | RX1 |    PA10  |  PB7  |
//  
// ШИНА: APB2
// ----- ----- -----


int16_t UART1_init(const UARTInitStructure_t *init) {
    // Включаем тактирование модуля UART
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // Настройка портов
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
      
    // Настройка TX (PA9) - Alternate Function output (Push-pull | Maximum output speed 10 MHz)
        // CNF = 0x10; MODE = 0x01
    GPIOA->CRH |= GPIO_CRH_CNF9_1;
    GPIOA->CRH &= ~GPIO_CRH_CNF9_0;
    GPIOA->CRH &= ~GPIO_CRH_MODE9_1;
    GPIOA->CRH |= GPIO_CRH_MODE9_0;

    // Настройка RX (PA10) - Input pull-up
        // CNF = 0x10; MODE = 0x00
    GPIOA->CRH |= GPIO_CRH_CNF10_1;
    GPIOA->CRH &= ~GPIO_CRH_CNF10_0;
    GPIOA->CRH &= ~GPIO_CRH_MODE10_1;
    GPIOA->CRH &= ~GPIO_CRH_MODE10_0;
        // Подтягиваем RX к единице
    GPIOA->BSRR |= GPIO_BSRR_BS10;

    // Сброс модуля
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    asm("nop");
    asm("nop");
    asm("nop");
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;
    asm("nop");
    asm("nop");
    asm("nop");
 
    // Инициализация основных регистров UART
    if(_uart_init(USART1, init) < 0)
      return -1;
    
    // Запускаем UART
    USART1->CR1 |= USART_CR1_UE;

    return 0;
}

int16_t UART1_put(uint8_t symbol) {
    if(USART1->SR & USART_SR_TXE) {
        return (USART1->DR = symbol);
        // return symbol;
    }
    return -1;
}

int16_t UART1_get() {
    if(USART1->SR & USART_SR_RXNE)
        return USART1->DR;
    return -1;
}

int16_t UART1_print(uint8_t symbol) { 
    int16_t res = -1;
    while (res != symbol) {
        res = UART1_put(symbol);
    }
    return res;
}

int16_t UART1_scan() {
    int16_t symbol = -1;
    while (symbol == -1) {
        symbol = UART1_get();
    }
    return symbol;
}

int16_t UART1_bytesRB() {

}
 
int16_t UART1_bytesWB() {

}
 
void UART1_clearRB() {

}
 
void UART1_clearWB() {

}


















// ----- ----- -----
// ----- UART2 -----
// ----- ----- -----
// ПОРТЫ:
//  |     |  DEFAULT | REMAP |
//  | TX2 |    PA2   |  PD5  |
//  | RX2 |    PA3   |  PD6  |
//  
// ШИНА: APB1
// ----- ----- -----


int16_t UART2_init(const UARTInitStructure_t *init) {
    // Включаем тактирование модуля UART
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Настройка портов
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
      
    // Настройка TX (PA9) - Alternate Function output (Push-pull | Maximum output speed 10 MHz)
        // CNF = 0x10; MODE = 0x01
    GPIOA->CRL |= GPIO_CRL_CNF2_1;
    GPIOA->CRL &= ~GPIO_CRL_CNF2_0;
    GPIOA->CRL &= ~GPIO_CRL_MODE2_1;
    GPIOA->CRL |= GPIO_CRL_MODE2_0;

    // Настройка RX (PA10) - Input pull-up
        // CNF = 0x10; MODE = 0x00
    GPIOA->CRL |= GPIO_CRL_CNF3_1;
    GPIOA->CRL &= ~GPIO_CRL_CNF3_0;
    GPIOA->CRL &= ~GPIO_CRL_MODE3_1;
    GPIOA->CRL &= ~GPIO_CRL_MODE3_0;
        // Подтягиваем RX к единице
    GPIOA->BSRR |= GPIO_BSRR_BS3;

    // Сброс модуля
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    asm("nop");
    asm("nop");
    asm("nop");
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;
    asm("nop");
    asm("nop");
    asm("nop");
 
    // Инициализация основных регистров UART
    if(_uart_init(USART2, init) < 0)
      return -1;
    
    // Запускаем UART
    USART2->CR1 |= USART_CR1_UE;

    return 0;
}

int16_t UART2_put(uint8_t symbol) {
    if(USART2->SR & USART_SR_TXE) {
        return (USART2->DR = symbol);
        // return symbol;
    }
    return -1;
}

int16_t UART2_get() {
    if(USART2->SR & USART_SR_RXNE)
        return USART2->DR;
    return -1;
}

int16_t UART2_print(uint8_t symbol) { 
    int16_t res = -1;
    while (res != symbol) {
        res = UART2_put(symbol);
    }
    return res;
}

int16_t UART2_scan() {
    int16_t symbol = -1;
    while (symbol == -1) {
        symbol = UART2_get();
    }
    return symbol;
}

int16_t UART2_bytesRB() {

}
 
int16_t UART2_bytesWB() {

}
 
void UART2_clearRB() {

}
 
void UART2_clearWB() {

}



















int16_t _uart_init(USART_TypeDef *UARTx, const UARTInitStructure_t *init) {
    uint32_t UARTx_CR1_VAL = 0;
    
    //установка скорости UART
    UARTx->BRR = (uint16_t)(init->bus_freq / init->baud);
    // UARTx->BRR = (uint16_t)(init->bus_freq);
    
    UARTx_CR1_VAL |= USART_CR1_TE | USART_CR1_RE;
    
    switch(init->data_bits) {
    case 8: break;
    case 9: UARTx_CR1_VAL |= USART_CR1_M; break;
    default: return -1;
    }
    
    switch(init->parity) {
    //None
    case 0: break; 
    //Even 
    case 1: UARTx_CR1_VAL |= USART_CR1_PCE; break;
    //Odd
    case 2: UARTx_CR1_VAL |= USART_CR1_PCE | USART_CR1_PS; break;
    default: return -1;
    }
    
    UARTx->CR1 = UARTx_CR1_VAL;
    
    switch(init->stop_bits) {
    case 1: break;
    case 2: UARTx->CR2 |= USART_CR2_STOP; break;
    default: return -1;
    }
    
    return 0;
}