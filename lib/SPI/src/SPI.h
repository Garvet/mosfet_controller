#ifndef __SPI_H__
#define __SPI_H__

#include <stm32f1xx.h>


void SPI1_init(uint8_t NSS);

uint8_t SPI1_transfer(uint8_t data);
void SPI1_write(uint8_t data);
uint8_t SPI1_read();

uint16_t SPI1_transfer16(uint16_t data);
void SPI1_write16(uint16_t data);
uint16_t SPI1_read16();

#endif //__SPI_H__