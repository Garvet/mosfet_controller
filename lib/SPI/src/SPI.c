#include "SPI.h"

void SPI1_init(uint8_t NSS) {
    //Включаем тактирование SPI1 и GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN;

    /// --- подключение выходов с платы --- 
    //Для начала сбрасываем все конфигурационные биты в нули
    if (NSS != 0)
        GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);
    GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
    //Настраиваем 
    if (NSS)
        //NSS:  CNF4 = 0x02 (10b) и MODE4 = 0x03 (11b) - режим Alternate function push-pull
        GPIOA->CRL |= GPIO_CRL_CNF4_1 | GPIO_CRL_MODE4_1 | GPIO_CRL_MODE5_0;
    //SCK:  CNF5 = 0x02 (10b) и MODE5 = 0x03 (11b) - режим Alternate function push-pull
    GPIOA->CRL |= GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_1 | GPIO_CRL_MODE5_0;
    //MISO: CNF6 = 0x01 (01b) и MODE6 = 0x00 (00b) - _Input floating_ / Input pull-up
    // GPIOA->CRL |= GPIO_CRL_CNF6_0;
    GPIOA->CRL |= GPIO_CRL_CNF6_1;
    GPIOA->ODR |= GPIO_ODR_ODR6;
    //MOSI: CNF7 = 0x02 (10b) и MODE7 = 0x03 (11b) - режим Alternate function push-pull
    GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_1 | GPIO_CRL_MODE5_0;

    /// --- подключение SPI1 --- 
    SPI1->CR1 &= ~SPI_CR1_BIDIMODE; // Дуплекстый режим (при 1 полудуплексный)
    SPI1->CR1 &= ~SPI_CR1_DFF;      // Размер кадра 8 бит (при 1 = 16 Бит)
    SPI1->CR1 &= ~SPI_CR1_LSBFIRST; // MSB first (при 1 = LSB first)
    SPI1->CR1 |= SPI_CR1_SSM;       // Программное управление SS
    SPI1->CR1 |= SPI_CR1_SSI;       // SS в высоком состоянии
    // делитель скорости 000:2, 001:4, 010:8, 011:16, 100:32, 101:64, 110:128, 111:256
    // SPI1 - шины ABP2 (быстрая), SPI1 - ABP1 (медленная)
    SPI1->CR1 |= SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;  // Скорость передачи: F_PCLK/32
    SPI1->CR1 |= SPI_CR1_MSTR;  //Режим Master (ведущий)
    SPI1->CR1 |= SPI_CR1_CPOL; // при простое значение тактового сигнала = 0/1
    // SPI1->CR1 &= ~SPI_CR1_CPOL; // при простое значение тактового сигнала = 0/1
    SPI1->CR1 |= SPI_CR1_CPHA; // 1-й переход - край захвата данных / 2-й (соотв. при 0 и при 1)
    // SPI1->CR1 &= ~SPI_CR1_CPHA; // 1-й переход - край захвата данных / 2-й (соотв. при 0 и при 1)

    // Включаем SPI
    SPI1->CR1 |= SPI_CR1_SPE; 
}



uint8_t SPI1_transfer(uint8_t data) {
    // while(!((SPI1->SR & SPI_SR_TXE)&(SPI1->SR & SPI_SR_RXNE)));
    while(!((SPI1->SR & SPI_SR_TXE)));
    SPI1->DR = data;
    while(!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

void SPI1_write(uint8_t data) {
    //Ждем, пока не освободится буфер передатчика
    while(!(SPI1->SR & SPI_SR_TXE));
    //заполняем буфер передатчика
    SPI1->DR = data;
}

uint8_t SPI1_read() {
    SPI1->DR = 0; // Зпускаем обмен
    // Ждем, пока не появится новое значение в буфере приемника
    while(!(SPI1->SR & SPI_SR_RXNE));
    // Возвращаем значение буфера приемника
    return SPI1->DR;
}



uint16_t SPI1_transfer16(uint16_t data) {
    // while(!((SPI1->SR & SPI_SR_TXE)&(SPI1->SR & SPI_SR_RXNE)));
    while(!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while(!(SPI1->SR & SPI_SR_RXNE));
    return SPI1->DR;
}

void SPI1_write16(uint16_t data) {
    //Ждем, пока не освободится буфер передатчика
    while(!(SPI1->SR & SPI_SR_TXE));
    //заполняем буфер передатчика
    SPI1->DR = data;
}

uint16_t SPI1_read16() {
    SPI1->DR = 0; // Зпускаем обмен
    // Ждем, пока не появится новое значение в буфере приемника
    while(!(SPI1->SR & SPI_SR_RXNE));
    // Возвращаем значение буфера приемника
    return SPI1->DR;
}